#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

/*
 * Display Configuration
 *
 * Step 1 — Select display controller (uncomment exactly one):
 *   USE_ST7789  ST7789 240×320 pixels  (Waveshare 2" LCD, default)
 *   USE_ST7735  ST7735 128×128 pixels  (TI BOOSTXL-EDUMKII)
 *
 * Step 2 — Select rendering mode (uncomment to enable LVGL):
 *   USE_LVGL    Use LVGL v9 for UI rendering instead of the built-in
 *               color-cycling demo.  Requires the lvgl/ git submodule.
 *               See README.md → "LVGL Integration" for setup instructions.
 */

/* --- Display controller (pick one) --- */
// #define USE_ST7789
// #define USE_ST7735
#define USE_ILI9341

/* --- Optional: LVGL rendering engine --- */
#define USE_LVGL

#endif /* DISPLAY_CONFIG_H */
