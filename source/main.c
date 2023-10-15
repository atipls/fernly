#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bionic.h"
#include "memio.h"
#include "serial.h"
#include "utils.h"
#include "scriptic.h"
#include "gpio.h"
#include "printf.h"

#include "fernvale-pmic.h"

#define PROMPT "fernly> "

static int serial_get_line(char *bfr, int len) {
       int cur = 0;

       while (cur < len) {
               bfr[cur] = serial_getc();
               serial_putc(bfr[cur]);

               /* Carriage Return */
               if (bfr[cur] == '\n') {
                       bfr[cur] = '\0';
                       return 0;
               }

               /* Linefeed */
               else if (bfr[cur] == '\r') {
                       bfr[cur] = '\0';
                       return 0;
               }

               /* Backspace */
               else if (bfr[cur] == 0x7f) {
                       bfr[cur] = '\0';

                       if (cur > 0) {
                               serial_putc('\b');
                               serial_putc(' ');
                               serial_putc('\b');
                               cur--;
                       }
               }

               /* Ctrl-U */
               else if (bfr[cur] == 0x15) {
                       while (cur > 0) {
                               serial_putc('\b');
                               serial_putc(' ');
                               serial_putc('\b');
                               bfr[cur] = '\0';
                               cur--;
                       }
               }

               /* Ctrl-W */
               else if (bfr[cur] == 0x17) {
                       while (cur > 0 && bfr[cur] != ' ') {
                               serial_putc('\b');
                               serial_putc(' ');
                               serial_putc('\b');
                               bfr[cur] = '\0';
                               cur--;
                       }
               }

               /* Escape code */
               else if (bfr[cur] == 0x1b) {
                       /* Next two characters are escape codes */
                       uint8_t next = serial_getc();
                       /* Sanity check: next should be '[' */
		       (void)next;

                       next = serial_getc();
               }
               else
                       cur++;
       }
       bfr[len - 1] = '\0';
       return -1;
}

static int cmd_help(int argc, char **argv);
extern int cmd_hex(int argc, char **argv);
extern int cmd_irq(int argc, char **argv);
extern int cmd_msleep(int argc, char **argv);
extern int cmd_peek(int argc, char **argv);
extern int cmd_poke(int argc, char **argv);
extern int cmd_readx(int argc, char **argv);
extern int cmd_writex(int argc, char **argv);
extern int cmd_spi(int argc, char **argv);
extern int cmd_spi_raw(int argc, char **argv);
extern int cmd_swi(int argc, char **argv);
extern int cmd_reboot(int argc, char **argv);
extern int cmd_led(int argc, char **argv);
extern int cmd_bl(int argc, char **argv);
extern int cmd_lcd(int argc, char **argv);
extern int cmd_load(int argc, char **argv);
extern int cmd_loadjump(int argc, char **argv);
extern int cmd_keypad(int argc, char **argv);

static const struct {
	int (*func)(int argc, char **argv);
	const char *name;
	const char *help;
} commands[] = {
	{
		.func = cmd_help,
		.name = "help",
		.help = "Print help about available commands",
	},
	{
		.func = cmd_reboot,
		.name = "reboot",
		.help = "Reboot Fernvale",
	},
	{
		.func = cmd_msleep,
		.name = "msleep",
		.help = "Sleep for some number of milliseconds",
	},
	{
		.func = cmd_hex,
		.name = "hex",
		.help = "Print area of memory as hex",
	},
	{
		.func = cmd_peek,
		.name = "peek",
		.help = "Look at one area of memory",
	},
	{
		.func = cmd_poke,
		.name = "poke",
		.help = "Write a value to an area of memory",
	},
	{
		.func = cmd_readx,
		.name = "readx",
		.help = "Read a value from an area of memory",
	},
	{
		.func = cmd_writex,
		.name = "writex",
		.help = "Write a value to an area of memory",
	},
	{
		.func = cmd_irq,
		.name = "irq",
		.help = "Manipulate IRQs",
	},

	{
		.func = cmd_spi,
		.name = "spi",
		.help = "Manipulate on-board SPI",
	},
	{
		.func = cmd_spi_raw,
		.name = "spi_raw",
		.help = "Manipulate on-board SPI (raw interface)",
	},
	{
		.func = cmd_swi,
		.name = "swi",
		.help = "Generate software interrupt",
	},
	{
		.func = cmd_led,
		.name = "led",
		.help = "Turn the on-board LED on or off",
	},
	{
		.func = cmd_bl,
		.name = "bl",
		.help = "Set the LCD backlight brightness",
	},
	{
		.func = cmd_lcd,
		.name = "lcd",
		.help = "Manipulate the LCD",
	},

	{
		.func = cmd_load,
		.name = "load",
		.help = "Load data to a specific area in memory",
	},
	{
		.func = cmd_loadjump,
		.name = "loadjmp",
		.help = "Load data to a specific area in memory, "
			"then jump to it",
	},

	{
		.func = cmd_keypad,
		.name = "keypad",
		.help = "Read keys from keypad until # is pressed ",
	},
};

int cmd_help(int argc, char **argv)
{
	int i;

	serial_puts("Fernly shell help.  Available commands:\n");
	for (i = 0; i < sizeof(commands) / sizeof(*commands); i++) {
		serial_puts("\t");
		serial_puts(commands[i].name);
		serial_puts("\t");
		serial_puts(commands[i].help);
		serial_puts("\n");
	}
	return 0;
}

static int shell_run_command(char *line) {
	char *lp, *cmd, *tokp;
	char *args[8];
	int i, n;

	lp = _strtok(line, " \t", &tokp);
	cmd = lp;
	n = 0;
	while ((lp = _strtok(NULL, " \t", &tokp)) != NULL) {
		if (n >= 7) {
			serial_puts("too many arguments\r\n");
			cmd = NULL;
			break;
		}
		args[n++] = lp;
	}
	args[n] = NULL;
	if (cmd == NULL)
		return -1;

	for (i = 0; i < sizeof(commands) / sizeof(*commands); i++)
		if (!_strcasecmp(commands[i].name, cmd))
			return commands[i].func(n, args);

	serial_puts("Unknown command: ");
	serial_puts(cmd);
	serial_puts("\n");

	return 0;
}

static int loop(void) {
	char line[256];

	serial_puts(PROMPT);
	serial_get_line(line, sizeof(line));
	serial_puts("\n");
	return shell_run_command(line);
}

int main(void) {
	uint8_t *p;
	serial_init();

	serial_puts("\nWelcome!\n");
	serial_puts("Setting up PLLs....\n");
	scriptic_run("set_plls");
	//scriptic_run("enable_psram");

	serial_puts("Setting up GPIO....\n");
	gpio_init();

	printf("We are at: %p\n", (void *)loop);
	printf("Stack pointer: %p\n", (void *)&p);

	while (1) loop();

	return 0x55AA55AA;
}
