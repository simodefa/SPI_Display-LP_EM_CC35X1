/*
 * Copyright (c) 2018-2024, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== spi_display.c ========
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* POSIX Header files */
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SPI.h>
#include <ti/display/Display.h>

/* Driver configuration */
#include "ti_drivers_config.h"
#include "display_config.h"

/* LCD driver selection - configure in display_config.h */
#ifdef USE_ST7789
#include "st7789_lcd.h"
#elif defined(USE_ST7735)
#include "st7735_lcd.h"
#elif defined(USE_ILI9341)
#include "ili9341_lcd.h"
#else
#error "No display configured! Edit display_config.h to select ST7789 or ST7735"
#endif

/* Optional LVGL rendering engine */
#ifdef USE_LVGL
#include "lvgl.h"
#include "lvgl_port.h"
#include "demos/lv_demos.h"
#endif

#define THREADSTACKSIZE (8192)  /* LVGL requires significant stack for rendering */

#ifdef DeviceFamily_CC35XX
    #define CONFIG_GPIO_LED_0 GPIO_INVALID_INDEX
    #define CONFIG_GPIO_LED_1 GPIO_INVALID_INDEX
#endif

static Display_Handle display;

/* ========== Animated Pattern Demo ========== */
static uint32_t frame_count = 0;

#ifdef USE_LVGL
/* -----------------------------------------------------------------------
 * LVGL demo screen
 *
 * Builds a simple UI with a label, a spinner, and a progress bar that
 * updates every frame.  Replace or extend this to suit your application.
 * SquareLine Studio users: replace the body of lvgl_demo_create() with
 * a call to ui_init() from the exported ui/ui.h header.
 * ----------------------------------------------------------------------- */
static lv_obj_t *g_progress_bar = NULL;
static lv_obj_t *g_label_fps    = NULL;

static void lvgl_demo_create(void)
{
    lv_obj_t *scr = lv_screen_active();

    /* Dark background */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1A1A2E), LV_PART_MAIN);

#ifdef USE_ST7735
    /*
     * ST7735 128x128 compact layout:
     *   Title  (12px font) — top, 4px margin
     *   Spinner (36x36)   — center
     *   Progress bar      — 14px above bottom
     *   Status label      — bottom, 4px margin
     */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "CC3551E + LVGL v9");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE94560), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    lv_obj_t *spinner = lv_spinner_create(scr);
    lv_obj_set_size(spinner, 36, 36);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -8);

    g_progress_bar = lv_bar_create(scr);
    lv_obj_set_size(g_progress_bar, LCD_WIDTH - 20, 8);
    lv_obj_align(g_progress_bar, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_bar_set_range(g_progress_bar, 0, 100);
    lv_bar_set_value(g_progress_bar, 0, LV_ANIM_OFF);

    g_label_fps = lv_label_create(scr);
    lv_label_set_text(g_label_fps, "--");
    lv_obj_set_style_text_color(g_label_fps, lv_color_hex(0xA8DADC), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_label_fps, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(g_label_fps, LV_ALIGN_BOTTOM_MID, 0, -4);

#else
    /*
     * ST7789 240x320 standard layout:
     *   Title  (16px font) — top, 12px margin
     *   Spinner (60x60)   — center, shifted up
     *   Progress bar      — 30px above bottom
     *   Status label      — bottom, 10px margin
     */
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "CC3551E + LVGL v9");
    lv_obj_set_style_text_color(title, lv_color_hex(0xE94560), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 12);

    lv_obj_t *spinner = lv_spinner_create(scr);
    lv_obj_set_size(spinner, 60, 60);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -20);

    g_progress_bar = lv_bar_create(scr);
    lv_obj_set_size(g_progress_bar, LCD_WIDTH - 30, 12);
    lv_obj_align(g_progress_bar, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_bar_set_range(g_progress_bar, 0, 100);
    lv_bar_set_value(g_progress_bar, 0, LV_ANIM_OFF);

    g_label_fps = lv_label_create(scr);
    lv_label_set_text(g_label_fps, "--");
    lv_obj_set_style_text_color(g_label_fps, lv_color_hex(0xA8DADC), LV_PART_MAIN);
    lv_obj_align(g_label_fps, LV_ALIGN_BOTTOM_MID, 0, -10);
#endif
}

