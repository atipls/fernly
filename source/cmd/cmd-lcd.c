#include "bionic.h"
#include "fernvale-lcd.h"
#include "lcd.h"
#include "memio.h"
#include "printf.h"
#include "reg_base.h"
#include <string.h>

#define LCD_DEBUG
#ifdef LCD_DEBUG

#define LCD_PARALLEL_CTRL0_REG (LCD_base + 0x0F00)
#define LCD_PARALLEL_DATA0_REG (LCD_base + 0x0F10)
#define LCD_PARALLEL_CTRL1_REG (LCD_base + 0x0F20)
#define LCD_PARALLEL_DATA1_REG (LCD_base + 0x0F30)
#define LCD_PARALLEL_CTRL2_REG (LCD_base + 0x0F40)
#define LCD_PARALLEL_DATA2_REG (LCD_base + 0x0F50)
#define LCD_SERIAL_CTRL0_REG (LCD_base + 0x0F80)
#define LCD_SERIAL_DATA0_REG (LCD_base + 0x0F90)
#define LCD_SERIAL_CTRL1_REG (LCD_base + 0x0FA0)
#define LCD_SERIAL_DATA1_REG (LCD_base + 0x0FB0)
#define LCD_SERIAL_SPE_CTRL0_REG (LCD_base + 0x0F88)
#define LCD_SERIAL_SPE_DATA0_REG (LCD_base + 0x0F98)
#define LCD_SERIAL_SPE_CTRL1_REG (LCD_base + 0x0FA8)
#define LCD_SERIAL_SPE_DATA1_REG (LCD_base + 0x0FB8)


#define LCD_PARALLEL0_A0_LOW_ADDR (LCD_base + 0x0F00)
#define LCD_PARALLEL0_A0_HIGH_ADDR (LCD_base + 0x0F10)
#define LCD_PARALLEL1_A0_LOW_ADDR (LCD_base + 0x0F20)
#define LCD_PARALLEL1_A0_HIGH_ADDR (LCD_base + 0x0F30)
#define LCD_PARALLEL2_A0_LOW_ADDR (LCD_base + 0x0F40)
#define LCD_PARALLEL2_A0_HIGH_ADDR (LCD_base + 0x0F50)
#define LCD_SERIAL0_A0_LOW_ADDR (LCD_base + 0x0F80)
#define LCD_SERIAL0_A0_HIGH_ADDR (LCD_base + 0x0F90)
#define LCD_SERIAL1_A0_LOW_ADDR (LCD_base + 0x0FA0)
#define LCD_SERIAL1_A0_HIGH_ADDR (LCD_base + 0x0FB0)

#define LCD_SERIAL0_A0_SPE_LOW_ADDR (LCD_base + 0x0F88)
#define LCD_SERIAL0_A0_SPE_HIGH_ADDR (LCD_base + 0x0F98)
#define LCD_SERIAL1_A0_SPE_LOW_ADDR (LCD_base + 0x0FA8)
#define LCD_SERIAL1_A0_SPE_HIGH_ADDR (LCD_base + 0x0FB8)

#define LCD_STA_REG (LCD_base + 0x0000)
#define LCD_INT_ENABLE_REG (LCD_base + 0x0004)
#define LCD_INT_STATUS_REG (LCD_base + 0x0008)
#define LCD_START_REG (LCD_base + 0x000C)
#define LCD_RSTB_REG (LCD_base + 0x0010)

#define LCD_SIF_PIX_CON_REG (LCD_base + 0x0018)
#define LCD_SIF0_TIMING_REG (LCD_base + 0x001C)
#define LCD_SIF1_TIMING_REG (LCD_base + 0x0020)
#define LCD_SIF_CON_REG (LCD_base + 0x0028)
#define LCD_SIF_CS_REG (LCD_base + 0x002C)
#define LCD_SIF_PAD_SEL_REG (LCD_base + 0x0300)
#define LCD_SIF_STR_BYTE_CON_REG (LCD_base + 0x0270)
#define LCD_SIF_WR_STR_BYTE_REG (LCD_base + 0x0278)
#define LCD_SIF_RD_STR_BYTE_REG (LCD_base + 0x027C)

