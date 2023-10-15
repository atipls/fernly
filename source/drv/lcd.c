#include "bionic.h"
#include "fernvale-clockgate.h"
#include "fernvale-gpio.h"
#include "fernvale-lcd.h"
#include "fernvale-pll.h"
#include "gpio.h"
#include "memio.h"
#include "power.h"
#include "printf.h"
#include <stdint.h>
#include "lcd.h"

static pixel_t *fb = (pixel_t *)0x40000;
static const uint32_t fb_height = 320;
static const uint32_t fb_width = 240;
static const uint32_t fb_bpp = 2;

#define lcd_cmd(_cmd_) writew(_cmd_, LCD_SER0_CMD_PORT_REG)
#define lcd_dat(_dat_) writew(_dat_, LCD_SER0_DAT_PORT_REG)

/* Note these don't flush the DMA, you have to do this yourself
 * explicitly later on.
 *    [slot] is the order to execute.
 */
enum lcd_slot_type {
  SLOT_DATA = 0,
  SLOT_CMD = 0x800000,
};

static void lcd_slot(enum lcd_slot_type type, uint16_t cmd, uint8_t slot) {
  writel(cmd | type, LCD_CMD_LIST_ADDR + (slot * 4));
}

static void lcd_setup_gpio(void) {
  gpio_setup(22, GPMODE(GPIO22_MODE_BPI_BUS2));
  gpio_setup(23, GPMODE(GPIO23_MODE_PI_BUS1));
  gpio_setup(45, GPMODE(GPIO45_MODE_LSRSTB));
  gpio_setup(47, GPMODE(GPIO47_MODE_LSCK0));
  gpio_setup(49, GPMODE(GPIO49_MODE_LSA0DA0));
}

static int lcd_setup(void) {
  lcd_setup_gpio();

  /* Power up the LCD block */

  PCTL_PowerUp(PD_LCD);
  PCTL_PowerUp(PD_SLCD);

  writel(CLKGATE_CTL0_LCD, CLKGATE_SYS_CTL0_CLR);
  writel(CLKGATE_CTL0_SLCD, CLKGATE_SYS_CTL0_CLR);

  uint32_t condh = readl(PLL_CTRL_CLK_CONDH);
  writel((condh & ~LCD_MUX_SEL(-1)) | LCD_MUX_SEL(0), PLL_CTRL_CLK_CONDH);

  _msleep(10);

  writew(0, LCD_INT_ENA_REG);
  writel(LCD_RESET_CLEAR, LCD_RUN_REG);
  writel(LCD_RESET_SET, LCD_RUN_REG);

  writel(LCD_RESET_SET, LCD_RESET_REG);
  _msleep(20);
  writel(LCD_RESET_CLEAR, LCD_RESET_REG);

  writel(LCD_WR_2ND(0) | LCD_WR_1ST(0) | LCD_RD_2ND(0) | LCD_RD_1ST(0) |
             LCD_CSH(0) | LCD_CSS(0),
         LCD_SIF0_TIMING_REG);
  writel(LCD_SIF0_SIZE8B | LCD_SIF0_3WIRE | LCD_SIF0_DIV2 | LCD_SIF0_HW_CS,
         LCD_SIF_CON_REG);
         

  //lcd_cmd(0x04);

  return 0;
}

