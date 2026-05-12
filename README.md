# SPI Display Project

SPI display demo for TI CC3551E wireless microcontroller with support for ST7789 (240×320) and ST7735 (128×128) displays.
Uses [LVGL v9](https://lvgl.io) for UI rendering by default, with support for the SquareLine Studio GUI design tool.
A built-in color-cycling demo is also available without LVGL for a minimal starting point.

This example project is based on SIMPLELINK-WIFI-SDK (install separately).

---

## Pre-requisites

1. [CCStudio IDE](https://www.ti.com/tool/download/CCSTUDIO) (v20.5.1 or newer) with TI Clang compiler (v4.0.4 LTS or newer)
2. [SIMPLELINK-SDK-WIFI-SDK](https://www.ti.com/tool/download/SIMPLELINK-WIFI-SDK) (v10.10 or newer)
3. [Wi-Fi Toolbox](https://www.ti.com/tool/download/SIMPLELINK-WIFI-TOOLBOX) (v4.1.16 or newer — tied to SIMPLELINK-SDK-WIFI-SDK installation)
4. [Sysconfig](https://www.ti.com/tool/download/SYSCONFIG) (v1.26.3 or newer — tied to SIMPLELINK-SDK-WIFI-SDK installation)
5. [LP-EM-CC35X1 LaunchPad development kit](https://www.ti.com/tool/LP-EM-CC35X1)
6. ST7789 or ST7735 based SPI display

**Tested hardware:**
- [Waveshare 2inch LCD Display Module](https://www.waveshare.com/2inch-lcd-module.htm) (ST7789 controller)
- [TI BOOSTXL-EDUMKII BoosterPack](https://www.ti.com/tool/BOOSTXL-EDUMKII) (ST7735 controller)

Other display/controller variants may require adjustments to the initialization sequence.

---

## Quick Start

### Hardware Setup

Connect your SPI display to the CC35X1 LaunchPad using these GPIO pins:

| Display Pin | CC35X1 GPIO | BoosterPack Pin | Signal |
|-------------|-------------|-----------------|--------|
| CLK (SCK)   | GPIO27      | BP.7            | SPI0 Clock |
| MOSI (DIN)  | GPIO29      | BP.15           | SPI0 Data |
| CS          | GPIO13      | BP.13           | Chip Select (software) |
| DC (RS)     | GPIO34      | BP.31           | Data/Command |
| RST         | GPIO30      | BP.17           | Reset (active low) |
| GND         | GND         | —               | Ground |
| VCC         | 3.3 V       | BP.1            | Power |

> **Note:** If using the BOOSTXL-EDUMKII with the ST7735 display, stack the BoosterPack directly on the LP-EM-CC35X1. No wiring required.

### Building & Flashing

1. **Clone the repository into the CCS workspace:**
   - In CCS Theia, open **Terminal → New Terminal** — this opens a shell in your CCS workspace directory
   - Run:
   ```bash
   git clone https://github.com/v-parmar-ti/SPI_Display-LP_EM_CC35X1.git SPI_Display
   cd SPI_Display
   git submodule update --init --recursive
   ```

2. **Import into CCS Theia:**
   - File → **Import** → **C/C++ → CCS Project from .projectspec**
   - Browse to `SPI_Display.projectspec` and click **Finish**
   - CCS generates the project configuration automatically — no manual include path setup needed

3. **Select display** — Edit `display_config.h` if needed (ST7789 + LVGL enabled by default):
   ```c
   /* --- Display controller (pick one) --- */
   #define USE_ST7789
   //#define USE_ST7735

   /* --- Optional: LVGL rendering engine --- */
   #define USE_LVGL
   ```

4. **Build** — Right-click project → **Build Project**

5. **Flash** — **Run → Start Debugging**

### Expected Output (default — LVGL enabled)

**Display:** Demo screen with title, animated spinner, cycling progress bar, and render-time label.

**Console (115200 baud, 8N1):**
```
========================================
SPI Display + LVGL v9 Demo
========================================
MCU: CC3551E
Display: ST7789 (240x320 pixels)
Renderer: LVGL v9
Status: Initializing...
========================================
LVGL ready. Running UI loop.
Frames: 100 | Last 100 frames: 812 ms | ~8 ms/frame
Frames: 200 | Last 100 frames: 809 ms | ~8 ms/frame
```

### Expected Output (built-in color demo, LVGL disabled)

To use the simpler built-in demo, comment out `USE_LVGL` in `display_config.h`.

**Display:** Solid colors cycling — Red → Yellow → Green → Cyan → Blue → Magenta

**Console:**
```
========================================
SPI Display Color Animation Demo
========================================
MCU: CC3551E
Display: ST7789 (240x320 pixels)
Status: Initializing...
========================================

Status: Runtime: 4.2 sec | Frames: 100 | FPS: 24.00
```

---

## LVGL Integration

[LVGL](https://lvgl.io) (Light and Versatile Graphics Library) is an open-source embedded UI framework.
This project includes a display port (`lvgl_port.c/h`) and a minimal configuration (`lv_conf.h`) that connect the ST7789/ST7735 drivers to LVGL v9.

### Step 1 — Clone the LVGL submodule

The LVGL submodule reference is already registered in this repository. After cloning, populate it with:

```bash
git submodule update --init --recursive
```

This is already included in the Quick Start clone step. The directory structure after cloning will be:

```
SPI_Display/
├── lvgl/           ← LVGL v9 source (populated by submodule update)
├── lv_conf.h       ← LVGL configuration
├── lvgl_port.c/h   ← Display port
└── ...
```

To pin to a specific LVGL release for reproducible builds:

```bash
cd lvgl/
git checkout v9.2.2   # or any v9.x tag
cd ..
git add lvgl
git commit -m "Pin LVGL submodule to v9.2.2"
```

### Step 2 — Import the project

If you followed the Quick Start, the project is already imported via `SPI_Display.projectspec` and all include paths and preprocessor defines are configured automatically. No manual project properties changes are needed.

If you need to set up the project properties manually (e.g. after recreating the project), add:

**Source path** (Properties → Build → Source Locations):
- `${PROJECT_ROOT}/lvgl/src` — recursive

**Include paths** (Properties → Build → TI Clang Compiler → Include Options):
- `${PROJECT_ROOT}/lvgl`
- `${PROJECT_ROOT}`

**Preprocessor define** (Properties → Build → TI Clang Compiler → Predefined Symbols):
- `LV_CONF_INCLUDE_SIMPLE`

### Step 3 — Verify display_config.h

LVGL is enabled by default. Confirm `display_config.h` matches your connected display:

```c
/* display_config.h */
#define USE_ST7789    /* or USE_ST7735 */
#define USE_LVGL      /* enabled by default */
```

### Step 4 — Build and flash

Build and flash exactly as in the Quick Start section.  
The LVGL demo screen shows a spinner, a cycling progress bar, and a render-time label.

**Console output with LVGL:**
```
========================================
SPI Display + LVGL v9 Demo
========================================
MCU: CC3551E
Display: ST7789 (240x320 pixels)
Renderer: LVGL v9
Status: Initializing...
========================================
LVGL ready. Running UI loop.
Frames: 100 | Last 100 frames: 812 ms | ~8 ms/frame
Frames: 200 | Last 100 frames: 809 ms | ~8 ms/frame
```

Timing is measured using LVGL's internal tick counter (`lv_tick_get()`), which is driven by the dedicated tick task and is always reliable. Each log line reports how long the last 100 `lv_timer_handler` calls took in total, and the average milliseconds per frame. The on-screen label shows the same value.

---

## Using SquareLine Studio (GUI Editor)

[SquareLine Studio](https://squareline.io) is the official visual UI editor for LVGL.
It lets you design screens with drag-and-drop and exports standard LVGL C code.

### Setting up a SquareLine project for this board

1. **Download and install** SquareLine Studio from https://squareline.io (free tier available).
2. **Create a new project:**
   - Board: *Generic* → *Arduino* (or any generic C project)
   - LVGL version: **v9.x**
   - Display resolution: `240 × 320` (ST7789) or `128 × 128` (ST7735)
   - Color depth: **16-bit (RGB565)**
3. **Design your UI** using the drag-and-drop editor.
4. **Export** — File → **Export → Export UI Files**
   - Set the export path to `SPI_Display/ui/`
   - SquareLine generates `ui.c`, `ui.h`, and component files under `ui/`

### Integrating SquareLine output

1. Add the exported `ui/` directory as a source location in CCS (same way as LVGL sources above).
2. In `spi_display.c`, replace the `lvgl_demo_create()` call with `ui_init()`:

```c
/* spi_display.c — inside displayThread(), LVGL path */
#ifdef USE_LVGL
    lv_init();
    lvgl_port_init();

    /* Replace this: */
    // lvgl_demo_create();

    /* With the SquareLine-generated init: */
    #include "ui/ui.h"
    ui_init();
    ...
#endif
```

3. Rebuild and flash. Your SquareLine UI will appear on the display.

> **Note:** SquareLine exports assume `lv_disp_get_scr_act()` in v8. For v9, the
> exported code calls `lv_screen_active()` automatically when you select LVGL v9
> as the target version during project setup.

---

## Selecting Display Controller

Both drivers are always compiled into the project. Select which one is active in `display_config.h`:

### ST7789 (240×320) — default
```c
/* --- Display controller (pick one) --- */
#define USE_ST7789
//#define USE_ST7735
```

### ST7735 (128×128)
```c
/* --- Display controller (pick one) --- */
//#define USE_ST7789
#define USE_ST7735
```

## Disabling LVGL

To use the built-in color-cycling demo instead of LVGL, comment out `USE_LVGL` in `display_config.h`:

```c
/* --- Optional: LVGL rendering engine --- */
//#define USE_LVGL
```

Save and rebuild.

---

## Project Overview

This project demonstrates SPI display control on the CC35xxE wireless microcontroller family.

### File Structure

```
SPI_Display/
├── SPI_Display.projectspec  CCS project definition (import this to set up the project)
├── spi_display.c            Main application (color demo + LVGL demo)
├── spi_display.syscfg       TI Drivers hardware configuration
├── main_freertos.c          FreeRTOS bootstrap
├── display_config.h         Display and renderer selection
├── st7789_lcd.c/h           ST7789 driver (240×320)
├── st7735_lcd.c/h           ST7735 driver (128×128)
├── lv_conf.h                LVGL v9 configuration
├── lvgl_port.c/h            LVGL display port (flush callback + tick task)
├── lvgl/                    LVGL v9 git submodule (populated via git submodule update)
│   └── ...
├── ui/                      SquareLine Studio export (add after exporting)
│   ├── ui.c
│   ├── ui.h
│   └── components/
├── README.md                This file
└── Debug/                   Build output
```

### Architecture

```
Application
   │
   ├── [USE_LVGL set — default]  lv_timer_handler()
   │                                 │
   │                                 └── lvgl_flush_cb()  → LCD_drawRegion()
   │                                           │
   │                                     st7789_lcd.c / st7735_lcd.c
   │                                           │
   │                                       SPI (TI Drivers)
   │                                           │
   │                                       ST7789 / ST7735
   │
   └── [USE_LVGL not set]        draw_color_bar_pattern()  → LCD_fillScreen()
```

**Threads:**
- `mainThread()` — initializes drivers; creates display thread and (if LVGL) tick thread
- `displayThread()` — animation loop or LVGL task pump

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| No display configured | Check `display_config.h` — must define `USE_ST7789` or `USE_ST7735` |
| Black screen after flashing | Check GPIO wiring; verify `display_config.h` matches connected display |
| Build fails with LVGL errors | Ensure `lvgl/` submodule is cloned; check include paths and `LV_CONF_INCLUDE_SIMPLE` define |
| `lv_mem: out of memory` assert | Increase `LV_MEM_SIZE` in `lv_conf.h` |
| Display shows nothing (LVGL) | Verify `lvgl_port_init()` called after `lv_init()`; check tick task is running |
| Cannot connect to target | Verify USB cable; try reconnecting LaunchPad |
| No UART output | Set terminal to 115200 baud (8N1) |
| Missing headers after clean | Rebuild with Project → Clean, then Build |

---

## Implementation Details

### SPI Communication

| Parameter | Value |
|-----------|-------|
| Interface | SPI0 |
| Clock rate | 40 MHz (ST7789) / 15 MHz (ST7735) |
| Mode | POL0_PHA0 (SPI Mode 0) |
| Data width | 8-bit |
| CS | Software (GPIO13, active low) |
| Color format | RGB565 (2 bytes per pixel, big-endian on SPI) |

### GPIO Control

| Signal | GPIO | Direction | Active Level |
|--------|------|-----------|--------------|
| CS     | GPIO13 (BP.13) | Output | Low |
| DC/RS  | GPIO34 (BP.31) | Output | High = data, Low = command |
| RST    | GPIO30 (BP.17) | Output | Low = reset |

### LVGL Port Details

- **Draw buffer:** 10 rows × LCD_WIDTH pixels (partial rendering, ~5 KB)
- **Color format:** `LV_COLOR_FORMAT_RGB565`
- **Flush mode:** Synchronous (no DMA); `lv_display_flush_ready()` called immediately
- **Tick source:** Manual `lv_tick_inc(1)` from a dedicated 1 ms FreeRTOS thread
- **Task pump:** `lv_timer_handler()` called every 5 ms from `displayThread()`

---

## Code Customization

### Change Frame Rate (built-in demo)

In `spi_display.c`:
```c
uint32_t target_frame_ms = 40;  /* 40 = 24 FPS | 33 = 30 FPS | 50 = 20 FPS */
```

### Change Color Palette (built-in demo)

```c
static const uint16_t color_palette[] = {
    0xF800,  /* RED */
    0xF840,  /* YELLOW */
    0x07E0,  /* GREEN */
    0x07FF,  /* CYAN */
    0x001F,  /* BLUE */
    0xF81F   /* MAGENTA */
};
```

Common RGB565 values: `0x0000` BLACK · `0xFFFF` WHITE · `0xF800` RED · `0x07E0` GREEN · `0x001F` BLUE

### Create Custom LVGL UI

Replace `lvgl_demo_create()` in `spi_display.c` with your own LVGL widget code,
or use SquareLine Studio to design and export the UI (see above).

---

## License

This project is provided as-is for educational and commercial use.