#define LCD_PARALLEL0_CONFIG_REG (LCD_base + 0x0030)
#define LCD_PARALLEL1_CONFIG_REG (LCD_base + 0x0034)
#define LCD_PARALLEL2_CONFIG_REG (LCD_base + 0x0038)
#define LCD_PARALLEL_PDW_REG (LCD_base + 0x003C)

//#define LCD_GAMMA_CON_REG           (LCD_base+0x0040)

#define LCD_CALC_HTT_REG (LCD_base + 0x0044)
#define LCD_SYNC_LCM_SIZE_REG (LCD_base + 0x0048)
#define LCD_SYNC_CNT_REG (LCD_base + 0x004C)

#define LCD_TECON_REG (LCD_base + 0x0050)
#define LCD_GMCCON_REG (LCD_base + 0x0054)

#define LCD_PALETTE_ADDR_REG (LCD_base + 0x007C)

#define LCD_ROI_CTRL_REG (LCD_base + 0x0080)
#define LCD_ROI_OFFSET_REG (LCD_base + 0x0084)
#define LCD_ROI_CMD_ADDR_REG (LCD_base + 0x0088)
#define LCD_ROI_DATA_ADDR_REG (LCD_base + 0x008C)
#define LCD_ROI_SIZE_REG (LCD_base + 0x0090)
#define LCD_ROI_BG_COLOR_REG (LCD_base + 0x009C)

#define REG_LCD_PALETTE_ADDR (*((volatile unsigned int *) LCD_PALETTE_ADDR_REG))
#define SET_LUT0_PALETTE_BUFF_ADDRESS(addr) REG_LCD_PALETTE_ADDR = (unsigned int) addr

/// command queue, max 64 entries
#define LCD_CMD_PARAMETER0_ADDR (LCD_base + 0x0C00)

#define LCD_DITHER_CON_REG (LCD_base + 0x0170)

#define LCD_NLI_START_REG (LCD_base + 0x0200)
#define LCD_NLI_CLEAR_REG (LCD_base + 0x0204)


#define LCD_FRAME_COUNT_CON_REG (LCD_base + 0x0220)
#define LCD_FRAME_COUNT_REG (LCD_base + 0x0224)

#define LCD_ULTRA_CON_REG (LCD_base + 0x0240)
#define LCD_CONSUME_RATE_REG (LCD_base + 0x0244)
#define LCD_DBI_ULTRA_TH_REG (LCD_base + 0x0248)
#define LCD_GMC_ULTRA_TH_REG (LCD_base + 0x024C)

/*------------------------------------------------------------------*/

#define REG_LCD_STA *((volatile unsigned short *) (LCD_STA_REG))
#define REG_LCD_INT_ENABLE *((volatile unsigned short *) (LCD_INT_ENABLE_REG))
#define REG_LCD_INT_STATUS *((volatile unsigned short *) (LCD_INT_STATUS_REG))
#define REG_LCD_START *((volatile unsigned short *) (LCD_START_REG))
#define REG_LCD_RSTB *((volatile unsigned short *) (LCD_RSTB_REG))

#define REG_LCD_PARALLEL0_CONFIG *((volatile unsigned int *) (LCD_PARALLEL0_CONFIG_REG))
#define REG_LCD_PARALLEL1_CONFIG *((volatile unsigned int *) (LCD_PARALLEL1_CONFIG_REG))
#define REG_LCD_PARALLEL2_CONFIG *((volatile unsigned int *) (LCD_PARALLEL2_CONFIG_REG))
#define REG_LCD_PARALLEL_PDW *((volatile unsigned int *) (LCD_PARALLEL_PDW_REG))

