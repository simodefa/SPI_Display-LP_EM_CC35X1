/*
 * lvgl_port.h
 *
 * LVGL v9 display port for CC3551E SPI_Display project.
 * Bridges the ST7789 / ST7735 low-level drivers to LVGL's display API.
 *
 * Usage:
 *   1. Call LCD_init() first (from st7789_lcd.h or st7735_lcd.h).
 *   2. Call lv_init().
 *   3. Call lvgl_port_init() — registers the display with LVGL.
 *   4. Drive the LVGL task pump from your display thread:
 *          lv_timer_handler();
 *          usleep(5000);   // 5 ms
 */

#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#ifdef USE_LVGL

#include "lvgl.h"

/**
 * Initialize the LVGL display port.
 *
 * Creates the LVGL display object, allocates draw buffers, and registers
 * the flush callback that forwards rendered pixels to the LCD driver.
 *
 * Prerequisites:
 *   - LCD_init() must already have been called.
 *   - lv_init() must already have been called.
 */
void lvgl_port_init(void);

#endif /* USE_LVGL */
#endif /* LVGL_PORT_H */
