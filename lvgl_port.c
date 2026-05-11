/*
 * lvgl_port.c
 *
 * LVGL v9 display port for CC3551E SPI_Display project.
 *
 * What this file does:
 *   - Creates an lv_display_t backed by a partial draw buffer
 *   - Implements the flush callback that converts LVGL's rendered output
 *     (little-endian RGB565) to big-endian and sends it through
 *     LCD_drawContentFrame() to the physical display
 *   - Provides a 1 ms tick task that drives lv_tick_inc()
 *
 * Flush strategy:
 *   LVGL renders one horizontal band at a time into LV_DRAW_BUF_LINES rows.
 *   The flush callback receives the dirty rectangle (lv_area_t) and the
 *   rendered pixel buffer.  It calls LCD_drawContentFrame() which:
 *     - sets the CASET/RASET address window
 *     - blasts the pixel data in 64-pixel chunks (already done inside the driver)
 *
 *   Because LCD_drawContentFrame() already handles the big-endian byte swap
 *   internally, we pass the raw lv_color_t buffer and let the driver convert.
 */

#include "display_config.h"

#ifdef USE_LVGL

#include "lvgl_port.h"

#include <stdint.h>
#include <unistd.h>

/* Pull in the active LCD driver header (LCD_WIDTH, LCD_HEIGHT, LCD_drawContentFrame) */
#ifdef USE_ST7789
#include "st7789_lcd.h"
#elif defined(USE_ST7735)
#include "st7735_lcd.h"
#elif defined(USE_ILI9341)
#include "ili9341_lcd.h"
#endif

/*
 * Draw buffer: LVGL renders LV_DRAW_BUF_LINES rows at a time.
 * Larger values reduce the number of flush calls per frame but cost more RAM.
 *   ST7789 (240 px wide): 10 lines × 240 × 2 bytes = 4800 bytes
 *   ST7735 (128 px wide): 10 lines × 128 × 2 bytes = 2560 bytes
 */
#define LV_DRAW_BUF_LINES 10

/* Static draw buffer — sized for the widest supported display (ST7789 240 px) */
static lv_color_t draw_buf[LCD_WIDTH * LV_DRAW_BUF_LINES];

/* -----------------------------------------------------------------------
 * Flush callback
 *
 * LVGL calls this when a rectangular region has been rendered.  We forward
 * it to LCD_drawContentFrame() which handles the SPI transfer.
 * ----------------------------------------------------------------------- */
static void lvgl_flush_cb(lv_display_t *disp,
                          const lv_area_t *area,
                          uint8_t *px_map)
{
    /*
     * area->x1/y1/x2/y2 are int32_t in LVGL v9.
     * LCD_drawRegion takes uint16_t — safe cast since coords are always
     * within display bounds (0..239 for ST7789, 0..127 for ST7735).
     *
     * px_map is little-endian RGB565. LCD_drawRegion performs the
     * big-endian byte swap before writing to SPI.
     */
    LCD_drawRegion((uint16_t)area->x1, (uint16_t)area->y1,
                   (uint16_t)area->x2, (uint16_t)area->y2,
                   (const uint16_t *)px_map);

    /* Inform LVGL that flush is complete (synchronous — no DMA) */
    lv_display_flush_ready(disp);
}

/* -----------------------------------------------------------------------
 * lvgl_port_init
 * ----------------------------------------------------------------------- */
void lvgl_port_init(void)
{
    /* Create display object matching physical dimensions */
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

    /* Set color format — RGB565 matches both ST7789 and ST7735 */
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

    /* Register flush callback */
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    /*
     * Set partial draw buffer.
     * LV_DISPLAY_RENDER_MODE_PARTIAL tells LVGL to render the screen in
     * horizontal bands rather than requiring a full-screen buffer.
     */
    lv_display_set_buffers(disp, draw_buf, NULL,
                           sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
}

/* -----------------------------------------------------------------------
 * LVGL tick task
 *
 * Runs as a FreeRTOS/POSIX thread.  Sleeps 1 ms per iteration and calls
 * lv_tick_inc(1) to advance LVGL's internal time base.
 * ----------------------------------------------------------------------- */
void *lvgl_port_tick_task(void *arg)
{
    (void)arg;
    while (1) {
        usleep(1000);       /* 1 ms */
        lv_tick_inc(1);
    }
    return NULL;
}

#endif /* USE_LVGL */