#define REG_LCD_CALC_HTT *((volatile unsigned int *) (LCD_CALC_HTT_REG))
#define REG_LCD_SYNC_LCM_SIZE *((volatile unsigned int *) (LCD_SYNC_LCM_SIZE_REG))
#define REG_LCD_SYNC_CNT *((volatile unsigned int *) (LCD_SYNC_CNT_REG))
#define REG_LCD_SYNC_COUNT *((volatile unsigned int *) (LCD_SYNC_CNT_REG))
#define REG_LCD_SIF0_TIMING_REG *((volatile unsigned int *) (LCD_SIF0_TIMING_REG))
#define REG_LCD_SIF1_TIMING_REG *((volatile unsigned int *) (LCD_SIF1_TIMING_REG))
#define REG_LCD_SIF_CON_REG *((volatile unsigned int *) (LCD_SIF_CON_REG))
#define REG_LCD_SIF_PIX_CON_REG *((volatile unsigned short *) LCD_SIF_PIX_CON_REG)
#define REG_LCD_SIF_CS_REG *((volatile unsigned int *) (LCD_SIF_CS_REG))
#define REG_LCD_SIF_PAD_SEL_REG *((volatile unsigned int *) (LCD_SIF_PAD_SEL_REG))
#define REG_LCD_SIF_STR_BYTE_CON_REG *((volatile unsigned int *) (LCD_SIF_STR_BYTE_CON_REG))
#define REG_LCD_SIF_WR_STR_BYTE_REG *((volatile unsigned int *) (LCD_SIF_WR_STR_BYTE_REG))
#define REG_LCD_SIF_RD_STR_BYTE_REG *((volatile unsigned int *) (LCD_SIF_RD_STR_BYTE_REG))

#define ENABLE_LCD_SERIAL_IF_HW_CS REG_LCD_SIF_CON_REG |= LCD_SERIAL_CONFIG_SIF_HW_CS_CTRL_BIT
#define DISABLE_LCD_SERIAL_IF_HW_CS REG_LCD_SIF_CON_REG &= ~(LCD_SERIAL_CONFIG_SIF_HW_CS_CTRL_BIT)

#define ENABLE_LCD_SERIAL0_CS_STAY_LOW REG_LCD_SIF_PIX_CON_REG |= LCD_SERIAL_PIX_CONFIG_SIF0_CS_STAY_LOW_BIT
#define DISABLE_LCD_SERIAL0_CS_STAY_LOW REG_LCD_SIF_PIX_CON_REG &= ~(LCD_SERIAL_PIX_CONFIG_SIF0_CS_STAY_LOW_BIT)

#define LCD_SERIAL_CONFIG_SIF_HW_CS_CTRL_BIT 0x01000000
#define LCD_SERIAL_CONFIG_CS0_LEVEL_BIT 0x01

#define ENABLE_LCD_SERIAL0_CS REG_LCD_SIF_CS_REG &= ~(LCD_SERIAL_CONFIG_CS0_LEVEL_BIT)
#define DISABLE_LCD_SERIAL0_CS REG_LCD_SIF_CS_REG |= LCD_SERIAL_CONFIG_CS0_LEVEL_BIT

#define SET_LCD_SERIAL0_IF_SIZE(n)                              \
    REG_LCD_SIF_CON_REG &= ~(LCD_SERIAL_CONFIG_SIF0_SIZE_MASK); \
    REG_LCD_SIF_CON_REG |= ((n & LCD_SERIAL_CONFIG_SIF0_SIZE_MASK) << LCD_SERIAL_CONFIG_SIF0_SIZE_OFFSET)
