#include "FreeRTOS.h"
#include "task.h"
#include "stm32f1xx_hal.h"
#include "watchdog_service.h"
#include "system_config.h"

extern IWDG_HandleTypeDef hiwdg;

void WatchdogTask(void *argument)
{
    for(;;)
    {
        if(Watchdog_Check())
        {
            HAL_IWDG_Refresh(&hiwdg); 
        }
        else
        {
            // 某任务异常
            // 不喂狗，让IWDG复位
        }

        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_TIMEOUT_MS));
    }
}
