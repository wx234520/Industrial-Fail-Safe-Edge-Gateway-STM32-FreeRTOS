#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "watchdog_service.h"
#include <stdio.h>
#include "semphr.h"
#include "esp8266.h"
#include "onenet.h"

#define ESP8266_ONENET_INFO		"AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"
#define ESP_RST_GPIO_Port        GPIOB
#define ESP_RST_Pin              GPIO_PIN_12

uint8_t temp = 25;
uint8_t humi = 60;
static system_data_t comm_data;

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern uint8_t esp8266_buf[512];
extern uint8_t dma_rx_buf[512];
extern QueueHandle_t CommQueue;

void ESP8266_Reset(void)
{
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_RESET);
    vTaskDelay(pdMS_TO_TICKS(200));

    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_SET);
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void CommTask(void *argument)
{
    ESP8266_Reset();

    printf("ESP Task Start\r\n");

    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, sizeof(esp8266_buf));
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

    /* 初始化ESP */
    ESP8266_Init();

    printf("Connect MQTT Server...\r\n");

    /* TCP连接 */
    while(ESP8266_SendCmd(
        "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n",
        "OK"))
    {
        printf("TCP Retry...\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    printf("TCP Connect Success\r\n");

    vTaskDelay(2000);
    /* MQTT连接 */
    while(OneNet_DevLink())
    {
        printf("MQTT Connect Fail, retry...\r\n");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    printf("MQTT Connect Success\r\n");

    OneNET_Subscribe();

    while(1)
    {
        // OneNet_SendData();

        // unsigned char *cmd = ESP8266_GetIPD(100);
        // if(cmd)
        // {
        //     OneNet_RevPro(cmd);
        // }

        // vTaskDelay(pdMS_TO_TICKS(5000));
        
    }
}
