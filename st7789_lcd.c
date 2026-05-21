/*
 * ST7789 LCD driver for 240x320 TFT display
 * Uses TI Drivers SPI and GPIO on LP-EM-CC35X1.
 */

#include "display_config.h"

#ifdef USE_ST7789

#include "st7789_lcd.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* TI Drivers */
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"

/* -----------------------------------------------------------------------
 * ST7789 command definitions (ST7789V datasheet opcodes)
 * ----------------------------------------------------------------------- */
#define ST7789_NOP      0x00
#define ST7789_SWRESET  0x01
#define ST7789_SLPOUT   0x11
#define ST7789_NORON    0x13
#define ST7789_INVOFF   0x20
#define ST7789_INVON    0x21   /* Display inversion ON: corrects ST7789 natural inversion */
#define ST7789_DISPOFF  0x28
#define ST7789_DISPON   0x29
#define ST7789_CASET    0x2A   /* Column address set */
#define ST7789_RASET    0x2B   /* Row address set */
#define ST7789_RAMWR    0x2C   /* Memory write */
#define ST7789_MADCTL   0x36   /* Memory data access control */
#define ST7789_COLMOD   0x3A   /* Interface pixel format */
#define ST7789_PORCTRL  0xB2   /* Porch setting */
#define ST7789_GCTRL    0xB7   /* Gate control */
#define ST7789_VCOMS    0xBB   /* VCOMS setting */
#define ST7789_LCMCTRL  0xC0   /* LCM control */
#define ST7789_VDVVRHEN 0xC2   /* VDV and VRH command enable */
#define ST7789_VRHS     0xC3   /* VRH set */
#define ST7789_VDVS     0xC4   /* VDV set */
#define ST7789_FRCTRL2  0xC6   /* Frame rate control in normal mode */
#define ST7789_PWCTRL1  0xD0   /* Power control 1 */
#define ST7789_GMCTRP1  0xE0   /* Positive gamma correction */
#define ST7789_GMCTRN1  0xE1   /* Negative gamma correction */

/* MADCTL bits */
#define MADCTL_MY   0x80
#define MADCTL_MX   0x40
#define MADCTL_MV   0x20
#define MADCTL_ML   0x10
#define MADCTL_BGR  0x08
#define MADCTL_MH   0x04

/* -----------------------------------------------------------------------
 * Module-private state
 * ----------------------------------------------------------------------- */
static SPI_Handle  gSpiHandle = NULL;

/* -----------------------------------------------------------------------
 * Low-level helpers
 * ----------------------------------------------------------------------- */
static void LCD_cs_lo(void)  { GPIO_write(CONFIG_GPIO_LCD_CS,  0); }
static void LCD_cs_hi(void)  { GPIO_write(CONFIG_GPIO_LCD_CS,  1); }
static void LCD_dc_lo(void)  { GPIO_write(CONFIG_GPIO_LCD_DC,  0); } /* command */
static void LCD_dc_hi(void)  { GPIO_write(CONFIG_GPIO_LCD_DC,  1); } /* data    */
static void LCD_rst_lo(void) { GPIO_write(CONFIG_GPIO_LCD_RST, 0); }
static void LCD_rst_hi(void) { GPIO_write(CONFIG_GPIO_LCD_RST, 1); }

static void LCD_spiWrite(const uint8_t *buf, size_t len)
{
    SPI_Transaction xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.count   = len;
    xfer.txBuf   = (void *)buf;
    xfer.rxBuf   = NULL;
    SPI_transfer(gSpiHandle, &xfer);
}

static void LCD_writeCmd(uint8_t cmd)
{
    LCD_dc_lo();
    LCD_cs_lo();
    LCD_spiWrite(&cmd, 1);
    LCD_cs_hi();
}

static void LCD_writeData(const uint8_t *data, size_t len)
{
    LCD_dc_hi();
    LCD_cs_lo();
    LCD_spiWrite(data, len);
    LCD_cs_hi();
}

static void LCD_writeDataByte(uint8_t b)
{
    LCD_writeData(&b, 1);
}

/* -----------------------------------------------------------------------
 * Initialisation sequence for ST7789V (240x320)
 * Based on the ST7789V datasheet and Waveshare reference implementation.
 * ----------------------------------------------------------------------- */
