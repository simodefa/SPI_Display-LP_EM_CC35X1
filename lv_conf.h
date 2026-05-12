/**
 * lv_conf.h
 * LVGL v9 configuration for CC3551E / SPI_Display project.
 *
 * Place this file at the project root (same level as the lvgl/ submodule).
 * Build system must define LV_CONF_INCLUDE_SIMPLE so that lvgl.h resolves
 * this file with  #include "lv_conf.h"  rather than a relative path.
 */

#if 1 /* Set to 1 to enable LVGL configuration (do not remove this guard) */

#ifndef LV_CONF_H
#define LV_CONF_H

/*
 * Tell LVGL to skip its Kconfig discovery chain (lv_conf_kconfig.h).
 * Without this, lv_conf_internal.h tries to include lv_conf_kconfig.h
 * from the submodule's src/ directory, which can interfere with our
 * lv_conf.h being recognised as the active configuration.
 */
#define LV_KCONFIG_IGNORE


#define LV_USE_SYSMON 1
#define LV_USE_PERF_MONITOR 1

/*=====================
* BUILD OPTIONS
*======================*/

/** Enable examples to be built with the library. */
#define LV_BUILD_EXAMPLES 1

/** Build the demos */
#define LV_BUILD_DEMOS 1

/*===================
 * DEMO USAGE
 ====================*/

 #if LV_BUILD_DEMOS
    /** Show some widgets. This might be required to increase `LV_MEM_SIZE`. */
    #define LV_USE_DEMO_WIDGETS 1

    /** Benchmark your system */
    #define LV_USE_DEMO_BENCHMARK 1
#endif

/*====================
 * COLOR SETTINGS
 *====================*/

/* Color depth matching ST7789/ST7735 RGB565 format */
#define LV_COLOR_DEPTH 16

/* Swap the bytes of RGB565 color: needed because SPI sends MSB first
 * and the display expects big-endian but LVGL produces little-endian
 * 16-bit words on most architectures. */
#define LV_COLOR_16_SWAP 1

/*====================
 * MEMORY SETTINGS
 *====================*/

/* LVGL internal heap size (bytes).
 * CC3551E has 256 KB SRAM. Allocate 48 KB for LVGL.
 * Increase if you get "lv_mem: couldn't allocate memory" assertions. */
#define LV_MEM_SIZE (128 * 1024U)

/* Use built-in memory allocator */
#define LV_MEM_CUSTOM 0

/*====================
 * HAL SETTINGS
 *====================*/

/* Default display refresh period in ms (how often lv_timer_handler is called) */
#define LV_DEF_REFR_PERIOD 33  /* ~30 Hz refresh */

/* Dot-per-inch of the display for DPI-aware sizing */
#define LV_DPI_DEF 114

/*====================
 * DRAW SETTINGS
 *====================*/

/* Draw buffer strategy: LVGL renders into a partial buffer, then flushes.
 * The buffer size is defined in lvgl_port.c (LV_DRAW_BUF_LINES). */
#define LV_DRAW_SW_SHADOW_CACHE_SIZE 0

/* Enable SW rendering */
#define LV_USE_DRAW_SW 1

/*====================
 * LOGGING
 *====================*/

/* Disable logging in production to save code/RAM */
#define LV_USE_LOG 1

#if LV_USE_LOG
    /** Set value to one of the following levels of logging detail:
     *  - LV_LOG_LEVEL_TRACE    Log detailed information.
     *  - LV_LOG_LEVEL_INFO     Log important events.
     *  - LV_LOG_LEVEL_WARN     Log if something unwanted happened but didn't cause a problem.
     *  - LV_LOG_LEVEL_ERROR    Log only critical issues, when system may fail.
     *  - LV_LOG_LEVEL_USER     Log only custom log messages added by the user.
     *  - LV_LOG_LEVEL_NONE     Do not log anything. */
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

    /** - 1: Print log with 'printf';
     *  - 0: User needs to register a callback with `lv_log_register_print_cb()`. */
    #define LV_LOG_PRINTF 0

    /** Set callback to print logs.
     *  E.g `my_print`. The prototype should be `void my_print(lv_log_level_t level, const char * buf)`.
     *  Can be overwritten by `lv_log_register_print_cb`. */
    //#define LV_LOG_PRINT_CB

    /** - 1: Enable printing timestamp;
     *  - 0: Disable printing timestamp. */
    #define LV_LOG_USE_TIMESTAMP 1

    /** - 1: Print file and line number of the log;
     *  - 0: Do not print file and line number of the log. */
    #define LV_LOG_USE_FILE_LINE 1

    /* Enable/disable LV_LOG_TRACE in modules that produces a huge number of logs. */
    #define LV_LOG_TRACE_MEM        1   /**< Enable/disable trace logs in memory operations. */
    #define LV_LOG_TRACE_TIMER      1   /**< Enable/disable trace logs in timer operations. */
    #define LV_LOG_TRACE_INDEV      1   /**< Enable/disable trace logs in input device operations. */
    #define LV_LOG_TRACE_DISP_REFR  1   /**< Enable/disable trace logs in display re-draw operations. */
    #define LV_LOG_TRACE_EVENT      1   /**< Enable/disable trace logs in event dispatch logic. */
    #define LV_LOG_TRACE_OBJ_CREATE 1   /**< Enable/disable trace logs in object creation (core `obj` creation plus every widget). */
    #define LV_LOG_TRACE_LAYOUT     1   /**< Enable/disable trace logs in flex- and grid-layout operations. */
    #define LV_LOG_TRACE_ANIM       1   /**< Enable/disable trace logs in animation logic. */
    #define LV_LOG_TRACE_CACHE      1   /**< Enable/disable trace logs in cache operations. */
