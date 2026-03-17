#include "driver_adc.h"
#include "driver_dht11.h"
#include "typedefs.h"
#include "system_config.h"
#include "FreeRTOS.h"
#include "task.h"

sensor_data_t g_sensor;

uint16_t light_filter(uint16_t new_value)
{
    static uint16_t buffer[FILTER_SIZE] = {0};
    static byte index = 0;
    static uint32_t sum = 0;

    sum -= buffer[index];
    buffer[index] = new_value;
    sum += buffer[index];

    index = (index + 1) % FILTER_SIZE;

    return sum / FILTER_SIZE;
}

bool Sensor_IsTrackOccupied(const sensor_data_t *data)
{
    if(data->filtered_light < TRACK_LIGHT_OCCUPIED_THRESHOLD)
    {
        return true; // Track is occupied
    }
    else if(data->filtered_light > TRACK_LIGHT_FREE_THRESHOLD)
    {
        return false; // Track is unoccupied
    }
    else
    {
        // In the hysteresis zone, maintain previous state or use additional logic
        static bool last_state = false;
        return last_state;
    }
}

void sensor_update(void)
{
    static TickType_t last_dht_tick = 0;
    TickType_t now;
    int hum, temp;

    g_sensor.raw_light = ADC_GetRaw(2); // Assuming light sensor is connected to channel 2
    g_sensor.filtered_light = light_filter(g_sensor.raw_light);

    g_sensor.voltage = ADC_GetRaw(0) * 30.0f / 4095.0f; // Assuming a 12-bit ADC and a reference voltage of 3.3V
    g_sensor.current = ADC_GetRaw(1) * 5.0f / 4095.0f; // Assuming a 12-bit ADC and a reference voltage of 3.3V
    g_sensor.track_occupied = Sensor_IsTrackOccupied(&g_sensor); // Assuming a 12-bit ADC and a reference voltage of 3.3V

    now = xTaskGetTickCount();
    if((now - last_dht_tick) >= pdMS_TO_TICKS(2000))
    {
        vTaskSuspendAll();
        if(DHT11_Read(&hum, &temp) == 0)
        {
            g_sensor.humidity = hum;
            g_sensor.temperature = temp;
            g_sensor.dht11_valid = true;
        }
        else
        {
            g_sensor.dht11_valid = false;
        }
        xTaskResumeAll();

        last_dht_tick = now;
    }
}
