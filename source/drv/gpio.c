
#include "gpio.h"
#include "memio.h"
#include "reg_base.h"

void gpio_init(void) {
  writel(GPIO_DIR0_MASK, GPIO_DIR0_CLR); // Set all GPIO input
  writel(GPIO_DIR1_MASK, GPIO_DIR1_CLR);

  writel(PULLEN0_MASK, GPIO_PULLEN0_CLR); // Disable Pullup/PullDown
  writel(PULLEN1_MASK, GPIO_PULLEN1_CLR);
  writel(RESEN0_0_MASK, GPIO_RESEN0_0_CLR);
  writel(RESEN0_1_MASK, GPIO_RESEN0_1_CLR);
  writel(RESEN1_0_MASK, GPIO_RESEN1_0_CLR);
  writel(RESEN1_1_MASK, GPIO_RESEN1_1_CLR);

  writel(GPIO_MODE0MASK, GPIO_MODE0_CLR);
  writel(GPIO_MODE1MASK, GPIO_MODE1_CLR);
  writel(GPIO_MODE2MASK, GPIO_MODE2_CLR);
  writel(GPIO_MODE3MASK, GPIO_MODE3_CLR);
  writel(GPIO_MODE4MASK, GPIO_MODE4_CLR);
  writel(GPIO_MODE5MASK, GPIO_MODE5_CLR);
  writel(GPIO_MODE6MASK, GPIO_MODE6_CLR);
}

void gpio_setup(uint16_t pin, uint16_t flags) {
      if (pin <= GPIOMAX)
    {
        if (flags & GPDO) GPIO_SETDIROUT(pin);
        else GPIO_SETDIRIN(pin);

        if (flags & GPDIEN) GPIO_SETINPUTEN(pin);
        else GPIO_SETINPUTDIS(pin);

        if (flags & GPPDN) GPIO_SETPULLDOWN(pin);
        else GPIO_SETPULLUP(pin);

        if (flags & GPPULLEN) GPIO_PULLENABLE(pin);
        else GPIO_PULLDISABLE(pin);

        if (flags & GPSMT) GPIO_SMTENABLE(pin);
        else GPIO_SMTDISABLE(pin);

        GPIO_SETMODE(pin, (flags >> 8));
    }
}