#endif  /*LV_USE_LOG*/

/*====================
 * ASSERTS
 *====================*/

/* Enable asserts during development; disable for release builds */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* Custom assert handler — map to while(1) halt for embedded debug */
#define LV_ASSERT_HANDLER_INCLUDE <stdint.h>
#define LV_ASSERT_HANDLER while(1);

/*====================
 * FONTS
 *====================*/

/* Built-in fonts (Montserrat subset bitmaps, included in lvgl/src/font/).
 * Enable only what you need to keep flash usage low. */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Default font — used by widgets unless overridden in a style */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Enable built-in symbols (material design icons subset) */
#define LV_FONT_MONTSERRAT_12_SUBPX 0
#define LV_USE_FONT_PLACEHOLDER 1

/*====================
 * WIDGETS
 *====================*/

/* Core widgets (enable only what you use to minimize flash) */
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CANVAS       1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMG          1
#define LV_USE_LABEL        1
#define LV_USE_LINE         1
#define LV_USE_ROLLER       1
#define LV_USE_SLIDER       1
#define LV_USE_SWITCH       1
#define LV_USE_TEXTAREA     1
#define LV_USE_TABLE        1

/*====================
 * THEMES
 *====================*/

/* Default theme */
#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   0    /* 0 = light, 1 = dark */
#define LV_THEME_DEFAULT_GROW   1

#define LV_USE_THEME_SIMPLE     0
#define LV_USE_THEME_MONO       0

/*====================
 * LAYOUTS
 *====================*/

#define LV_USE_FLEX  1
#define LV_USE_GRID  1

/*====================
 * EXTRA COMPONENTS
 *====================*/

#define LV_USE_ANIMIMG      1
#define LV_USE_CALENDAR     1
#define LV_USE_CHART        1
#define LV_USE_COLORWHEEL   1
#define LV_USE_IMGBTN       1
#define LV_USE_KEYBOARD     1
#define LV_USE_LED          1
#define LV_USE_LIST         1
#define LV_USE_MENU         1
#define LV_USE_METER        1
#define LV_USE_MSGBOX       1
#define LV_USE_SPAN         1
#define LV_USE_SPINBOX      1
#define LV_USE_SPINNER      1
#define LV_USE_TABVIEW      1
#define LV_USE_TILEVIEW     1
#define LV_USE_WIN          1

/*====================
 * SquareLine Studio
 *====================*/

/* SquareLine Studio exports use lv_i18n for translations.
 * Enable if your SquareLine project uses multi-language support. */
#define LV_USE_I18N 0

/*====================
 * MISC
 *====================*/

/* Garbage collector: not needed on bare-metal */
#define LV_ENABLE_GC 0

/* Enable sprintf-based number formatting in labels */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/* Image caching: cache decoded images in heap.
 * Set to 0 to disable and save RAM. */
#define LV_IMG_CACHE_DEF_SIZE 0

/* Compiler attribute for large constant arrays in flash */
#define LV_ATTRIBUTE_LARGE_CONST __attribute__((section(".rodata")))

/* Place draw buffers in specific RAM section if needed.
 * Leave empty to use default heap. */
#define LV_ATTRIBUTE_MEM_FAST


/*==================
 * OTHERS
 *==================*/

/** 1: Enable system monitor component */
#define LV_USE_SYSMON 1
#if LV_USE_SYSMON
    /** Get the idle percentage. E.g. uint32_t my_get_idle(void); */
    // #define LV_SYSMON_GET_IDLE lv_os_get_idle_percent

    /** 1: Show CPU usage and FPS count.
     *  - Requires `LV_USE_SYSMON = 1` */
    #define LV_USE_PERF_MONITOR 1
    #if LV_USE_PERF_MONITOR
        #define LV_USE_PERF_MONITOR_POS LV_ALIGN_BOTTOM_RIGHT

        /** 0: Displays performance data on the screen; 1: Prints performance data using log. */
        #define LV_USE_PERF_MONITOR_LOG_MODE 0
    #endif
#endif /*LV_USE_SYSMON*/


#endif /* LV_CONF_H */
#endif /* lv_conf.h end */
