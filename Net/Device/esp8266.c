#include "main.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "esp8266.h"
#include "usart.h"
#include "driver_oled.h"
#include <string.h>
#include <stdio.h>


#define ESP8266_WIFI_INFO		"AT+CWJAP=\"3-201\",\"282537137234zhi\"\r\n"
#define ESP_RX_BUF_SIZE 2048
#define ESP_DMA_BUF_SIZE 256

// extern uint8_t huart2_rev_byte;
extern SemaphoreHandle_t esp_rx_semaphore;
extern SemaphoreHandle_t esp_buf_mutex;
extern SemaphoreHandle_t esp_tx_mutex;

volatile uint16_t esp_rx_write = 0;
volatile uint16_t esp_rx_read = 0;
uint8_t dma_rx_buf[ESP_DMA_BUF_SIZE];
uint8_t esp_rx_buf[ESP_RX_BUF_SIZE];

int ESP8266_Read(char *buf,int maxlen)
{
    int i=0;

    xSemaphoreTake(esp_buf_mutex,portMAX_DELAY);

    while(esp_rx_read != esp_rx_write)
    {
        buf[i++] = esp_rx_buf[esp_rx_read++];

        if(esp_rx_read >= ESP_RX_BUF_SIZE)
            esp_rx_read = 0;

        if(i >= maxlen-1)
            break;
    }

    xSemaphoreGive(esp_buf_mutex);

    buf[i] = 0;

    return i;
}

// void ESP8266_Clear(void)
// {
//     xSemaphoreTake(esp_buf_mutex,portMAX_DELAY);

//     memset(esp_rx_buf,0,sizeof(esp_rx_buf));
//     esp_rx_write = 0;
//     esp_rx_read  = 0;

//     xSemaphoreGive(esp_buf_mutex);
// }

void ESP8266_Clear(void)
{
    xSemaphoreTake(esp_buf_mutex,portMAX_DELAY);

    memset(esp_rx_buf,0,sizeof(esp_rx_buf));
	esp_rx_read = 0;
	esp_rx_write = 0;

    xSemaphoreGive(esp_buf_mutex);
}

_Bool ESP8266_WaitRecive(void)
{
	if(xSemaphoreTake(esp_rx_semaphore,pdMS_TO_TICKS(3000)) == pdPASS)
	{
		// 等待信号量（中断收到完整帧后释放）
		return REV_OK;							    //返回接收完成标志
	}	
	return REV_WAIT;								//超时返回接收未完成标志
}

// _Bool ESP8266_SendCmd(char *cmd, char *res)
// {
//     char recvBuf[512];
//     int recvLen = 0;

//     uint32_t tickStart = xTaskGetTickCount();

//     ESP8266_Clear();

//     HAL_UART_Transmit(&huart1,(uint8_t*)cmd,strlen(cmd),1000);

//     while((xTaskGetTickCount() - tickStart) < pdMS_TO_TICKS(3000))
//     {
//         int len = ESP8266_Read(recvBuf + recvLen, sizeof(recvBuf) - recvLen - 1);

//         if(len > 0)
//         {
//             recvLen += len;
//             recvBuf[recvLen] = 0;

//             printf("ESP RX: %s\r\n", recvBuf);

//             if(strstr(recvBuf, res))
//             {
//                 return 0;
//             }

//             if(strstr(recvBuf, "ERROR"))
//             {
//                 return 1;
//             }
//         }

//         vTaskDelay(pdMS_TO_TICKS(10));
//     }

//     return 1;
// }