static void lvgl_demo_update(uint32_t render_ms)
{
    if (g_progress_bar) {
        /* Cycle the bar value 0-100 over 100 frames */
        int32_t val = (int32_t)(frame_count % 101);
        lv_bar_set_value(g_progress_bar, val, LV_ANIM_OFF);
    }
    if (g_label_fps) {
        /* Show render time per frame — more stable than a derived FPS counter */
        if (render_ms > 0)
            lv_label_set_text_fmt(g_label_fps, "%lu ms/frame", (unsigned long)render_ms);
    }
}

#if LV_USE_LOG
void print_cb(lv_log_level_t level, const char * buf)
{
    LV_UNUSED(level);
    Display_printf(display, 0, 0, "\r%s\n", buf);
}
#endif
#endif /* USE_LVGL */

/* Color palette */
static const uint16_t color_palette[] = {
    0xF800,  /* RED */
    0xF840,  /* YELLOW */
    0x07E0,  /* GREEN */
    0x07FF,  /* CYAN */
    0x001F,  /* BLUE */
    0xF81F   /* MAGENTA */
};
#define PALETTE_SIZE (sizeof(color_palette) / sizeof(color_palette[0]))

/* Simple color bar animation - just cycles through colors filling the screen */
void draw_color_bar_pattern(void)
{
    int color_idx = (frame_count / 30) % PALETTE_SIZE;
    uint16_t color = color_palette[color_idx];
    LCD_fillScreen(color);
}

/*
 *  ======== displayThread ========
 */
