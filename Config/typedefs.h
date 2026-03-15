#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t byte;
typedef uint16_t uint;
typedef uint32_t ulong;

typedef enum
{
    SYS_NORMAL,
    SYS_WARNING,
    SYS_FAULT,
    SYS_ERROR
}system_state_t;

typedef struct
{
    uint16_t raw_light;     // 原始ADC值（用于调试）
    uint16_t filtered_light;// 滤波后光强值
    
    float    voltage;       
    float    current;       
    bool     track_occupied;// 轨道占用标志
    
    int temperature;
    int humidity;
    bool dht11_valid;
} sensor_data_t;

typedef struct
{
    float voltage;
    float current;
    uint8_t track_state;   // 0 未占用 1 占用

    int temperature;
    int humidity;
    bool dht11_valid;

    system_state_t system_state;
}system_data_t;

typedef struct {
    float voltage_low_warning;     // 低压预警阈值 (V)
    float voltage_high_warning;    // 高压预警阈值 (V)

    float current_high_warning;       // 高流预警

    int temp_high_warning;        

    int humi_low_warning;
    int humi_high_warning;

    float vol_hysteresis;              // 迟滞宽度 
    float cur_hysteresis;
    int temp_hysteresis;
    int humi_hysteresis;
} alarm_threshold_t;

#endif /* TYPEDEFS_H_ */
