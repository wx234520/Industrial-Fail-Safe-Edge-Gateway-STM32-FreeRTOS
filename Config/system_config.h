#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

/* ================== Sensor_service ================== */

#define FILTER_SIZE 8
#define TRACK_LIGHT_FREE_THRESHOLD   1200
#define TRACK_LIGHT_OCCUPIED_THRESHOLD  800

/* ================== Watchdog ================== */

#define WATCHDOG_TIMEOUT_MS        500

/* ================== Alarm Threshold ================== */

#define VOLTAGE_LOW_WARNING        24.6f
#define VOLTAGE_HIGH_WARNING       30.0f

#define CURRENT_HIGH_WARNING       5.0f

#define TEMP_HIGH_WARNING          35

#define HUMI_LOW_WARNING           30
#define HUMI_HIGH_WARNING          80

#define VOL_ALARM_HYSTERESIS       0.8f
#define CUR_ALARM_HYSTERESIS       0.5f
#define TEMP_ALARM_HYSTERESIS      10
#define HUMI_ALARM_HYSTERESIS      10

/* ================== Debug ================== */

#define SYSTEM_DEBUG_ENABLE        1

#endif
