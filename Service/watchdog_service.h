#ifndef WATCHDOG_SERVICE_H
#define WATCHDOG_SERVICE_H

#include <stdint.h>
#include "typedefs.h"

typedef enum
{
    WD_SENSOR = 0,
    WD_PROCESS,
    WD_SAFETY,
    WD_DISPLAY,
    WD_COMM,
    WD_TASK_MAX

}wd_task_id_t;

void Watchdog_Init(void);

void Watchdog_Register(wd_task_id_t id);

void Watchdog_Feed(wd_task_id_t id);

uint8_t Watchdog_Check(void);

#endif
