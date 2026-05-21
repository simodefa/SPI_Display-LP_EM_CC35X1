/*
 * ILI9341 LCD driver for 240x320 TFT display
 * Uses TI Drivers SPI and GPIO on LP-EM-CC35X1.
 */

#include "display_config.h"

#ifdef USE_ILI9341

#include "ili9341_lcd.h"

#include <stdint.h>
#include <string.h>
#include <unistd.h>

/* TI Drivers */
#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"

/* -----------------------------------------------------------------------
 * ILI9341 command definitions (ILI9341 datasheet opcodes)
 * ----------------------------------------------------------------------- */
#define ILI9341_CMD_RST          0x01   /* Software reset */
#define ILI9341_CMD_SLEEPOUT     0x11   /* Sleep out */
#define ILI9341_CMD_GAMMA        0x26   /* Gamma set */
#define ILI9341_CMD_DISPLAYOFF   0x28   /* Display off */
#define ILI9341_CMD_DISPLAYON    0x29   /* Display on */
#define ILI9341_CMD_COLADDR      0x2A   /* Column address set */
#define ILI9341_CMD_PAGEADDR     0x2B   /* Page address set */
#define ILI9341_CMD_GRAM         0x2C   /* Memory write */
#define ILI9341_CMD_MAC          0x36   /* Memory access control */
#define ILI9341_CMD_PIXELFORMAT  0x3A   /* Interface pixel format */
#define ILI9341_CMD_WDB          0x51   /* Write display brightness */
#define ILI9341_CMD_WCD          0x53   /* Write CTRL display */
#define ILI9341_CMD_RGBINTERFACE 0xB0   /* RGB interface signal control */
#define ILI9341_CMD_FRC          0xB1   /* Frame rate control (normal mode) */
#define ILI9341_CMD_BPC          0xB5   /* Blanking porch control */
#define ILI9341_CMD_DFC          0xB6   /* Display function control */
#define ILI9341_CMD_PWR1         0xC0   /* Power control 1 */
#define ILI9341_CMD_PWR2         0xC1   /* Power control 2 */
#define ILI9341_CMD_VCOM1        0xC5   /* VCOM control 1 */
#define ILI9341_CMD_VCOM2        0xC7   /* VCOM control 2 */
#define ILI9341_CMD_PWRA         0xCB   /* Power control A */
#define ILI9341_CMD_PWRB         0xCF   /* Power control B */
#define ILI9341_CMD_PGAMMA       0xE0   /* Positive gamma correction */
#define ILI9341_CMD_NGAMMA       0xE1   /* Negative gamma correction */
#define ILI9341_CMD_DTCA         0xE8   /* Driver timing control A */
#define ILI9341_CMD_DTCB         0xEA   /* Driver timing control B */
#define ILI9341_CMD_PWRSEQ       0xED   /* Power on sequence control */
#define ILI9341_CMD_3GAMMAEN     0xF2   /* Enable 3-gamma */
#define ILI9341_CMD_INTERFACE    0xF6   /* Interface control */
#define ILI9341_CMD_PRC          0xF7   /* Pump ratio control */
#define ILI9341_CMD_INVON        0x21   /* Display inversion on */
#define ILI9341_CMD_INVOFF       0x20   /* Display inversion off */

/* MAC (0x36) bits */
#define MAC_MY   0x80   /* Row address order */
#define MAC_MX   0x40   /* Column address order */
#define MAC_MV   0x20   /* Row/column exchange */
#define MAC_ML   0x10   /* Vertical refresh order */
#define MAC_BGR  0x08   /* BGR panel order */
#define MAC_MH   0x04   /* Horizontal refresh order */

/* -----------------------------------------------------------------------
 * Module-private state
 * ----------------------------------------------------------------------- */
static SPI_Handle  gSpiHandle = NULL;

/* -----------------------------------------------------------------------
 * Low-level helpers
 * ----------------------------------------------------------------------- */
static void LCD_dc_lo(void)  { GPIO_write(CONFIG_GPIO_LCD_DC,  0); } /* command */
static void LCD_dc_hi(void)  { GPIO_write(CONFIG_GPIO_LCD_DC,  1); } /* data    */
static void LCD_rst_lo(void) { GPIO_write(CONFIG_GPIO_LCD_RST, 0); }
static void LCD_rst_hi(void) { GPIO_write(CONFIG_GPIO_LCD_RST, 1); }

static void trace_hi(void) { GPIO_write(CONFIG_GPIO_TRACE, 1); }
static void trace_lo(void) { GPIO_write(CONFIG_GPIO_TRACE, 0); }

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
    LCD_spiWrite(&cmd, 1);
}

