#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "typedefs.h"
#include "sensor_service.h"

static system_data_t g_process_data;

extern QueueHandle_t SafeQueue;
extern QueueHandle_t DisplayQueue;

void ProcessTask(void *argument)
{
    for(;;)
    {
        g_process_data.voltage = g_sensor.voltage;
        g_process_data.current = g_sensor.current;
        g_process_data.track_state = g_sensor.track_occupied;

        //xQueueSend(SafeQueue, &g_process_data, portMAX_DELAY);
        xQueueOverwrite(SafeQueue, &g_process_data);

        Watchdog_Feed(WD_PROCESS);
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