void LCD_init(void)
{
    /* Open SPI */
    SPI_init();

    SPI_Params spiParams;
    SPI_Params_init(&spiParams);
    spiParams.bitRate     = 40000000;  /* 40 MHz - ST7789 supports up to 62.5 MHz, CC3551E limited to 40 MHz */
    spiParams.frameFormat = SPI_POL0_PHA0;
    spiParams.mode        = SPI_CONTROLLER;
    spiParams.dataSize    = 8;
    gSpiHandle = SPI_open(CONFIG_SPI_LCD, &spiParams);

    /* Hardware reset */
    LCD_rst_hi();
    usleep(5000);
    LCD_rst_lo();
    usleep(20000);
    LCD_rst_hi();
    usleep(150000);

    /* Software reset */
    LCD_writeCmd(ST7789_SWRESET);
    usleep(150000);

    /* Sleep out - datasheet requires min 120 ms before next command */
    LCD_writeCmd(ST7789_SLPOUT);
    usleep(120000);

    /* Pixel format: 16-bit RGB565 for both MCU and RGB interfaces (0x55) */
    LCD_writeCmd(ST7789_COLMOD);
    LCD_writeDataByte(0x55);

    /* Porch control: back/front porch and separate porch settings */
    LCD_writeCmd(ST7789_PORCTRL);
    {
        const uint8_t d[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
        LCD_writeData(d, sizeof(d));
    }

    /* Gate control */
    LCD_writeCmd(ST7789_GCTRL);
    LCD_writeDataByte(0x35);

    /* VCOMS: 0.925 V */
    LCD_writeCmd(ST7789_VCOMS);
    LCD_writeDataByte(0x28);

    /* LCM control */
    LCD_writeCmd(ST7789_LCMCTRL);
    LCD_writeDataByte(0x0C);

    /* VDV and VRH register write enable */
    LCD_writeCmd(ST7789_VDVVRHEN);
    LCD_writeDataByte(0x01);
    LCD_writeDataByte(0xFF);

    /* VRH: 4.45 V + (vcom + vcom offset + 0.5 vdv) */
    LCD_writeCmd(ST7789_VRHS);
    LCD_writeDataByte(0x10);

    /* VDV: 0 V */
    LCD_writeCmd(ST7789_VDVS);
    LCD_writeDataByte(0x20);

    /* Frame rate: 60 Hz in normal mode */
    LCD_writeCmd(ST7789_FRCTRL2);
    LCD_writeDataByte(0x0F);

    /* Power control 1: AVDD 6.8V, AVCL -4.8V, VDDS 2.3V */
    LCD_writeCmd(ST7789_PWCTRL1);
    LCD_writeDataByte(0xA4); LCD_writeDataByte(0xA1);

    /* Memory data access control: BGR panel order, portrait */
    LCD_writeCmd(ST7789_MADCTL);
    LCD_writeDataByte(MADCTL_BGR);

    /* Gamma positive (14 bytes) */
    LCD_writeCmd(ST7789_GMCTRP1);
    {
        const uint8_t gpos[] = {
            0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28,
            0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17
        };
        LCD_writeData(gpos, sizeof(gpos));
    }

    /* Gamma negative (14 bytes) */
    LCD_writeCmd(ST7789_GMCTRN1);
    {
        const uint8_t gneg[] = {
            0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28,
            0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E
        };
        LCD_writeData(gneg, sizeof(gneg));
    }

    /* Display inversion ON: corrects ST7789's natural colour inversion */
    LCD_writeCmd(ST7789_INVON);

    /* Normal display mode */
    LCD_writeCmd(ST7789_NORON);
    usleep(10000);

    /* Display on */
    LCD_writeCmd(ST7789_DISPON);
    usleep(10000);

    /* White background */
    LCD_fillScreen(LCD_COLOR_WHITE);
}

/* -----------------------------------------------------------------------
 * Set address window for subsequent RAMWR
 * ----------------------------------------------------------------------- */
static void LCD_setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    LCD_writeCmd(ST7789_CASET);
    LCD_writeDataByte((uint8_t)(x0 >> 8)); LCD_writeDataByte((uint8_t)(x0 & 0xFF));
    LCD_writeDataByte((uint8_t)(x1 >> 8)); LCD_writeDataByte((uint8_t)(x1 & 0xFF));

    LCD_writeCmd(ST7789_RASET);
    LCD_writeDataByte((uint8_t)(y0 >> 8)); LCD_writeDataByte((uint8_t)(y0 & 0xFF));
    LCD_writeDataByte((uint8_t)(y1 >> 8)); LCD_writeDataByte((uint8_t)(y1 & 0xFF));

    LCD_writeCmd(ST7789_RAMWR);
}

/* -----------------------------------------------------------------------
 * Fill screen with one color
 * ----------------------------------------------------------------------- */