static void LCD_writeData(const uint8_t *data, size_t len)
{
    LCD_dc_hi();
    LCD_spiWrite(data, len);
}

static void LCD_writeDataByte(uint8_t b)
{
    LCD_writeData(&b, 1);
}

/* -----------------------------------------------------------------------
 * Initialisation sequence for ILI9341 (240x320)
 * Based on the ILI9341 datasheet and standard reference implementation.
 * ----------------------------------------------------------------------- */
void LCD_init(void)
{
    SPI_Params spiParams;
    SPI_Params_init(&spiParams);
    spiParams.bitRate     = 80000000;
    spiParams.frameFormat = SPI_POL1_PHA1;
    spiParams.mode        = SPI_CONTROLLER;
    spiParams.dataSize    = 8;
    gSpiHandle = SPI_open(CONFIG_SPI_LCD, &spiParams);
    /* SPI_open() returns NULL if the peripheral is unavailable.
     * Trap here rather than silently hard-faulting on the first SPI transfer. */
    if (gSpiHandle == NULL) {
        while (1) {}   /* attach debugger or check SPI_init()/SysConfig */
    }

    /* Hardware reset */
    LCD_rst_hi();
    usleep(5000);
    LCD_rst_lo();
    usleep(20000);
    LCD_rst_hi();
    usleep(150000);

    /* Software reset */
    LCD_writeCmd(ILI9341_CMD_RST);
    usleep(150000);

    /* Display off */
    LCD_writeCmd(ILI9341_CMD_DISPLAYOFF);
    usleep(10000);

    /* Power control A: Vcore=1.6V, DDVDH=5.6V */
    LCD_writeCmd(ILI9341_CMD_PWRA);
    {
        const uint8_t d[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
        LCD_writeData(d, sizeof(d));
    }

    /* Power control B */
    LCD_writeCmd(ILI9341_CMD_PWRB);
    {
        const uint8_t d[] = {0x00U, 0x83U, 0x30U};
        LCD_writeData(d, sizeof(d));
    }

    /* Driver timing control A */
    LCD_writeCmd(ILI9341_CMD_DTCA);
    {
        const uint8_t d[] = {0x85, 0x01U, 0x79U};
        LCD_writeData(d, sizeof(d));
    }

    /* Driver timing control B */
    LCD_writeCmd(ILI9341_CMD_DTCB);
    {
        const uint8_t d[] = {0x00, 0x00};
        LCD_writeData(d, sizeof(d));
    }

    /* Power on sequence control */
    LCD_writeCmd(ILI9341_CMD_PWRSEQ);
    {
        const uint8_t d[] = {0x64, 0x03, 0x12, 0x81};
        LCD_writeData(d, sizeof(d));
    }

    /* Pump ratio control: DDVDH=2xVCI */
    LCD_writeCmd(ILI9341_CMD_PRC);
    LCD_writeDataByte(0x20);

    /* Power control 1: VRH=4.60V */
    LCD_writeCmd(ILI9341_CMD_PWR1);
    LCD_writeDataByte(0x26U);

    /* Power control 2: SAP, BT3+BT1 */
    LCD_writeCmd(ILI9341_CMD_PWR2);
    LCD_writeDataByte(0x11U);

    /* VCOM control 1: VMH=3.775V, VML=-1.425V */
    LCD_writeCmd(ILI9341_CMD_VCOM1);
    {
        const uint8_t d[] = {0x35U, 0x3eU};
        LCD_writeData(d, sizeof(d));
    }

    /* VCOM control 2: VCOM offset = VMH-58 */
    LCD_writeCmd(ILI9341_CMD_VCOM2);
    LCD_writeDataByte(0xBEU);

    /* Memory access control: portrait orientation, BGR panel order */
    LCD_writeCmd(ILI9341_CMD_MAC);
    LCD_writeDataByte(MAC_MV | MAC_BGR);

    /* Pixel format: 16-bit RGB565 for both MCU and RGB interfaces (0x55) */
    LCD_writeCmd(ILI9341_CMD_PIXELFORMAT);
    LCD_writeDataByte(0x55);

    /* Frame rate control: fosc, 70 Hz */
    LCD_writeCmd(ILI9341_CMD_FRC);
    {
        const uint8_t d[] = {0x00, 0x1FU};
        LCD_writeData(d, sizeof(d));
    }

    /* Display function control */
    LCD_writeCmd(ILI9341_CMD_DFC);
    {
        const uint8_t d[] = {0x0aU, 0x82U, 0x27, 0x00};
        LCD_writeData(d, sizeof(d));
    }

    /* 3-gamma function: disabled */
    LCD_writeCmd(ILI9341_CMD_3GAMMAEN);
    LCD_writeDataByte(0x00);

    LCD_writeCmd(ILI9341_CMD_COLADDR);
    {
        const uint8_t d[] = {0x00U, 0x00U, 0x00U, 0xEFU};
        LCD_writeData(d, sizeof(d));
    }

    LCD_writeCmd(ILI9341_CMD_PAGEADDR);
    {
        const uint8_t d[] = {0x00U, 0x00U, 0x01U, 0x3FU};
        LCD_writeData(d, sizeof(d));
    }

    /* Gamma set: gamma curve 1 */
    LCD_writeCmd(ILI9341_CMD_GAMMA);
    LCD_writeDataByte(0x01);

    /* Positive gamma correction (15 bytes) */
    LCD_writeCmd(ILI9341_CMD_PGAMMA);
    {
        const uint8_t gpos[] = {
            0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08,
            0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00
        };
        LCD_writeData(gpos, sizeof(gpos));
    }

    /* Negative gamma correction (15 bytes) */
    LCD_writeCmd(ILI9341_CMD_NGAMMA);
    {
        const uint8_t gneg[] = {
            0x00, 0x0E, 0x14, 0x03, 0x11, 0x07,
            0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
        };
        LCD_writeData(gneg, sizeof(gneg));
    }

    /* Sleep out - datasheet requires min 5 ms before next command */
    LCD_writeCmd(ILI9341_CMD_SLEEPOUT);
    usleep(120000);

    /* Display on */
    LCD_writeCmd(ILI9341_CMD_DISPLAYON);
    usleep(10000);
}

/* -----------------------------------------------------------------------
 * Set address window for subsequent GRAM write
 * ----------------------------------------------------------------------- */
static void LCD_setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    uint8_t data[4];

    /*Column addresses*/
    data[0] = (x0 >> 8) & 0xFF;
    data[1] = x0 & 0xFF;
    data[2] = (x1 >> 8) & 0xFF;
    data[3] = x1 & 0xFF;

    LCD_writeCmd(ILI9341_CMD_COLADDR);
    LCD_writeData(data, 4);

    /*Page addresses*/
    data[0] = (y0 >> 8) & 0xFF;
    data[1] = y0 & 0xFF;
    data[2] = (y1 >> 8) & 0xFF;
    data[3] = y1 & 0xFF;

    LCD_writeCmd(ILI9341_CMD_PAGEADDR);
    LCD_writeData(data, 4);

    LCD_writeCmd(ILI9341_CMD_GRAM);
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
    for (int r = 0; r < h; r++) {
        LCD_spiWrite(row, (size_t)(w * 2));
    }
}

