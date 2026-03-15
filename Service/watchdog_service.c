#include "watchdog_service.h"
#include <string.h>

static uint32_t wd_counter[WD_TASK_MAX];
static uint32_t wd_last[WD_TASK_MAX];
static uint8_t  wd_enable[WD_TASK_MAX];
static uint8_t  wd_miss[WD_TASK_MAX];

#define WD_MAX_MISS 6   /* 500ms * 6 = 3s 容忍时间 */

void Watchdog_Init(void)
{
    memset(wd_counter, 0, sizeof(wd_counter));
    memset(wd_last, 0, sizeof(wd_last));
    memset(wd_enable, 0, sizeof(wd_enable));
    memset(wd_miss, 0, sizeof(wd_miss));
}

void Watchdog_Register(wd_task_id_t id)
{
    if(id < WD_TASK_MAX)
    {
        wd_enable[id] = 1;
        wd_counter[id] = 0;
        wd_last[id] = 0;
        wd_miss[id] = 0;
    }
}

void Watchdog_Feed(wd_task_id_t id)
{
    if(id < WD_TASK_MAX && wd_enable[id])
    {
        wd_counter[id]++;
    }
}

uint8_t Watchdog_Check(void)
{
    for(int i = 0; i < WD_TASK_MAX; i++)
    {
        if(wd_enable[i] == 0)
            continue;

        if(wd_counter[i] == wd_last[i])
        {
            wd_miss[i]++;

            if(wd_miss[i] >= WD_MAX_MISS)
            {
                return 0;
            }
        }
        else
        {
            wd_last[i] = wd_counter[i];
            wd_miss[i] = 0;
        }
    }

    return 1;
}
