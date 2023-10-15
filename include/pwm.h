#ifndef _PWM_H_
#define _PWM_H_

#include <stdint.h>
#include "reg_base.h"

#define KPLED_PWMCHANNEL PWM_CHANNEL2
#define LCD_PWMCHANNEL PWM_CHANNEL3

typedef enum tag_PWM {
  PWM_CHANNEL1,
  PWM_CHANNEL2,
  PWM_CHANNEL3,
  PWM_CHANNEL4,
  PWM_CHANNELS
} TPWM;

typedef enum tag_PWM_FLAGS {
  PWF_CLKDIV1 = (0 << 0),
  PWF_CLKDIV2 = (1 << 0),
  PWF_CLKDIV4 = (2 << 0),
  PWF_CLKDIV8 = (3 << 0),
  PWF_ENABLED = (1 << 2),
  PWF_FSEL_32K = (1 << 3),
  PWF_ALW_HIGH = (1 << 4)
} TPWM_FLAGS;

typedef __attribute__((aligned(4))) struct tag_PWMREGS {
  uint16_t Ctrl;
  uint16_t Count;
  uint16_t Threshold;
} TPWMREGS, *pPWMREGS;

typedef struct tag_PWM_CONTEXT {
  TPWMREGS *PWM;
  uint32_t PD_Code;
  TPWM_FLAGS Flags;
  uint16_t Threshold;
  uint8_t Duty;
} TPWM_CONTEXT, *pPWM_CONTEXT;

// CTRL
#define PWM_CLKDIV(v) (((v)&0x03) << 0) // This field is missing for PWM2.
#define PWM_CLKSEL_13M (0 << 2)
#define PWM_CLKSEL_32K (1 << 2)
#define PWM3_ALWAYS_HIGH (1 << 3) // For PWM3 only (1 for duty = 100%))
// COUNT
#define PWM_COUNT(v) (((v)&0x1FFF) << 0) // This register is missing for PWM2.
// THRESH
#define PWM_THRESH(v) (((v)&0x1FFF) << 0) // This field is missing for PWM2.
#define PWM2_DUTY_0 (0 << 0)              // Duty setup for PWM2
#define PWM2_DUTY_50 (1 << 0)
#define PWM2_DUTY_100 (2 << 0)
#define PWM2_DUTY_MASK 0x03

#define PWM2_BASE (PWM_2CH_base + 0x000C)
#define PWM3_BASE (PWM_2CH_base + 0x0018)

#define PWM1 ((volatile TPWMREGS *)PWM_base)
#define PWM2 ((volatile TPWMREGS *)PWM2_BASE)
#define PWM3 ((volatile TPWMREGS *)PWM3_BASE)
#define PWM4 ((volatile TPWMREGS *)PWM2_base)

extern int PWM_SetupChannel(TPWM Index, uint16_t Count, uint16_t Threshold,
                                TPWM_FLAGS Flags);
extern int PWM_SetPowerDown(TPWM Index, int PowerDown);
extern int PWM_SetCount(TPWM Index, uint16_t Count);
extern int PWM_SetThreshold(TPWM Index, uint16_t Threshold);
extern int PWM_SetDutyCycle(TPWM Index, uint32_t Duty);

#endif /* _PWM_H_ */