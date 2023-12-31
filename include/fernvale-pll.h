#ifndef __FV_PLL_H__
#define __FV_PLL_H__

/* Register names and values adapted from:
 * https://github.com/wiko-sources/cink-slim/blob
 *	/master/mediatek/platform/mt6577
 *	/kernel/core/include/mach/mt6577_clock_manager.h
 */

#define PLL_CTRL_ADDR 0xa0170000
#define PLL_CLK_ADDR 0x80000100

#define PLL_CTRL_XOSC_CON0 (PLL_CTRL_ADDR + 0x00)
#define PLL_CTRL_XOSC_CON1 (PLL_CTRL_ADDR + 0x04)

#define PLL_CTRL_CLKSQ_CON0 (PLL_CTRL_ADDR + 0x20)
#define PLL_CTRL_CLKSQ_CON1 (PLL_CTRL_ADDR + 0x24)
#define PLL_CTRL_CLKSQ_CON2 (PLL_CTRL_ADDR + 0x28)

#define PLL_CTRL_CON0 (PLL_CTRL_ADDR + 0x40)
#define PLL_CTRL_CON1 (PLL_CTRL_ADDR + 0x44)
#define PLL_CTRL_CON2 (PLL_CTRL_ADDR + 0x48)
#define PLL_CTRL_CON3 (PLL_CTRL_ADDR + 0x4c)
#define PLL_CTRL_CON4 (PLL_CTRL_ADDR + 0x50)
#define PLL_CTRL_CON5 (PLL_CTRL_ADDR + 0x54)
#define PLL_CTRL_CON6 (PLL_CTRL_ADDR + 0x58)
#define PLL_CTRL_CON7 (PLL_CTRL_ADDR + 0x5c)
#define PLL_CTRL_CON8 (PLL_CTRL_ADDR + 0x5c)
#define PLL_CTRL_CON9 (PLL_CTRL_ADDR + 0x5c)
#define PLL_CTRL_CON10 (PLL_CTRL_ADDR + 0x5c)
#define PLL_CTRL_CON11 (PLL_CTRL_ADDR + 0x5c)

#define PLL_CTRL_DPM_CON0 (PLL_CTRL_ADDR + 0x90)
#define PLL_CTRL_DPM_CON1 (PLL_CTRL_ADDR + 0x94)
#define PLL_CTRL_DPM_CON2 (PLL_CTRL_ADDR + 0x98)

#define PLL_CTRL_MPLL_CON0 (PLL_CTRL_ADDR + 0x100)
#define PLL_CTRL_MPLL_CON1 (PLL_CTRL_ADDR + 0x104)
#define PLL_CTRL_MPLL_CON2 (PLL_CTRL_ADDR + 0x108)

#define PLL_CTRL_UPLL_CON0 (PLL_CTRL_ADDR + 0x140)
#define PLL_CTRL_UPLL_CON1 (PLL_CTRL_ADDR + 0x144)
#define PLL_CTRL_UPLL_CON2 (PLL_CTRL_ADDR + 0x148)

#define PLL_CTRL_EPLL_CON0 (PLL_CTRL_ADDR + 0x180)
#define PLL_CTRL_EPLL_CON1 (PLL_CTRL_ADDR + 0x184)
#define PLL_CTRL_EPLL_CON2 (PLL_CTRL_ADDR + 0x188)

#define PLL_CTRL_FH_CON0 (PLL_CTRL_ADDR + 0x500)
#define PLL_CTRL_FH_CON1 (PLL_CTRL_ADDR + 0x504)
#define PLL_CTRL_FH_CON2 (PLL_CTRL_ADDR + 0x508)
#define PLL_CTRL_FH_CON3 (PLL_CTRL_ADDR + 0x50c)
#define PLL_CTRL_FH_CON4 (PLL_CTRL_ADDR + 0x510)

#define PLL_CTRL_MDDS_CON0 (PLL_CTRL_ADDR + 0x640)
#define PLL_CTRL_MDDS_CON1 (PLL_CTRL_ADDR + 0x644)
#define PLL_CTRL_MDDS_CON2 (PLL_CTRL_ADDR + 0x648)

#define PLL_CTRL_EDDS_CON0 (PLL_CTRL_ADDR + 0x680)
#define PLL_CTRL_EDDS_CON1 (PLL_CTRL_ADDR + 0x684)
#define PLL_CTRL_EDDS_CON2 (PLL_CTRL_ADDR + 0x688)

#define PLL_CTRL_CLK_CONDA (PLL_CLK_ADDR + 0x00)
#define PLL_CTRL_CLK_CONDB (PLL_CLK_ADDR + 0x04)
#define PLL_CTRL_CLK_CONDC (PLL_CLK_ADDR + 0x08)
#define PLL_CTRL_CLK_CONDD (PLL_CLK_ADDR + 0x0c)
#define PLL_CTRL_CLK_CONDE (PLL_CLK_ADDR + 0x10)
#define PLL_CTRL_CLK_CONDF (PLL_CLK_ADDR + 0x14)
#define PLL_CTRL_CLK_CONDG (PLL_CLK_ADDR + 0x18)
#define PLL_CTRL_CLK_CONDH (PLL_CLK_ADDR + 0x1c)

#define SFC_MUX_SEL(v) (((v)&0x07) << 0)
#define LCD_MUX_SEL(v) (((v)&0x07) << 4)
#define DSP_MUX_SEL(v) (((v)&0x03) << 9)

#endif /* __FV_PLL_H__ */
