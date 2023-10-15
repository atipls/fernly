#include <string.h>
#include "bionic.h"
#include "memio.h"
#include "serial.h"
#include "utils.h"

char welcome_banner[] = "Fernly stage 1 loader for 6261\nWrite four bytes of program size, then write program data...\n>";

#define LOADADDR 0x10000000

int main() {
  uint32_t i;
  uint32_t psize;
  volatile uint8_t *p;
  void (*jumpaddr)(void);

  serial_init();

  serial_puts(welcome_banner);

  p = (volatile uint8_t *)&psize;
  for (i = 0; i < 4; ++i)
    p[i] = serial_getc();

  p = (volatile uint8_t *)LOADADDR;
  for (i = 0; i < psize; ++i)
    p[i] = serial_getc();

  jumpaddr = (void (*)(void))LOADADDR;
  jumpaddr();

  return 0;
}
