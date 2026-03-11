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

// extern uint8_t huart2_rev_byte;
extern SemaphoreHandle_t esp_rx_semaphore;
extern SemaphoreHandle_t esp_buf_mutex;

uint8_t esp8266_buf[256];
uint16_t esp8266_cnt = 0, esp8266_cntPre = 0;


//==========================================================
//	函数名称：	ESP8266_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Clear(void)
{
	xSemaphoreTake(esp_buf_mutex,portMAX_DELAY);
	
	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;
	
	xSemaphoreGive(esp_buf_mutex);
}

//==========================================================
//	函数名称：	ESP8266_WaitRecive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(xSemaphoreTake(esp_rx_semaphore,pdMS_TO_TICKS(3000)) == pdPASS)
	{
		// 等待信号量（中断收到完整帧后释放）
		return REV_OK;							    //返回接收完成标志
	}	
	return REV_WAIT;								//超时返回接收未完成标志
}

//==========================================================
//	函数名称：	ESP8266_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	ESP8266_Clear();
	
	HAL_UART_Transmit(&huart1,(uint8_t*)cmd,strlen(cmd),500);

    uint8_t timeOut = 50;

    while(timeOut--)
    {
        if(ESP8266_WaitRecive() == REV_OK)
        {
            if(strstr((char *)esp8266_buf, res) != NULL)
            {
                ESP8266_Clear();
                return 0;
            }

            if(strstr((char *)esp8266_buf, "ERROR") != NULL)
            {
                ESP8266_Clear();
                return 1;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return 1;
}

//==========================================================
//	函数名称：	ESP8266_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	ESP8266_Clear();								//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
        HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 500);
	}

}

//==========================================================
//	函数名称：	ESP8266_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP8266的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//搜索“IPD”头
			if(ptrIPD == NULL)											//如果没找到，可能是IPD头的延迟，还是需要等待一会，但不会超过设定的时间
			{
				//printf("\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//找到':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
				
			}
		}
		vTaskDelay(pdMS_TO_TICKS(5));													//延时等待
	} while(timeOut--);
	
	return NULL;														//超时还未找到，返回空指针

}

//==========================================================
//	函数名称：	ESP8266_Init
//
//	函数功能：	初始化ESP8266
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP8266_Init(void)
{
	ESP8266_Clear();
	
	printf("1. AT\r\n"); 
	while(ESP8266_SendCmd("AT\r\n", "OK"))
	vTaskDelay(pdMS_TO_TICKS(500));	

	printf("2. CWMODE\r\n");
	while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
	vTaskDelay(pdMS_TO_TICKS(500));
	
	printf("3. AT+CWDHCP\r\n");
	while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
	vTaskDelay(pdMS_TO_TICKS(500));
	
	printf("4. CWJAP\r\n");
	while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP"))
	vTaskDelay(pdMS_TO_TICKS(500));
	
	printf("5. ESP8266 Init OK\r\n");
	vTaskDelay(pdMS_TO_TICKS(500));

}
