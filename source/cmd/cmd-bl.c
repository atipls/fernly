#include "bionic.h"
#include "fernvale-bl.h"
#include "memio.h"
#include "serial.h"
#include "pwm.h"

#define RG_ISINKS_CH0(v) (((v)&0x07) << 4)
#define ISINKS_CH0_4mA 0
#define ISINKS_CH0_8mA 1
#define ISINKS_CH0_12mA 2
#define ISINKS_CH0_16mA 3
#define ISINKS_CH0_20mA 6
#define ISINKS_CH0_24mA 7

int cmd_bl(int argc, char **argv) {
  uint32_t level;
  int i;

  if (argc < 1) {
    serial_puts("Usage: bl [level 0-5]\n");
    return -1;
  }

  writew(0, BLLED_GANG_REG0);

  PWM_SetupChannel(LCD_PWMCHANNEL, 163, 0, PWF_FSEL_32K | PWF_CLKDIV1);

  level = strtoul(argv[0], NULL, 0);

  if (level > BLLED_MAX_LEVEL) {
    level = 0;
    serial_puts("Backlight level should be 0-5\n");
  }

  if (level > 0) {
    for (i = 0; i < BLLED_BANKS; i++) {
      *((volatile uint32_t *)BLLED_REG0_BANK(i)) =
          RG_ISINKS_CH0(ISINKS_CH0_20mA) | (1 << 0);
      PWM_SetDutyCycle(LCD_PWMCHANNEL, 100);
      PWM_SetPowerDown(LCD_PWMCHANNEL, 0);
    }

  } else {
    for (i = 0; i < BLLED_BANKS; i++) {
      *((volatile uint32_t *)BLLED_REG0_BANK(i)) = 0; // meh
    }
  }

  return 0;
}
