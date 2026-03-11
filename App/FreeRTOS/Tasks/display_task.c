#include "FreeRTOS.h"
#include "task.h"
#include "driver_oled.h"
#include "sensor_service.h"
#include "watchdog_service.h"
#include "typedefs.h"
#include "queue.h"

extern QueueHandle_t DisplayQueue;
static system_data_t display_data;

void DisplayTask(void *argument)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "Voltage:  .   V");
    OLED_ShowString(2, 1, "Current: .   A");
    OLED_ShowString(3, 1, "Track:");
    OLED_ShowString(4, 1, "State:");

    for(;;)
    {   
        if(xQueueReceive(DisplayQueue, &display_data, portMAX_DELAY) == pdPASS)
        {
            OLED_ShowNum(1, 9, display_data.voltage, 2); 
            OLED_ShowNum(2, 9, display_data.current, 1); 

            OLED_ShowNum(1, 12, display_data.voltage * 1000, 3);
            OLED_ShowNum(2, 11, display_data.current * 1000, 3);

            if(display_data.track_state == 0)
            {
                OLED_ShowString(3, 7,"UNOCC");
            }
            else
            {
                OLED_ShowString(3, 7,"OCC  ");
            }

            if(display_data.system_state == SYS_WARNING)
            {
                OLED_ShowString(4, 7, "WARNING!");
            }
            else if(display_data.system_state == SYS_FAULT)
            {
                OLED_ShowString(4, 7, "FAULT!  ");
            }
            else if(display_data.system_state == SYS_ERROR)
            {
                OLED_ShowString(4, 7, "ERROR!   ");
            }
            else
            {
                OLED_ShowString(4, 7, "NORMAL  ");
            }
        } 

        Watchdog_Feed(WD_DISPLAY);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
