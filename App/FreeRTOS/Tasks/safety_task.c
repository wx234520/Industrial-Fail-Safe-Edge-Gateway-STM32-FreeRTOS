#include "FreeRTOS.h"
#include "task.h"
#include "typedefs.h"
#include "queue.h"
#include "safety_service.h"

extern QueueHandle_t SafeQueue;
extern QueueHandle_t DisplayQueue;

void SafetyTask(void *argument)
{
    static system_data_t safety_data;
    uint32_t heartbeat_counter = 0;
    TickType_t last_heartbeat = xTaskGetTickCount();

    for(;;)
    {
        if(xQueueReceive(SafeQueue, &safety_data, pdMS_TO_TICKS(100)) == pdPASS)
        {        
            safety_data.system_state = WarningProcess(&safety_data);
            xQueueOverwrite(DisplayQueue,&safety_data);

            heartbeat_counter = 0; // Reset heartbeat counter on successful data reception
            last_heartbeat = xTaskGetTickCount(); // Update last heartbeat time
        }

        Watchdog_Feed(WD_SAFETY);
        vTaskDelay(pdMS_TO_TICKS(100));
        heartbeat_counter++;

        if(heartbeat_counter > 10) // If no data received for 1 second
        {
            SafetyService_HeartbeatTimeout();
        }
    }
}