/* -----------------------------------------------------------------------
 * Blit an arbitrary rectangular region (LVGL flush target).
 * Coordinates are full uint16_t — no truncation, no centering offset.
 * pixels must contain (x1-x0+1)*(y1-y0+1) RGB565 values in row-major order.
 * ----------------------------------------------------------------------- */
void LCD_drawRegion(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                    const uint16_t *pixels)
{
    // trace_hi();
    LCD_setAddrWindow(x0, y0, x1, y1);
    // trace_lo();

    int total = (int)(x1 - x0 + 1) * (int)(y1 - y0 + 1);

    trace_hi();

    LCD_dc_hi();
    LCD_spiWrite((uint8_t *)pixels, (size_t)(total * 2));

    trace_lo();
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
    for (int i = 0; i < total; i += CHUNK_PIXELS) {
        int n = total - i;
        if (n > CHUNK_PIXELS) n = CHUNK_PIXELS;
        for (int j = 0; j < n; j++) {
            buf[j * 2]     = (uint8_t)(frame[i + j] >> 8);
            buf[j * 2 + 1] = (uint8_t)(frame[i + j] & 0xFF);
        }
        LCD_spiWrite(buf, (size_t)(n * 2));
    }
}

/* -----------------------------------------------------------------------
 * Blit a full 240x320 RGB565 image
 * ----------------------------------------------------------------------- */
void LCD_drawImage(const uint16_t *image)
{
    LCD_setAddrWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

#define CHUNK_PIXELS 64
    uint8_t buf[CHUNK_PIXELS * 2];
    const uint16_t *src = image;
    int total = LCD_WIDTH * LCD_HEIGHT;

    LCD_dc_hi();
    for (int i = 0; i < total; i += CHUNK_PIXELS) {
        int n = total - i;
        if (n > CHUNK_PIXELS) n = CHUNK_PIXELS;
        for (int j = 0; j < n; j++) {
            buf[j * 2]     = (uint8_t)(src[i + j] >> 8);
            buf[j * 2 + 1] = (uint8_t)(src[i + j] & 0xFF);
        }
        LCD_spiWrite(buf, (size_t)(n * 2));
    }
}

#endif /* USE_ILI9341 */
