#include "power.h"

void PCTL_Initialize(void)
{
    CNFG_PDN_CON0_SET = CNFG_PDN0_MASK;
    CNFG_PDN_CON1_SET = CNFG_PDN1_MASK;
    CNFG_PDN_CON2_SET = CNFG_PDN2_MASK;

    ACFG_CLK_CON_SET = ACFG_CLK_MASK;
}

int PCTL_GetPeripheralPowerStatus(uint32_t Periph)
{
    if      (Periph <= PD_CNFG_MAX) return !(CNFG_PDN_CON(Periph >> 4) & (1 << (Periph & 0x0F)));
    else if (Periph <= PD_ACFG_MAX) return !(ACFG_CLK_CON & (1 << (Periph & 0x0F)));

    return 0;
}