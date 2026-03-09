#include "safety_service.h"
#include "driver_led.h"
#include "driver_buzzer.h"
#include "typedefs.h"
#include "system_config.h"

static system_state_t state = SYS_NORMAL;

static alarm_threshold_t g_alarm_threshold = {
    .voltage_low_warning = VOLTAGE_LOW_WARNING,   
    .voltage_high_warning = VOLTAGE_HIGH_WARNING,   
    .current_high_warning = CURRENT_HIGH_WARNING,  
    .hysteresis = ALARM_HYSTERESIS              
};

system_state_t WarningProcess(system_data_t *data)
{
    static uint8_t start_filter = 0;
    if(start_filter < 10)
    {
        start_filter++;
        return SYS_NORMAL;
    }

    float v = data->voltage;
    float i = data->current;

    float vol_low  = g_alarm_threshold.voltage_low_warning;
    float vol_high = g_alarm_threshold.voltage_high_warning;
    float hyst = g_alarm_threshold.hysteresis;
    float curr_high = g_alarm_threshold.current_high_warning;

    switch (state) {
        case SYS_NORMAL:
            if (v < vol_low || v > vol_high || i > curr_high) {
                state = SYS_WARNING;
            }
            break;

        case SYS_WARNING:
            if (v < vol_low - hyst || v > vol_high + hyst || i > curr_high + hyst) {
                state = SYS_FAULT;
            }
            else if (v >= vol_low && v <= vol_high && i <= curr_high) {
                state = SYS_NORMAL;
            }
            break;

        case SYS_FAULT:
            if (v >= vol_low && v <= vol_high && i <= curr_high) {
                state = SYS_NORMAL;
            }
            break;

        default:
            state = SYS_NORMAL;
            break;
    }

    switch (state) {
        case SYS_NORMAL:
            LED_OFF();
            Buzzer_Off();
            break;
        case SYS_WARNING:
            LED_Warning();   
            break;
        case SYS_FAULT:
            LED_Warning();   
            Buzzer_On();
            break;
    }
    return state;
}

void SafetyService_HeartbeatTimeout(void)
{
    state = SYS_ERROR;

    LED_Warning();
    Buzzer_On();
}
