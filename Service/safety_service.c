#include <stdio.h>
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
    .temp_high_warning = TEMP_HIGH_WARNING,
    .humi_low_warning = HUMI_LOW_WARNING,
    .humi_high_warning = HUMI_HIGH_WARNING,
    .vol_hysteresis = VOL_ALARM_HYSTERESIS,    
    .cur_hysteresis = CUR_ALARM_HYSTERESIS,
    .temp_hysteresis = TEMP_ALARM_HYSTERESIS,
    .humi_hysteresis = HUMI_ALARM_HYSTERESIS    
};

void SafetyService_SetThreshold(const alarm_threshold_t *threshold)
{
    if(threshold == NULL)
        return;

    g_alarm_threshold = *threshold;
}

void SafetyService_GetThreshold(alarm_threshold_t *threshold)
{
    if(threshold == NULL)
        return;

    *threshold = g_alarm_threshold;
}

system_state_t WarningProcess(system_data_t *data)
{
    static uint8_t start_filter = 0;

    if(start_filter < 10)
    {
        start_filter++;
        state = SYS_NORMAL;
        LED_OFF();
        Buzzer_Off();
        return state;
    }

    float v = data->voltage;
    float i = data->current;
    float t = data->temperature;
    float h = data->humidity;

    float vol_low   = g_alarm_threshold.voltage_low_warning;
    float vol_high  = g_alarm_threshold.voltage_high_warning;
    float curr_high = g_alarm_threshold.current_high_warning;

    int temp_high = g_alarm_threshold.temp_high_warning;

    int humi_low  = g_alarm_threshold.humi_low_warning;
    int humi_high = g_alarm_threshold.humi_high_warning;

    float vol_hyst = g_alarm_threshold.vol_hysteresis;
    float cur_hyst = g_alarm_threshold.cur_hysteresis;
    int temp_hyst = g_alarm_threshold.temp_hysteresis;
    int humi_hyst = g_alarm_threshold.humi_hysteresis;

    uint8_t warning_flag = 0;
    uint8_t fault_flag = 0;

    /* 电压/电流告警 */
    if(v < vol_low || v > vol_high || i > curr_high)
    {
        warning_flag = 1;
    }

    if(v < vol_low - vol_hyst || v > vol_high + vol_hyst || i > curr_high + cur_hyst)
    {
        fault_flag = 1;
    }

    /* 温度告警 */
    if(t > temp_high)
    {
        warning_flag = 1;
    }

    if(t > temp_high + temp_hyst)
    {
        fault_flag = 1;
    }

    /* 湿度告警 */
    if(h < humi_low || h > humi_high)
    {
        warning_flag = 1;
    }

    if(h < humi_low - humi_hyst || h > humi_high + humi_hyst)
    {
        fault_flag = 1;
    }

    /* 状态机 */
    switch(state)
    {
        case SYS_NORMAL:
            if(fault_flag)
                state = SYS_FAULT;
            else if(warning_flag)
                state = SYS_WARNING;
            break;

        case SYS_WARNING:
            if(fault_flag)
                state = SYS_FAULT;
            else if(!warning_flag)
                state = SYS_NORMAL;
            break;

        case SYS_FAULT:
            if(!fault_flag && !warning_flag)
                state = SYS_NORMAL;
            else if(!fault_flag && warning_flag)
                state = SYS_WARNING;
            break;

        default:
            state = SYS_NORMAL;
            break;
    }

    switch(state)
    {
        case SYS_NORMAL:
            LED_OFF();
            Buzzer_Off();
            break;

        case SYS_WARNING:
            LED_Warning();
            Buzzer_Off();
            break;

        case SYS_FAULT:
            LED_Warning();
            Buzzer_On();
            break;

        default:
            LED_OFF();
            Buzzer_Off();
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
