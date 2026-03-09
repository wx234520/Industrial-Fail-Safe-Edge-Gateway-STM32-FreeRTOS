#ifndef WATCHDOG_SERVICE_H
#define WATCHDOG_SERVICE_H

#include <stdint.h>
#include "typedefs.h"

void Watchdog_Feed(wd_task_id_t id);

uint8_t Watchdog_Check(void);

#endif
