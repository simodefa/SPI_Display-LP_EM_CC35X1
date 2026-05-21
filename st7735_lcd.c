/*
 * ST7735 LCD driver for 128x128 TFT display (BOOSTXL-EDUMKII)
 * Uses TI Drivers SPI and GPIO on LP-EM-CC35X1.
 */

#include "display_config.h"

#ifdef USE_ST7735

#include "st7735_lcd.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* TI Drivers */
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"

/* -----------------------------------------------------------------------
 * ST7735 command definitions
 * ----------------------------------------------------------------------- */
#define ST7735_NOP      0x00
#define ST7735_SWRESET  0x01
#define ST7735_SLPOUT   0x11
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1

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
 * Initialisation sequence for CFAF128128B-0145T (ST7735S, 1.45" 128x128)
 * ----------------------------------------------------------------------- */
void LCD_init(void)
{
    /* Open SPI */
    SPI_init();

    SPI_Params spiParams;
    SPI_Params_init(&spiParams);
    spiParams.bitRate     = 15000000;  /* 15 MHz - max supported by ST7735 */
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

    /* Software reset + sleep-out */
    LCD_writeCmd(ST7735_SWRESET);
    usleep(150000);
    LCD_writeCmd(ST7735_SLPOUT);
    usleep(500000);

    /* Frame rate control */
    LCD_writeCmd(ST7735_FRMCTR1);
    LCD_writeDataByte(0x01); LCD_writeDataByte(0x2C); LCD_writeDataByte(0x2D);

    LCD_writeCmd(ST7735_FRMCTR2);
    LCD_writeDataByte(0x01); LCD_writeDataByte(0x2C); LCD_writeDataByte(0x2D);

    LCD_writeCmd(ST7735_FRMCTR3);
    LCD_writeDataByte(0x01); LCD_writeDataByte(0x2C); LCD_writeDataByte(0x2D);
    LCD_writeDataByte(0x01); LCD_writeDataByte(0x2C); LCD_writeDataByte(0x2D);

    /* Display inversion */
    LCD_writeCmd(ST7735_INVCTR);
    LCD_writeDataByte(0x07);

    /* Power control */
    LCD_writeCmd(ST7735_PWCTR1);
    LCD_writeDataByte(0xA2); LCD_writeDataByte(0x02); LCD_writeDataByte(0x84);

    LCD_writeCmd(ST7735_PWCTR2);
    LCD_writeDataByte(0xC5);

    LCD_writeCmd(ST7735_PWCTR3);
    LCD_writeDataByte(0x0A); LCD_writeDataByte(0x00);

    LCD_writeCmd(ST7735_PWCTR4);
    LCD_writeDataByte(0x8A); LCD_writeDataByte(0x2A);

    LCD_writeCmd(ST7735_PWCTR5);
    LCD_writeDataByte(0x8A); LCD_writeDataByte(0xEE);

    LCD_writeCmd(ST7735_VMCTR1);
    LCD_writeDataByte(0x0E);

    LCD_writeCmd(ST7735_INVOFF);

    /* Memory access control: RGB order, portrait 128x128 */
    LCD_writeCmd(ST7735_MADCTL);
    LCD_writeDataByte(MADCTL_BGR);

    /* 16-bit color */
    LCD_writeCmd(ST7735_COLMOD);
    LCD_writeDataByte(0x05);

    /* Gamma positive */
    LCD_writeCmd(ST7735_GMCTRP1);
    {
        const uint8_t gpos[] = {
            0x0F,0x1A,0x0F,0x18,0x2F,0x28,0x20,0x22,
            0x1F,0x1B,0x23,0x37,0x00,0x07,0x02,0x10
        };
        LCD_writeData(gpos, sizeof(gpos));
    }

    /* Gamma negative */
    LCD_writeCmd(ST7735_GMCTRN1);
    {
        const uint8_t gneg[] = {
            0x0F,0x1B,0x0F,0x17,0x33,0x2C,0x29,0x2E,
            0x30,0x30,0x39,0x3F,0x00,0x07,0x03,0x10
        };
        LCD_writeData(gneg, sizeof(gneg));
    }

    LCD_writeCmd(ST7735_NORON);
    usleep(10000);

    LCD_writeCmd(ST7735_DISPON);
    usleep(100000);

    /* White background */
    LCD_fillScreen(LCD_COLOR_WHITE);
}

/* -----------------------------------------------------------------------
 * Set address window for subsequent RAMWR
 * ----------------------------------------------------------------------- */
static void LCD_setAddrWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    LCD_writeCmd(ST7735_CASET);
    LCD_writeDataByte(0x00); LCD_writeDataByte(x0 + 2);
    LCD_writeDataByte(0x00); LCD_writeDataByte(x1 + 2);

    LCD_writeCmd(ST7735_RASET);
    LCD_writeDataByte(0x00); LCD_writeDataByte(y0 + 1);
    LCD_writeDataByte(0x00); LCD_writeDataByte(y1 + 1);

    LCD_writeCmd(ST7735_RAMWR);
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

    LCD_setAddrWindow((uint8_t)x, (uint8_t)y,
                      (uint8_t)(x + w - 1), (uint8_t)(y + h - 1));

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
    /* ST7735 uses 8-bit address window with pixel offsets (+2 col, +1 row) */
    LCD_setAddrWindow((uint8_t)(x0), (uint8_t)(y0),
                      (uint8_t)(x1), (uint8_t)(y1));

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
    LCD_setAddrWindow(0, yOffset, frameWidth - 1, yOffset + contentHeight - 1);

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
 * Blit a full 128x128 RGB565 image
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

#endif /* USE_ST7735 */
