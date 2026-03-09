#include "watchdog_service.h"

static uint32_t wd_counter[WD_TASK_MAX];
static uint32_t wd_last[WD_TASK_MAX];

void Watchdog_Feed(wd_task_id_t id)
{
    if(id < WD_TASK_MAX)
    {
        wd_counter[id]++;
    }
}

uint8_t Watchdog_Check(void)
{
    for(int i = 0; i < WD_TASK_MAX; i++)
    {
        if(wd_counter[i] == wd_last[i])
        {
            return 0; // 某任务没喂狗
        }

        wd_last[i] = wd_counter[i];
    }

    return 1;
}