void *displayThread(void *arg0)
{
    /* Initialize the LCD */
    LCD_init();

    Display_printf(display, 0, 0, "");
    Display_printf(display, 0, 0, "========================================");
#ifdef USE_LVGL
    Display_printf(display, 0, 0, "SPI Display + LVGL v9 Demo");
#else
    Display_printf(display, 0, 0, "SPI Display Color Animation Demo");
#endif
    Display_printf(display, 0, 0, "========================================");
    Display_printf(display, 0, 0, "MCU: CC3551E");
#ifdef USE_ST7789
    Display_printf(display, 0, 0, "Display: ST7789 (240x320 pixels)");
#endif
#ifdef USE_ST7735
    Display_printf(display, 0, 0, "Display: ST7735 (128x128 pixels)");
#endif
#ifdef USE_ILI9341
    Display_printf(display, 0, 0, "Display: ILI9341 (240x320 pixels)");
#endif
#ifdef USE_LVGL
    Display_printf(display, 0, 0, "Renderer: LVGL v9");
#endif
    Display_printf(display, 0, 0, "Status: Initializing...");
    Display_printf(display, 0, 0, "========================================");
    Display_printf(display, 0, 0, "");

#ifdef USE_LVGL
    /* ---- LVGL path ---------------------------------------------------- */
    lv_init();
    lvgl_port_init();

#if LV_USE_LOG
    lv_log_register_print_cb(print_cb);
#endif

    /* Build the demo UI (replace with ui_init() for SquareLine projects) */
    // lvgl_demo_create();
    lv_demo_benchmark();

    Display_printf(display, 0, 0, "LVGL ready. Running UI loop.");

    uint32_t last_logged_frame = 0;
    uint32_t batch_start_tick  = lv_tick_get(); /* ms from LVGL tick — always valid */

    while (1)
    {
#if 0
        /* Drive LVGL rendering and animation engine */
        lv_timer_handler();

        frame_count++;

        /* Log every 100 lv_timer_handler calls */
        if (frame_count - last_logged_frame >= 100) {
            uint32_t now_tick   = lv_tick_get();
            uint32_t elapsed_ms = now_tick - batch_start_tick; /* wraps safely at UINT32_MAX */
            uint32_t ms_per_frame = elapsed_ms / 100;

            last_logged_frame = frame_count;
            batch_start_tick  = now_tick;

            /* Update on-screen label */
            lvgl_demo_update(ms_per_frame);

            Display_printf(display, 0, 0,
                "Frames: %lu | Last 100 frames: %lu ms | ~%lu ms/frame",
                (unsigned long)frame_count,
                (unsigned long)elapsed_ms,
                (unsigned long)ms_per_frame);
        }

        usleep(5000);  /* 5 ms — yields to other tasks between renders */
#endif

        usleep(lv_timer_handler() * 1000);
    }

#else
    /* ---- Built-in color-cycling demo path ----------------------------- */
    uint32_t last_logged_frame = 0;

    /* Target frame rate: 24 FPS = ~41.67ms per frame */
    uint32_t target_frame_ms = 40;  /* milliseconds per frame */
    struct timespec frame_start, frame_end, demo_start_time;
    uint32_t frame_elapsed_ms;
    uint64_t total_elapsed_ms;

    /* Record demo start time for accurate runtime reporting */
    clock_gettime(CLOCK_MONOTONIC, &demo_start_time);

    /* Animation loop */
    while (1)
    {
        /* Get frame start time */
        clock_gettime(CLOCK_MONOTONIC, &frame_start);

        /* Draw simple color cycling pattern (no need to clear, pattern fills entire screen) */
        draw_color_bar_pattern();

        /* Increment local frame counter */
        frame_count++;

        /* Log statistics every 100 frames */
        if (frame_count - last_logged_frame >= 100) {
            last_logged_frame = frame_count;

            clock_gettime(CLOCK_MONOTONIC, &frame_end);
            total_elapsed_ms = (frame_end.tv_sec - demo_start_time.tv_sec) * 1000 +
                               (frame_end.tv_nsec - demo_start_time.tv_nsec) / 1000000;

            double elapsed_seconds = total_elapsed_ms / 1000.0;
            double fps = (elapsed_seconds > 0) ? frame_count / elapsed_seconds : 0;

            /* Print status update */
            Display_printf(display, 0, 0, "");
            Display_printf(display, 0, 0, "Status: Runtime: %.1f sec | Frames: %d | FPS: %.2f",
                          elapsed_seconds, frame_count, fps);
        }

        /* Frame rate limiting: maintain consistent 24 FPS */
        clock_gettime(CLOCK_MONOTONIC, &frame_end);
        frame_elapsed_ms = (frame_end.tv_sec - frame_start.tv_sec) * 1000 +
                           (frame_end.tv_nsec - frame_start.tv_nsec) / 1000000;

        if (frame_elapsed_ms < target_frame_ms) {
            uint32_t sleep_ms = target_frame_ms - frame_elapsed_ms;
            usleep(sleep_ms * 1000);
        }
    }
#endif /* USE_LVGL */

    return (NULL);
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    pthread_t thread0;
    pthread_attr_t attrs;
    struct sched_param priParam;
    int retc;
    int detachState;

    /* Call driver init functions. */
    Display_init();
    GPIO_init();
    SPI_init();

    /* Configure the LED pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);

    /* Configure LCD control pins as outputs */
    GPIO_setConfig(CONFIG_GPIO_LCD_CS, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_HIGH);
    GPIO_setConfig(CONFIG_GPIO_LCD_DC, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LCD_RST, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_HIGH);

    /* Open the display for output */
    display = Display_open(Display_Type_UART, NULL);
    if (display == NULL)
    {
        /* Failed to open display driver */
        while (1) {}
    }

    /* Turn on user LED */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);


    /* Create application threads */
    pthread_attr_init(&attrs);

    detachState = PTHREAD_CREATE_DETACHED;
    /* Set priority and stack size attributes */
    retc        = pthread_attr_setdetachstate(&attrs, detachState);
    if (retc != 0)
    {
        /* pthread_attr_setdetachstate() failed */
        while (1) {}
    }

    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
    if (retc != 0)
    {
        /* pthread_attr_setstacksize() failed */
        while (1) {}
    }


    /* Create display thread */
    priParam.sched_priority = 2;
    pthread_attr_setschedparam(&attrs, &priParam);

    retc = pthread_create(&thread0, &attrs, displayThread, NULL);
    if (retc != 0)
    {
        /* pthread_create() failed */
        while (1) {}
    }

    return (NULL);
}
