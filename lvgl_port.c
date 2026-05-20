/*
 * lvgl_port.c
 *
 * LVGL v9 display port for CC3551E SPI_Display project.
 *
 * What this file does:
 *   - Creates an lv_display_t backed by two partial draw buffers
 *   - A dedicated flush task handles every band flush:
 *       1. Waits for a flush request from the LVGL task
 *       2. Starts a DMA SPI transfer via LCD_drawRegionAsync()
 *       3. Blocks on LCD_waitDmaDone() until the DMA finishes
 *       4. Calls lv_display_flush_ready() so LVGL can reuse the buffer
 *   - Provides a 1 ms tick source via lv_tick_set_cb()
 *
 * Double-buffer strategy:
 *   LVGL alternates between draw_buf and draw_buf2 in
 *   LV_DISPLAY_RENDER_MODE_PARTIAL.  While the flush task is DMA-ing one
 *   band, LVGL renders the next band into the other buffer — maximising
 *   CPU / DMA parallelism and keeping the display pipeline full.
 *
 * Thread model:
 *   LVGL task   →  lvgl_flush_cb() posts gFlushReqSem, returns immediately
 *   Flush task  →  wakes on gFlushReqSem, runs DMA, calls flush_ready()
 */

#include "display_config.h"

#ifdef USE_LVGL

#include "lvgl_port.h"

#include <stdint.h>
#include <unistd.h>

/* FreeRTOS */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

/* Pull in the active LCD driver header (LCD_WIDTH, LCD_HEIGHT,
 * LCD_drawRegionAsync, LCD_waitDmaDone) */
#ifdef USE_ST7789
#include "st7789_lcd.h"
#elif defined(USE_ST7735)
#include "st7735_lcd.h"
#elif defined(USE_ILI9341)
#include "ili9341_lcd.h"
#endif

static uint32_t lvgl_tick_cb(void)
{
    return (uint32_t)xTaskGetTickCount();
}

/*
 * Draw buffers: LVGL alternates between the two, rendering into one while
 * the other is being DMA'd to the display.
 *
 * Size per buffer: LCD_WIDTH × LV_DRAW_BUF_LINES × 2 bytes (RGB565).
 * Adjust LV_DRAW_BUF_LINES to balance RAM vs flush granularity:
 *   ILI9341  320 px wide, 10 lines → 6 400 B per buffer (12 800 B total)
 *   ST7789   240 px wide, 10 lines → 4 800 B per buffer ( 9 600 B total)
 *   ST7735   128 px wide, 10 lines → 2 560 B per buffer ( 5 120 B total)
 */
#define LV_DRAW_BUF_LINES 10

static lv_color_t draw_buf[LCD_WIDTH * LV_DRAW_BUF_LINES];
static lv_color_t draw_buf2[LCD_WIDTH * LV_DRAW_BUF_LINES];

/* -----------------------------------------------------------------------
 * Flush thread state
 * ----------------------------------------------------------------------- */

/* Binary semaphore: LVGL task posts it when a band is ready to flush.
 * Flush task blocks on it, then does the DMA transfer. */
static SemaphoreHandle_t gFlushReqSem;

/* LVGL display handle, flush rectangle, and pixel pointer — written by
 * lvgl_flush_cb (LVGL task) and read by lvgl_flush_task (flush task).
 * Access is serialised: the flush task only reads after taking gFlushReqSem,
 * and LVGL only writes before posting it. */
static lv_display_t  *gDisplay;
static lv_area_t      gFlushArea;
static const uint8_t *gFlushPixmap;

/* -----------------------------------------------------------------------
 * Flush callback (called from the LVGL task)
 *
 * Stores the dirty rectangle and pixel buffer then wakes the flush task.
 * Does NOT call lv_display_flush_ready() — the flush task does that after
 * the DMA transfer completes.
 * ----------------------------------------------------------------------- */
static void lvgl_flush_cb(lv_display_t *disp,
                          const lv_area_t *area,
                          uint8_t *px_map)
{
    gDisplay     = disp;
    gFlushArea   = *area;
    gFlushPixmap = (const uint8_t *)px_map;
    xSemaphoreGive(gFlushReqSem);
}

/* -----------------------------------------------------------------------
 * Flush task
 *
 * Dedicated high-priority task that serialises all SPI transfers.
 * For ILI9341, LCD_drawRegionAsync() launches a DMA transfer and returns
 * immediately; LCD_waitDmaDone() blocks until the ISR signals completion.
 * For ST7789 / ST7735, both calls are synchronous (the SPI is not yet
 * running in DMA mode on those drivers), but the separate thread still
 * allows LVGL to render the next band in parallel on the CPU.
 * ----------------------------------------------------------------------- */
#define FLUSH_TASK_STACK_WORDS  512u
#define FLUSH_TASK_PRIORITY     (configMAX_PRIORITIES - 1u)

static StackType_t  gFlushTaskStack[FLUSH_TASK_STACK_WORDS];
static StaticTask_t gFlushTaskTCB;

static void lvgl_flush_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        /* Wait until LVGL has rendered a band and posted gFlushReqSem */
        xSemaphoreTake(gFlushReqSem, portMAX_DELAY);

        /* Start the DMA pixel transfer (returns immediately on ILI9341) */
        LCD_drawRegionAsync(
            (uint16_t)gFlushArea.x1, (uint16_t)gFlushArea.y1,
            (uint16_t)gFlushArea.x2, (uint16_t)gFlushArea.y2,
            (const uint16_t *)gFlushPixmap);

        /* Block until DMA (or synchronous SPI) is done */
        LCD_waitDmaDone();

        /* Release the buffer back to LVGL — it may now render the next band */
        lv_display_flush_ready(gDisplay);
    }
}

/* -----------------------------------------------------------------------
 * lvgl_port_init
 * ----------------------------------------------------------------------- */
void lvgl_port_init(void)
{
    lv_tick_set_cb(lvgl_tick_cb);

    /* Semaphore starts empty; flush task blocks until LVGL posts to it */
    gFlushReqSem = xSemaphoreCreateBinary();

    /* Create display object matching physical dimensions */
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

    /* Set color format — RGB565 big-endian matches all supported controllers */
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);

    /* Register flush callback */
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    /*
     * Double-buffered partial rendering:
     *   draw_buf  — LVGL renders into this; flush task DMA's it to the display
     *   draw_buf2 — LVGL renders the next band here while draw_buf is DMA-ing
     * LV_DISPLAY_RENDER_MODE_PARTIAL: LVGL renders LV_DRAW_BUF_LINES rows
     * at a time, keeping RAM usage low while still enabling parallelism.
     */
    lv_display_set_buffers(disp, draw_buf, draw_buf2,
                           sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Spawn the dedicated flush thread at the highest priority so DMA
     * acknowledgement is never delayed by lower-priority work */
    xTaskCreateStatic(lvgl_flush_task,
                      "lvgl_flush",
                      FLUSH_TASK_STACK_WORDS,
                      NULL,
                      FLUSH_TASK_PRIORITY,
                      gFlushTaskStack,
                      &gFlushTaskTCB);
}

#endif /* USE_LVGL */
