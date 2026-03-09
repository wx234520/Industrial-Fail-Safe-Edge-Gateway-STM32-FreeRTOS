#include "FreeRTOS.h"
#include "task.h"
#include "driver_adc.h"
#include "sensor_service.h"
#include "watchdog_service.h"

void SensorTask(void *argument)
{
    ADC_Start();

    for(;;)
    {
        sensor_update();

        Watchdog_Feed(WD_SENSOR);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