_Bool ESP8266_SendCmd(char *cmd, char *res)
{
    static char recvBuf[512];
    int recvLen = 0;

	memset(recvBuf,0,sizeof(recvBuf));

    uint32_t tick = xTaskGetTickCount();

    xSemaphoreTake(esp_tx_mutex, portMAX_DELAY);

    ESP8266_Clear();

    HAL_UART_Transmit(&huart1,(uint8_t*)cmd,strlen(cmd),1000);

    while((xTaskGetTickCount() - tick) < pdMS_TO_TICKS(3000))
    {
        int len = ESP8266_Read(recvBuf + recvLen,
                               sizeof(recvBuf) - recvLen - 1);

        if(len > 0)
        {
            recvLen += len;
            recvBuf[recvLen] = 0;

            printf("ESP RX: %s",recvBuf + recvLen - len);

            if(strstr(recvBuf,res))
            {
                xSemaphoreGive(esp_tx_mutex);
                return 0;
            }

            if(strstr(recvBuf,"ERROR"))
            {
                xSemaphoreGive(esp_tx_mutex);
                return 1;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    xSemaphoreGive(esp_tx_mutex);

    return 1;
}


// void ESP8266_SendData(unsigned char *data, unsigned short len)
// {
//     char cmdBuf[32];

//     ESP8266_Clear();

//     sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);

//     if(!ESP8266_SendCmd(cmdBuf, ">"))
//     {
// 		printf("SendData Start\r\n");
//         HAL_UART_Transmit(&huart1, data, len, 1000);
// 		printf("SendData End\r\n");
//         ESP8266_WaitRecive();
//     }
// 	else
//     {
//         printf("CIPSEND Fail\r\n");
//     }
// }

void ESP8266_SendData(unsigned char *data, unsigned short len) 
{ 
	char cmd[32]; 
	char buf[128]; 
	
	uint32_t tick; 
	
	xSemaphoreTake(esp_tx_mutex, portMAX_DELAY); 
	sprintf(cmd,"AT+CIPSEND=%d\r\n",len); 
	
	ESP8266_Clear(); 
	HAL_UART_Transmit(&huart1,(uint8_t*)cmd,strlen(cmd),1000); 
	tick = xTaskGetTickCount(); 
	
	/* 等待 > */ 
	while((xTaskGetTickCount() - tick) < pdMS_TO_TICKS(3000)) 
	{ 
		int r = ESP8266_Read(buf,sizeof(buf)); 
		if(r > 0) 
		{ 
			if(strstr(buf,">")) break; 
		} 
		vTaskDelay(pdMS_TO_TICKS(10)); 
	} 
	
	HAL_UART_Transmit(&huart1,data,len,1000); 
	tick = xTaskGetTickCount(); 
	
	/* 等待 SEND OK */ 
	while((xTaskGetTickCount() - tick) < pdMS_TO_TICKS(3000)) 
	{ 
		int r = ESP8266_Read(buf,sizeof(buf)); 
		
		if(r > 0) 
		{ 
			if(strstr(buf,"SEND OK")) break; 
		} 
		
		vTaskDelay(pdMS_TO_TICKS(10)); 
	} 
	xSemaphoreGive(esp_tx_mutex); 
}


// unsigned char *ESP8266_GetIPD(unsigned short timeOut)
// {
//     static char recvBuf[1024];
//     static int recvLen = 0;

//     char *ptrIPD;
//     char *ptrColon;

//     while(timeOut--)
//     {
//         int len = ESP8266_Read(recvBuf + recvLen, sizeof(recvBuf) - recvLen);

//         if(len > 0)
//         {
//             recvLen += len;
//             recvBuf[recvLen] = 0;

//             ptrIPD = strstr(recvBuf, "+IPD,");

//             if(ptrIPD)
//             {
//                 ptrColon = strchr(ptrIPD, ':');

//                 if(ptrColon)
//                 {
//                     return (unsigned char*)(ptrColon + 1);
//                 }
//             }

//             if(recvLen > 900)
//                 recvLen = 0;
//         }

//         vTaskDelay(pdMS_TO_TICKS(10));
//     }

//     return NULL;
// }

unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{
    static char buf[1024];
    static int len = 0;

    char *ipd;
    int dataLen;

    while(timeOut--)
    {
        int r = ESP8266_Read(buf + len,sizeof(buf) - len - 1);

        if(r > 0)
        {
            len += r;
            buf[len] = 0;

            ipd = strstr(buf,"+IPD,");

            if(ipd)
            {
                sscanf(ipd,"+IPD,%d:",&dataLen);

                char *data = strchr(ipd,':');

                if(data)
                {
                    data++;

                    if((buf + len) - data >= dataLen)
                    {
                        return (unsigned char*)data;
                    }
                }
            }

            if(len > 900)
                len = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return NULL;
}

// void ESP8266_Init(void)
// {	
// 	ESP8266_Clear();
	
// 	printf("1. AT\r\n"); 
// 	while(ESP8266_SendCmd("AT\r\n", "OK"))
// 	vTaskDelay(pdMS_TO_TICKS(500));	

// 	printf("2. CWMODE\r\n");
// 	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
// 	vTaskDelay(pdMS_TO_TICKS(500));

// 	printf("3. CIPMUX\r\n");
// 	while(ESP8266_SendCmd("AT+CIPMUX=0\r\n","OK"))
//     vTaskDelay(pdMS_TO_TICKS(500));
	
// 	printf("4. AT+CWDHCP\r\n");
// 	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
// 	vTaskDelay(pdMS_TO_TICKS(500));
	
// 	printf("5. CWJAP\r\n");
// 	while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP"))
// 	vTaskDelay(pdMS_TO_TICKS(2000));
	
// 	printf("ESP8266 Init OK\r\n");
// 	vTaskDelay(pdMS_TO_TICKS(3000));
// }

void ESP8266_Init(void) 
{ 
	printf("1. AT\r\n"); 
	while(ESP8266_SendCmd("AT\r\n","OK")); 
	
	printf("2. CWMODE\r\n"); 
	while(ESP8266_SendCmd("AT+CWMODE=1\r\n","OK")); 
	
	printf("3. CIPMUX\r\n"); 
	while(ESP8266_SendCmd("AT+CIPMUX=0\r\n","OK")); 
	
	printf("4. CWDHCP\r\n"); 
	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n","OK")); 
	
	printf("5. CWJAP\r\n"); 
	while(ESP8266_SendCmd(ESP8266_WIFI_INFO,"GOT IP")); 
	printf("WiFi OK\r\n"); 
	
	ESP8266_Clear(); 
	vTaskDelay(pdMS_TO_TICKS(1000)); 
	
	printf("ESP8266 Init OK\r\n"); 
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        for(uint16_t i=0;i<Size;i++)
        {
            esp_rx_buf[esp_rx_write++] = dma_rx_buf[i];

            if(esp_rx_write >= ESP_RX_BUF_SIZE)
                esp_rx_write = 0;
        }

        xSemaphoreGiveFromISR(esp_rx_semaphore,&xHigherPriorityTaskWoken);

        HAL_UARTEx_ReceiveToIdle_DMA(&huart1,dma_rx_buf,ESP_DMA_BUF_SIZE);

        __HAL_DMA_DISABLE_IT(huart1.hdmarx,DMA_IT_HT);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

