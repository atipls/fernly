#include "pwm.h"
#include "power.h"

static TPWM_CONTEXT PWMINFO[PWM_CHANNELS] = {
        {(TPWMREGS *) PWM1, PD_PWM1},
        {(TPWMREGS *) PWM2, PD_PWM2},
        {(TPWMREGS *) PWM3, PD_PWM3},
        {(TPWMREGS *) PWM4, PD_PWM4},
};

int PWM_SetupChannel(TPWM Index, uint16_t Count, uint16_t Threshold,
                     TPWM_FLAGS Flags) {
    if (Index >= PWM_CHANNELS)
        return 0;
    else {
        pPWMREGS PWM = PWMINFO[Index].PWM;

        PWM->Ctrl = (Flags & PWF_FSEL_32K) ? PWM_CLKSEL_32K : PWM_CLKSEL_13M;
        PWMINFO[Index].Threshold = PWM_THRESH(Threshold);

        if (Index == PWM_CHANNEL2) {
            /* PWM2 setup */
            PWMINFO[Index].Threshold &= PWM2_DUTY_MASK;
            if (PWMINFO[Index].Threshold > PWM2_DUTY_100)
                PWMINFO[Index].Threshold = PWM2_DUTY_100;
        } else {
            /* PWM1, PWM2, PWM4 setup */
            PWM->Ctrl |= PWM_CLKDIV(Flags & PWF_CLKDIV8);
            PWM->Count = PWM_COUNT(Count);
            if (((uintptr_t) PWM == PWM3_BASE) && (Flags & PWF_ALW_HIGH))
                PWM->Ctrl |= PWM3_ALWAYS_HIGH;
        }
        PWM->Threshold = PWM_THRESH(Threshold);
        PWM_SetPowerDown(Index, !!(Flags & PWF_ENABLED));
        PWMINFO[Index].Flags = Flags;
    }
    return 1;
}

int PWM_SetPowerDown(TPWM Index, int PowerDown) {
    if (Index >= PWM_CHANNELS)
        return 0;
    else {
        int AlreadyInState = !(PWMINFO[Index].Flags & PWF_ENABLED);

        if (AlreadyInState ^ PowerDown) {
            PWMINFO[Index].PWM->Threshold =
                    (PowerDown) ? 0 : PWMINFO[Index].Threshold;

            if (PowerDown) {
                PCTL_PowerDown(PWMINFO[Index].PD_Code);
                PWMINFO[Index].Flags &= ~PWF_ENABLED;
            } else {
                PCTL_PowerUp(PWMINFO[Index].PD_Code);
                PWMINFO[Index].Flags |= PWF_ENABLED;
            }
        }
    }
    return 1;
}

int PWM_SetCount(TPWM Index, uint16_t Count) {
    if (Index >= PWM_CHANNELS)
        return 0;
    else if (Index >= PWM_CHANNEL2)
        return 1;
    else {
        PWMINFO[Index].PWM->Count = PWM_COUNT(Count);
    }
    return 1;
}

int PWM_SetThreshold(TPWM Index, uint16_t Threshold) {
    if (Index >= PWM_CHANNELS)
        return 0;
    else if (Index >= PWM_CHANNEL2) {
        PWMINFO[Index].Threshold &= PWM2_DUTY_MASK;
        if (PWMINFO[Index].Threshold > PWM2_DUTY_100)
            PWMINFO[Index].Threshold = PWM2_DUTY_100;
    }
    PWMINFO[Index].Threshold = PWM_THRESH(Threshold);
    PWMINFO[Index].PWM->Threshold = PWMINFO[Index].Threshold;

    return 1;
}

int PWM_Min(int a, int b) { return (a < b) ? a : b; }

int PWM_SetDutyCycle(TPWM Index, uint32_t Duty) {// Duty in percents [0..100]
    if (Index >= PWM_CHANNELS)
        return 0;
    else if (Index == PWM_CHANNEL2) {
        if (Duty < 50)
            PWMINFO[Index].Threshold = PWM2_DUTY_0;
        else if (Duty < 100)
            PWMINFO[Index].Threshold = PWM2_DUTY_50;
        else
            PWMINFO[Index].Threshold = PWM2_DUTY_100;
    } else {
        Duty = PWM_Min(Duty, 100);
        if (PWMINFO[Index].PWM->Count)
            PWMINFO[Index].Threshold =
                    PWM_THRESH((Duty * (PWMINFO[Index].PWM->Count - 1)) / 100);
        else
            PWMINFO[Index].Threshold = 0;
    }
    PWMINFO[Index].PWM->Threshold = PWMINFO[Index].Threshold;

    return 1;
}