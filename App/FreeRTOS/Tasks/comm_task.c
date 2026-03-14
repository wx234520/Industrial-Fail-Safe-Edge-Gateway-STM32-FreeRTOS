#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "watchdog_service.h"
#include <stdio.h>
#include <string.h>
#include "semphr.h"
#include "esp8266.h"
#include "onenet.h"

#define ESP8266_ONENET_INFO		"AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n"
#define ESP_RST_GPIO_Port        GPIOB
#define ESP_RST_Pin              GPIO_PIN_12
#define ESP_DMA_BUF_SIZE 256

system_data_t comm_data;
char rxbuf[512];

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern uint8_t dma_rx_buf[ESP_DMA_BUF_SIZE];
extern QueueHandle_t CommQueue;
extern SemaphoreHandle_t esp_tx_mutex;

void ESP8266_Reset(void) 
{ 
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_RESET); 
    vTaskDelay(pdMS_TO_TICKS(200)); 
    HAL_GPIO_WritePin(ESP_RST_GPIO_Port, ESP_RST_Pin, GPIO_PIN_SET); vTaskDelay(pdMS_TO_TICKS(2000)); 
} 

void CommTask_Init(void)
 { 
    ESP8266_Reset(); 
    printf("ESP Task Start\r\n"); 
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, ESP_DMA_BUF_SIZE); 
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT); 
    /* 初始化ESP */ ESP8266_Init(); 
} 

void MQTT_Connect(void) 
{ 
    printf("Connect MQTT Server...\r\n"); 
    while(ESP8266_SendCmd( "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n", "OK")) 
    { 
        printf("TCP Retry...\r\n"); 
        ESP8266_SendCmd("AT+CIPCLOSE\r\n","OK");
        vTaskDelay(pdMS_TO_TICKS(3000)); 
    } 
    printf("TCP Connect Success\r\n"); 
    vTaskDelay(pdMS_TO_TICKS(3000)); 
   
    while(OneNet_DevLink()) 
    { 
        printf("MQTT Connect Fail, retry...\r\n"); 
        ESP8266_SendCmd("AT+CIPCLOSE\r\n","OK"); 
        vTaskDelay(pdMS_TO_TICKS(3000)); 
        ESP8266_SendCmd( "AT+CIPSTART=\"TCP\",\"mqtt.heclouds.com\",1883\r\n", "OK"); 
        vTaskDelay(pdMS_TO_TICKS(2000)); 
    } 
    printf("MQTT Connect Success\r\n"); 
    OneNET_Subscribe(); 
}
 
void CommTask(void *argument) 
{ 
    uint32_t lastSend = 0; 
    uint8_t *dataPtr = NULL; 
    CommTask_Init(); 
    MQTT_Connect(); 
    
    for(;;) 
    { 
        /* 读取队列数据 */ 
        if(xQueueReceive(CommQueue, &comm_data, pdMS_TO_TICKS(100)) == pdPASS) 
        { 
        // printf("Vol=%.3f Cur=%.3f Track=%d Sys=%d\r\n", 
        // comm_data.voltage, 
        // comm_data.current, 
        // comm_data.track_state, 
        // comm_data.system_state); 
        } 
        
        /* 每5秒上传一次 */ 
        // if(xTaskGetTickCount() - lastSend >= pdMS_TO_TICKS(5000)) 
        // { 
        // printf("Upload MQTT\r\n"); 
        // xSemaphoreTake(esp_tx_mutex, portMAX_DELAY); 
        // OneNet_SendData(); 
        // xSemaphoreGive(esp_tx_mutex); 
        // lastSend = xTaskGetTickCount(); 
        // } 
        if(xTaskGetTickCount() - lastSend >= pdMS_TO_TICKS(10000))
        { 
            printf("Upload MQTT\r\n"); 
            if(OneNet_SendData()) 
            { 
                printf("TCP Lost Reconnect\r\n"); 
                ESP8266_SendCmd("AT+CIPCLOSE\r\n","OK"); 
                vTaskDelay(pdMS_TO_TICKS(2000)); 
                MQTT_Connect(); 
            } 
            lastSend = xTaskGetTickCount(); 
        } 
        
        /* 处理云端下行 */
        dataPtr = ESP8266_GetIPD(1); 
        if(dataPtr != NULL) 
        { 
            OneNet_RevPro(dataPtr); 
        } 
        
        vTaskDelay(pdMS_TO_TICKS(10)); 
    } 
}