void LCD_fillScreen(uint16_t color)
{
    LCD_fillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

/* -----------------------------------------------------------------------
 * Fill rectangle
 * ----------------------------------------------------------------------- */
void LCD_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if (w <= 0 || h <= 0) return;

    LCD_setAddrWindow((uint16_t)x, (uint16_t)y,
                      (uint16_t)(x + w - 1), (uint16_t)(y + h - 1));

    /* Build a row buffer in big-endian and blast it */
    uint8_t row[LCD_WIDTH * 2];
    for (int i = 0; i < w && i < LCD_WIDTH; i++) {
        row[i * 2]     = (uint8_t)(color >> 8);
        row[i * 2 + 1] = (uint8_t)(color & 0xFF);
    }

    LCD_dc_hi();
    LCD_cs_lo();
    for (int r = 0; r < h; r++) {
        LCD_spiWrite(row, (size_t)(w * 2));
    }
    LCD_cs_hi();
}

/* -----------------------------------------------------------------------
 * Blit an arbitrary rectangular region (LVGL flush target).
 * Coordinates are full uint16_t — no truncation, no centering offset.
 * pixels must contain (x1-x0+1)*(y1-y0+1) RGB565 values in row-major order.
 * ----------------------------------------------------------------------- */
void LCD_drawRegion(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                    const uint16_t *pixels)
{
    LCD_setAddrWindow(x0, y0, x1, y1);

#define REGION_CHUNK 64
    uint8_t buf[REGION_CHUNK * 2];
    int total = (int)(x1 - x0 + 1) * (int)(y1 - y0 + 1);

    LCD_dc_hi();
    LCD_cs_lo();
    for (int i = 0; i < total; i += REGION_CHUNK) {
        int n = total - i;
        if (n > REGION_CHUNK) n = REGION_CHUNK;
        for (int j = 0; j < n; j++) {
            buf[j * 2]     = (uint8_t)(pixels[i + j] >> 8);
            buf[j * 2 + 1] = (uint8_t)(pixels[i + j] & 0xFF);
        }
        LCD_spiWrite(buf, (size_t)(n * 2));
    }
    LCD_cs_hi();
}

/* -----------------------------------------------------------------------
 * Blit a partial-height frame into a horizontal band
 * ----------------------------------------------------------------------- */
void LCD_drawContentFrame(const uint16_t *frame, uint8_t yOffset, uint8_t contentHeight,
                          uint16_t frameWidth)
{
    /* Center the frame horizontally when narrower than the display */
    uint16_t x0 = (LCD_WIDTH > frameWidth) ? (LCD_WIDTH - frameWidth) / 2 : 0;
    uint16_t x1 = x0 + frameWidth - 1;

    LCD_setAddrWindow(x0, yOffset, x1, yOffset + contentHeight - 1);

#define CHUNK_PIXELS 64
    uint8_t buf[CHUNK_PIXELS * 2];
    int total = (int)frameWidth * contentHeight;

    LCD_dc_hi();
    LCD_cs_lo();
    for (int i = 0; i < total; i += CHUNK_PIXELS) {
        int n = total - i;
        if (n > CHUNK_PIXELS) n = CHUNK_PIXELS;
        for (int j = 0; j < n; j++) {
            buf[j * 2]     = (uint8_t)(frame[i + j] >> 8);
            buf[j * 2 + 1] = (uint8_t)(frame[i + j] & 0xFF);
        }
        LCD_spiWrite(buf, (size_t)(n * 2));
    }
    LCD_cs_hi();
}

/* -----------------------------------------------------------------------
 * Blit a full 240x320 RGB565 image
 * ----------------------------------------------------------------------- */
void LCD_drawImage(const uint16_t *image)
{
    LCD_setAddrWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    /* Convert to big-endian in chunks and send */
#define CHUNK_PIXELS 64
    uint8_t buf[CHUNK_PIXELS * 2];
    const uint16_t *src = image;
    int total = LCD_WIDTH * LCD_HEIGHT;

    LCD_dc_hi();
    LCD_cs_lo();
    for (int i = 0; i < total; i += CHUNK_PIXELS) {
        int n = total - i;
        if (n > CHUNK_PIXELS) n = CHUNK_PIXELS;
        for (int j = 0; j < n; j++) {
            buf[j * 2]     = (uint8_t)(src[i + j] >> 8);
            buf[j * 2 + 1] = (uint8_t)(src[i + j] & 0xFF);
        }
        LCD_spiWrite(buf, (size_t)(n * 2));
    }
    LCD_cs_hi();
}

#endif /* USE_ST7789 */
