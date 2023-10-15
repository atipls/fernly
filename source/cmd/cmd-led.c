#include <string.h>
#include "bionic.h"
#include "memio.h"
#include "printf.h"
#include "serial.h"
#include "fernvale-kbd.h"
#include "fernvale-gpio.h"

int cmd_led(int argc, char **argv)
{
	uint32_t state;

	if (argc < 1) {
		printf("Usage: led [1 = on, 0 = off]\n");
		return -1;
	}

	state = strtoul(argv[0], NULL, 0);

	*((volatile uint32_t *) GPIO_CTRL_MODE2) =
		GPIO_CTRL_MODE2_IO16_GPIO16 |\
		GPIO_CTRL_MODE2_IO16_MASK |\
		GPIO_CTRL_MODE2_IO18_GPIO18 |\
		GPIO_CTRL_MODE2_IO18_MASK;

//		GPIO_CTRL_MODE2_IO17_GPIO17 |
//		GPIO_CTRL_MODE2_IO17_MASK |

	*((volatile uint32_t *) GPIO_CTRL_MODE1) =
		GPIO_CTRL_MODE1_IO12_GPIO12 |\
		GPIO_CTRL_MODE1_IO12_MASK;
	*((volatile uint32_t *) GPIO_CTRL_MODE0) =
		GPIO_CTRL_MODE0_IO5_GPIO15 |\
		GPIO_CTRL_MODE0_IO5_MASK;

	if( state ) {
		/*
		*((volatile uint32_t *) GPIO_CTRL_DOUT2) = 0xffff;
		*((volatile uint32_t *) GPIO_CTRL_DOUT2) = 0xffff;
		*((volatile uint32_t *) GPIO_CTRL_DOUT2) = 0xffff;
		*/
		//	  *((volatile uint32_t *) BIG_LED_ADDR) = BIG_LED_ON;
	} else {
		/*
		*((volatile uint32_t *) GPIO_CTRL_DOUT2) = 0x0;
		*((volatile uint32_t *) GPIO_CTRL_DOUT2) = 0x0;
		*((volatile uint32_t *) GPIO_CTRL_DOUT2) = 0x0;
		*/
		//	  *((volatile uint32_t *) BIG_LED_ADDR) = BIG_LED_OFF;
	}

	return 0;
}

