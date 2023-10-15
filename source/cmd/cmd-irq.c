#include <string.h>

#include "bionic.h"
#include "irq.h"
#include "printf.h"
#include "serial.h"

static void print_help(void)
{
	serial_puts("Usage:\n");
	serial_puts("irq init            Initialize interrupts\n");
	serial_puts("irq sim [num]      Simulate IRQ [num]\n");
	serial_puts("irq enable [num]   Enable IRQ [num]\n");
	serial_puts("irq disable [num]  Disable IRQ [num]\n");
}

int cmd_irq(int argc, char **argv)
{
	int num;


	if (argc == 1 && !_strcasecmp(argv[0], "init")) {
		printf("Initializing interrupts\n");
		irq_init();
		return 0;
	}

	if (argc == 1 && !_strcasecmp(argv[0], "undef")) {
		asm volatile(".word 0xffffffff\n");
		return 0;
	}


	if (argc != 2) {
		print_help();
		return -1;
	}

	num = strtoul(argv[1], NULL, 0);
	if (num >= __irq_max__) {
		printf("Only %d IRQs present\n", __irq_max__);
		return -1;
	}

	if (!_strcasecmp(argv[0], "sim")) {
		printf("Simulating IRQ %d\n", num);
		irq_stimulate(num);
	}

	else if (!_strcasecmp(argv[0], "enable")) {
		printf("Enabling IRQ %d\n", num);
		irq_enable(num);
	}

	else if (!_strcasecmp(argv[0], "disable")) {
		printf("Disabling IRQ %d\n", num);
		irq_disable(num);
	}

	else {
		printf("Unknown command\n");
		print_help();
		return -1;
	}

	return 0;
}

int cmd_swi(int argc, char **argv)
{
	printf("Generating SWI...\n");
	asm volatile ("swi #0\n");
	printf("Returned from SWI\n");

	return 0;
}
