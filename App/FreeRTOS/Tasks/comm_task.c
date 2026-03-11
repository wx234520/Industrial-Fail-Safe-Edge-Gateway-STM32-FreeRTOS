#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "watchdog_service.h"
#include <stdio.h>
#include "semphr.h"
#include "esp8266.h"

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern uint8_t esp8266_buf[256];
extern uint8_t dma_rx_buf[256];

void CommTask(void *argument)
{
    printf("ESP Task Start\r\n");

    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, sizeof(dma_rx_buf));
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

    ESP8266_Init();

    while(1)
    {
        printf("ESP Alive\r\n");

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

}