#define LCD_SERIAL_CONFIG_SIF0_SIZE_MASK 0x0007
#define LCD_SERIAL_CONFIG_SIF0_SIZE_OFFSET 0
typedef enum {
    LCD_SCNF_IF_WIDTH_8 = 0,
    LCD_SCNF_IF_WIDTH_9,
    LCD_SCNF_IF_WIDTH_16,
    LCD_SCNF_IF_WIDTH_18,
    LCD_SCNF_IF_WIDTH_24,
    LCD_SCNF_IF_WIDTH_32
} LCD_SCNF_IF_WIDTH_ENUM;

#define MAIN_LCD_CMD_ADDR LCD_SERIAL0_A0_LOW_ADDR
#define MAIN_LCD_DATA_ADDR LCD_SERIAL0_A0_HIGH_ADDR
#define LCD_IS_RUNNING (REG_LCD_STA & LCD_STATUS_RUN_BIT)

static void lcd_dump(void) {
    /* Dump registers */
    //	printf("LCD_PAR0_CMD_PORT:      %04x\n", readw(LCD_PAR0_CMD_PORT_REG));
    //	printf("LCD_PAR0_DAT_PORT:      %04x\n", readw(LCD_PAR0_DAT_PORT_REG));
    //	printf("LCD_PAR1_CMD_PORT:      %04x\n", readw(LCD_PAR1_CMD_PORT_REG));
    //	printf("LCD_PAR1_DAT_PORT:      %04x\n", readw(LCD_PAR1_DAT_PORT_REG));
    //	printf("LCD_PAR0_CFG:           %08x\n", readl(LCD_PAR0_CFG_REG));
    //	printf("LCD_PAR1_CFG:           %08x\n", readl(LCD_PAR1_CFG_REG));
    printf("LCD_STATUS:             %04x\n", readw(LCD_STATUS_REG));
    printf("LCD_INT_ENA:            %04x\n", readw(LCD_INT_ENA_REG));
    printf("LCD_INT_STAT:           %04x\n", readw(LCD_INT_STAT_REG));
    printf("LCD_RUN:                %04x\n", readw(LCD_RUN_REG));
    printf("LCD_RESET:              %04x\n", readw(LCD_RESET_REG));
    printf("LCD_PAR_DATA_WIDTH:     %08x\n", readl(LCD_PAR_DATA_WIDTH_REG));
    printf("LCD_TEARING:            %08x\n", readl(LCD_TEARING_REG));
    printf("LCD_AUTOCOPY_CTRL:      %08x\n", readl(LCD_AUTOCOPY_CTRL_REG));
    printf("LCD_AUTOCOPY_OFFSET:    %08x\n", readl(LCD_AUTOCOPY_OFFSET_REG));
    printf("LCD_AUTOCOPY_SIZE:      %08x\n", readl(LCD_AUTOCOPY_SIZE_REG));
    printf("LCD_AUTOCOPY_CMD_ADDR:  %04x\n", readw(LCD_AUTOCOPY_CMD_ADDR_REG));
    printf("LCD_AUTOCOPY_DATA_ADDR: %04x\n", readw(LCD_AUTOCOPY_DATA_ADDR_REG));
    printf("LCD_LAYER0_CTRL:        %08x\n", readl(LCD_LAYER0_CTRL_REG));
    printf("LCD_LAYER0_OFFSET:      %08x\n", readl(LCD_LAYER0_OFFSET_REG));
    printf("LCD_LAYER0_SIZE:        %08x\n", readl(LCD_LAYER0_SIZE_REG));
    printf("LCD_LAYER0_SRC_ADDR:    %08x\n", readl(LCD_LAYER0_SRC_ADDR_REG));
    printf("LCD_FRAME_COUNTER_CON:  %08x\n", readl(LCD_FRAME_COUNTER_CON_REG));
    printf("LCD_FRAME_COUNTER:      %08x\n", readl(LCD_FRAME_COUNTER_REG));

    // Read LCD version.
    DISABLE_LCD_SERIAL_IF_HW_CS;
    ENABLE_LCD_SERIAL0_CS;
    SET_LCD_SERIAL0_IF_SIZE(LCD_SCNF_IF_WIDTH_8);


    while (LCD_IS_RUNNING)
        ;

    *(volatile uint8_t *) MAIN_LCD_CMD_ADDR = 0x14;

	uint8_t iDataH1 = *(volatile uint8_t *) MAIN_LCD_DATA_ADDR;
	uint8_t iDataH2 = *(volatile uint8_t *) MAIN_LCD_DATA_ADDR;

    DISABLE_LCD_SERIAL0_CS;
    ENABLE_LCD_SERIAL_IF_HW_CS;

	uint16_t lcd_version = (iDataH1 << 8) | iDataH2;
	printf("LCD version: %04x\n", lcd_version);
}
#endif /* LCD_DEBUG */

