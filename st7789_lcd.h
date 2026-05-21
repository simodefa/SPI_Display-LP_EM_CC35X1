/*
 * ST7789 LCD driver for 240x320 TFT display
 * Uses TI Drivers SPI and GPIO.
 *
 * Pin mapping (LP-EM-CC35X1 + ST7789 Display):
 *   SPI CLK  -> GPIO27 (BP pin 7,  SPI0 SCLK)
 *   SPI MOSI -> GPIO29 (BP pin 15, SPI0 PICO)
 *   CS       -> GPIO13 (BP pin 13, software GPIO)
 *   DC/RS    -> GPIO34 (BP pin 31, data/command select)
 *   RST      -> GPIO30 (BP pin 17, active-low reset)
 *   Backlight -> BP pin 1 (or any other 3.3V header)
 */
#ifndef ST7789_LCD_H
#define ST7789_LCD_H

#include <stdint.h>

/* Display dimensions */
#define LCD_WIDTH   240
#define LCD_HEIGHT  320

/* RGB565 color helpers */
#define LCD_COLOR_BLACK   0x0000u
#define LCD_COLOR_WHITE   0xFFFFu
#define LCD_COLOR_RED     0xF800u
#define LCD_COLOR_GREEN   0x07E0u
#define LCD_COLOR_BLUE    0x001Fu

/**
 * Initialize the ST7789 LCD.
 * Must be called once before any draw functions.
 */
void LCD_init(void);

/**
 * Fill the entire 240x320 display with one color.
 * @param color  RGB565 color value
 */
void LCD_fillScreen(uint16_t color);

/**
 * Draw a filled rectangle.
 * @param x, y   top-left corner
 * @param w, h   width and height in pixels
 * @param color  RGB565 color
 */
void LCD_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * Blit a 240x320 RGB565 image to the display.
 * @param image  pointer to LCD_WIDTH * LCD_HEIGHT uint16_t pixels
 */
void LCD_drawImage(const uint16_t *image);

/**
 * Blit an arbitrary rectangle of RGB565 pixels to the display.
 * Used by the LVGL flush callback — full uint16_t coordinates, no centering.
 * @param x0,y0  top-left pixel (inclusive)
 * @param x1,y1  bottom-right pixel (inclusive)
 * @param pixels pointer to (x1-x0+1)*(y1-y0+1) RGB565 values, row-major
 */
void LCD_drawRegion(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                    const uint16_t *pixels);

/**
 * Blit a partial-height RGB565 frame into a horizontal band on the display.
 * @param frame          pointer to frameWidth * contentHeight uint16_t pixels
 * @param yOffset        first row on the display to write (0-based)
 * @param contentHeight  number of rows to write
 * @param frameWidth     number of pixels per row in frame (centered if < LCD_WIDTH)
 */
void LCD_drawContentFrame(const uint16_t *frame, uint8_t yOffset, uint8_t contentHeight,
                          uint16_t frameWidth);

#endif /* ST7789_LCD_H */
