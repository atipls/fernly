#include "bionic.h"
#include "memio.h"
#include "scriptic.h"

extern struct scriptic set_plls;
extern struct scriptic enable_psram;
extern struct scriptic spi_run;
extern struct scriptic spi_init;
extern struct scriptic set_kbd;

static struct scriptic *scripts[] = {
    &set_plls, &enable_psram, &spi_run, &spi_init, &set_kbd,
};

struct scriptic *scriptic_get(const char *name) {
  struct scriptic *script = NULL;
  int i;

  for (i = 0; i < sizeof(scripts) / sizeof(*scripts); i++) {
    if (!_strcasecmp(name, scripts[i]->name)) {
      script = scripts[i];
      break;
    }
  }

  return script;
}

int scriptic_run(const char *name) {
  struct scriptic *script;

  script = scriptic_get(name);
  if (!script) {
#ifdef SCRIPTIC_DEBUG
    printf("scriptic: Unrecognized script name: %s\n", name);
#endif
    return -1;
  }
  return scriptic_execute(script);
}