static int is_command(int argc, char **argv, const char *cmd) {
    return ((argc > 0) && !_strcasecmp(argv[0], cmd));
}

static pixel_t color_wheel(int step) {
    step &= 255;
    if (step < 85) {
        return rgb(step * 3, 255 - step * 3, 0);
    } else if (step < 170) {
        step -= 85;
        return rgb(255 - step * 3, 0, step * 3);
    } else {
        step -= 170;
        return rgb(0, step * 3, 255 - step * 3);
    }
}

int cmd_lcd(int argc, char **argv) {
    int i;
    int ret;

    if (is_command(argc, argv, "init")) {
        printf("Initializing LCD... ");
        ret = lcd_init();
        if (ret)
            printf("failed: %d\n", ret);
        else
            printf("Ok\n");
    }
#ifdef LCD_DEBUG
    else if (is_command(argc, argv, "dump")) {
        lcd_dump();
    }
#endif
    else if (is_command(argc, argv, "run")) {
        printf("Running LCD... ");
        ret = lcd_run();
        if (ret)
            printf("failed: %d\n", ret);
        else
            printf("Ok\n");
    } else if (is_command(argc, argv, "stop")) {
        printf("Stopping LCD... ");
        ret = lcd_stop();
        if (ret)
            printf("failed: %d\n", ret);
        else
            printf("Ok\n");
    } else if (is_command(argc, argv, "tpp1")) {
        int w = lcd_width();
        int h = lcd_height();
        int total = w * h;

        for (i = 0; i < total; i++)
            lcd_addpixel(i);
    } else if (is_command(argc, argv, "tpp2")) {
        int x, y;

        i = 0;
        for (y = 0; y < lcd_height(); y++)
            for (x = 0; x < lcd_width(); x++)
                lcd_addpixel(rgb(i++, 0, 0));
    } else if (is_command(argc, argv, "tpd")) {
        static int step = 0;
        pixel_t *fb;
        int x, y;
        int w, h;

        fb = lcd_fb();

        h = lcd_height();
        w = lcd_width();

        /* Stupid clear-screen */
        memset(fb, 0, w * h * lcd_bpp());

        printf("Width: %d  Height: %d\n", w, h);

        i = step++;
        fb = lcd_fb();
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++) {
                /* Swap axes, to verify X and Y work */
                if (step & 1)
                    *fb++ = color_wheel(y + step);
                else
                    *fb++ = color_wheel(x + step);
            }
        }
        lcd_run();
    } else {
        printf("lcd sub-commands (usage: lcd [subcmd]):\n");
        printf("\tinit    Initialize LCD registers\n");
        printf("\trun     Transfer one frame of the LCD\n");
        printf("\tstop    Stop and reset LCD auto-update\n");
#ifdef LCD_DEBUG
        printf("\tdump    Dump current register list\n");
#endif
        printf("\ttpp1    Display bitbanged, PIO 'test pattern 1'\n");
        printf("\ttpp2    Display bitbanged, PIO 'test pattern 2'\n");
        printf("\ttpd     DMA test pattern (flips on each iteration)\n");
    }

    return 0;
}
