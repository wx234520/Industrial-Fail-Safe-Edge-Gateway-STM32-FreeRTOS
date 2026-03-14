/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include "driver_oled.h"
#include "driver_led.h"
#include "driver_buzzer.h"
#include "typedefs.h"
#include "sensor_task.h"
#include "process_task.h"
#include "display_task.h"
#include "watchdog_task.h"
#include "safety_task.h"
#include "comm_task.h"
#include "semphr.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define PRIO_SENSOR      osPriorityRealtime
#define PRIO_PROCESS     osPriorityHigh
#define PRIO_SAFETY      osPriorityAboveNormal
#define PRIO_WATCHDOG    osPriorityAboveNormal
#define PRIO_DISPLAY     osPriorityNormal
#define PRIO_COMM        osPriorityNormal
#define PRIO_LOG         osPriorityLow
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
QueueHandle_t DisplayQueue;
QueueHandle_t SafeQueue;
QueueHandle_t CommQueue;
SemaphoreHandle_t esp_rx_semaphore;
SemaphoreHandle_t esp_buf_mutex;
SemaphoreHandle_t esp_tx_mutex;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  OLED_Init();
  LED_Init();
  Buzzer_Init();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  SafeQueue = xQueueCreate(1, sizeof(system_data_t));
  DisplayQueue = xQueueCreate(1, sizeof(system_data_t));
  CommQueue = xQueueCreate(1,sizeof(system_data_t));
  esp_rx_semaphore = xSemaphoreCreateBinary();
  esp_buf_mutex = xSemaphoreCreateMutex();
  esp_tx_mutex = xSemaphoreCreateMutex();
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  //defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  xTaskCreate(SensorTask, "SensorTask", 128, NULL, PRIO_SENSOR, NULL);
  xTaskCreate(ProcessTask, "ProcessTask", 128, NULL, PRIO_PROCESS, NULL);
  xTaskCreate(DisplayTask, "DisplayTask", 128, NULL, PRIO_DISPLAY, NULL);
  xTaskCreate(SafetyTask, "SafetyTask", 128, NULL, PRIO_SAFETY, NULL);
  xTaskCreate(WatchdogTask, "WatchdogTask", 128, NULL, PRIO_WATCHDOG,NULL);
  xTaskCreate(CommTask, "CommTask", 1024, NULL, PRIO_COMM, NULL);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    vTaskDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