static void lcd_panel_setup(void) {
  lcd_cmd(0x11); // SleepIn
  _msleep(120);
  lcd_cmd(0x28); // display off
  //------------- display control setting -----------------------//
  lcd_cmd(0xfe);
  lcd_cmd(0xef);
  lcd_cmd(0x36);
  lcd_dat(0x28);
  lcd_cmd(0x3a);
  lcd_dat(0x05);

  lcd_cmd(0x35);
  lcd_dat(0x00);
  lcd_cmd(0x44);
  lcd_dat(0x00);
  lcd_dat(0x60);

  //------Power Control Registers Initial----//
  lcd_cmd(0xa4);
  lcd_dat(0x44);
  lcd_dat(0x44);
  lcd_cmd(0xa5);
  lcd_dat(0x42);
  lcd_dat(0x42);
  lcd_cmd(0xaa);
  lcd_dat(0x88);
  lcd_dat(0x88);
  lcd_cmd(0xe8);
  lcd_dat(0x11);
  lcd_dat(0x71);
  lcd_cmd(0xe3);
  lcd_dat(0x01);
  lcd_dat(0x10);
  lcd_cmd(0xff);
  lcd_dat(0x61);
  lcd_cmd(0xAC);
  lcd_dat(0x00);

  lcd_cmd(0xAe);
  lcd_dat(0x2b); // 20161020

  lcd_cmd(0xAd);
  lcd_dat(0x33);
  lcd_cmd(0xAf);
  lcd_dat(0x55);
  lcd_cmd(0xa6);
  lcd_dat(0x2a);
  lcd_dat(0x2a);
  lcd_cmd(0xa7);
  lcd_dat(0x2b);
  lcd_dat(0x2b);
  lcd_cmd(0xa8);
  lcd_dat(0x18);
  lcd_dat(0x18);
  lcd_cmd(0xa9);
  lcd_dat(0x2a);
  lcd_dat(0x2a);
  //-----display window 240X320---------//
  lcd_cmd(0x2a);
  lcd_dat(0x00);
  lcd_dat(0x00);
  lcd_dat(0x01);
  lcd_dat(0x3f);
  lcd_cmd(0x2b); // 0x002B = 239
  lcd_dat(0x00);
  lcd_dat(0x00);
  lcd_dat(0x00);
  lcd_dat(0xef); // 0x013F = 319
  lcd_cmd(0x20); //  Display Inversion Off (for colors)

  //    lcd_cmd(0x2c);

  //------------gamma setting------------------//
  lcd_cmd(0xf0);
  lcd_dat(0x02);
  lcd_dat(0x01);
  lcd_dat(0x00);
  lcd_dat(0x00);
  lcd_dat(0x02);
  lcd_dat(0x09);

  lcd_cmd(0xf1);
  lcd_dat(0x01);
  lcd_dat(0x02);
  lcd_dat(0x00);
  lcd_dat(0x11);
  lcd_dat(0x1c);
  lcd_dat(0x15);

  lcd_cmd(0xf2);
  lcd_dat(0x0a);
  lcd_dat(0x07);
  lcd_dat(0x29);
  lcd_dat(0x04);
  lcd_dat(0x04);
  lcd_dat(0x38); // v43n  39

  lcd_cmd(0xf3);
  lcd_dat(0x15);
  lcd_dat(0x0d);
  lcd_dat(0x55);
  lcd_dat(0x04);
  lcd_dat(0x03);
  lcd_dat(0x65); // v43p 66

  lcd_cmd(0xf4);
  lcd_dat(0x0f); // v50n
  lcd_dat(0x1d); // v57n
  lcd_dat(0x1e); // v59n
  lcd_dat(0x0a); // v61n 0b
  lcd_dat(0x0d); // v62n 0d
  lcd_dat(0x0f);

  lcd_cmd(0xf5);
  lcd_dat(0x05); // v50p
  lcd_dat(0x12); // v57p
  lcd_dat(0x11); // v59p
  lcd_dat(0x34); // v61p 35
  lcd_dat(0x34); // v62p 34
  lcd_dat(0x0f);
  //-------end gamma setting----//
  lcd_cmd(0x11); // SleepOut
  _msleep(120);
  lcd_cmd(0x29); // Display ON
  lcd_cmd(0x2c); // Display ON
}

/* Fill pre-frame command buffer.  These commands are sent out before
 * pixel data, whenever RUN is enabled.
 */
static void lcd_fill_cmd_buffer(void) {
  int ncommands = 0;

  /* Memory write */
  lcd_slot(SLOT_CMD, 0x2c, ncommands++);

  /* Count the number of commands and add it to AUTOCOPY_CTRL */
  writel((readl(LCD_AUTOCOPY_CTRL_REG) & ~LCD_AUTOCOPY_CTRL_CMD_COUNT_MASK) |
             ((ncommands - 1) << LCD_AUTOCOPY_CTRL_CMD_COUNT_SHIFT),
         LCD_AUTOCOPY_CTRL_REG);
}

static int lcd_dma_setup(void) {

  writel(LCD_AUTOCOPY_CTRL_FORMAT_RGB | LCD_AUTOCOPY_CTRL_FORMAT_PAD_LSB |
             LCD_AUTOCOPY_CTRL_FORMAT_RGB565 |
             LCD_AUTOCOPY_CTRL_FORMAT_IFACE_8BIT,
         LCD_AUTOCOPY_CTRL_REG);

  writel(LCD_WROIOFX(0) | LCD_WROIOFY(0), LCD_AUTOCOPY_OFFSET_REG);
  writel(LCD_CSIF0, LCD_AUTOCOPY_CMD_ADDR_REG);
  writel(LCD_DSIF0, LCD_AUTOCOPY_DATA_ADDR_REG);

  writel(LCD_WROICOL(fb_width) | LCD_WROIROW(fb_height), LCD_AUTOCOPY_SIZE_REG);
  writel(rgb(30, 90, 30), LCD_AUTOCOPY_BG_COLOR_REG);

  return 0;
}

int lcd_init(void) {
  lcd_setup();
  lcd_panel_setup();
  //lcd_dma_setup();
  return 0;
}

int lcd_run(void) {
  writew(0, LCD_RUN_REG);

  /* Must refill the command buffer before sending another frame */
  lcd_fill_cmd_buffer();

  writew(LCD_RUN_BIT, LCD_RUN_REG);
  return 0;
}

int lcd_stop(void) {
  writew(1, LCD_RUN_REG);
  writew(0, LCD_RUN_REG);
  return 0;
}

pixel_t *lcd_fb(void) { return fb; }

uint32_t lcd_width(void) { return fb_width; }

uint32_t lcd_height(void) { return fb_height; }

uint32_t lcd_bpp(void) { return fb_bpp; }

void lcd_addpixel(pixel_t px) {
  lcd_dat(px >> 8);
  lcd_dat(px & 0xff);
}
