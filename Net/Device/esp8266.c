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

static int ESP_FindToken(const uint8_t *buf, int len, const char *token)
{
    int tlen = strlen(token);
    if(len <= 0 || tlen <= 0 || len < tlen)
        return -1;

    for(int i = 0; i <= len - tlen; i++)
    {
        if(memcmp(&buf[i], token, tlen) == 0)
            return i;
    }

    return -1;
}

static void ESP_DebugPrintChunk(const uint8_t *buf, int len)
{
    char line[256];
    int j = 0;

    if(buf == NULL || len <= 0)
        return;

    memset(line, 0, sizeof(line));

    for(int i = 0; i < len && j < (int)sizeof(line) - 2; i++)
    {
        uint8_t c = buf[i];

        if(c == '\r')
        {
            continue;
        }
        else if(c == '\n')
        {
            if(j > 0)
            {
                line[j] = 0;
                printf("ESP: %s\r\n", line);
                j = 0;
                memset(line, 0, sizeof(line));
            }
        }
        else if(c >= 32 && c <= 126)
        {
            line[j++] = c;
        }
    }

    if(j > 0)
    {
        line[j] = 0;
        printf("ESP: %s\r\n", line);
    }
}

static void ESP8266_RxRestart(void)
{
    HAL_UART_DMAStop(&huart1);

    taskENTER_CRITICAL();
    esp_rx_read = 0;
    esp_rx_write = 0;
    memset(esp_rx_buf, 0, sizeof(esp_rx_buf));
    memset(dma_rx_buf, 0, sizeof(dma_rx_buf));
    taskEXIT_CRITICAL();

    while(xSemaphoreTake(esp_rx_semaphore, 0) == pdPASS)
    {
    }

    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, ESP_DMA_BUF_SIZE);
    __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
}

int ESP8266_Read(uint8_t *buf, int maxlen)
{
    int i = 0;

    if(buf == NULL || maxlen <= 0)
        return 0;

    taskENTER_CRITICAL();

    while((esp_rx_read != esp_rx_write) && (i < maxlen))
    {
        buf[i++] = esp_rx_buf[esp_rx_read++];

        if(esp_rx_read >= ESP_RX_BUF_SIZE)
            esp_rx_read = 0;
    }

    taskEXIT_CRITICAL();

    return i;
}

_Bool ESP8266_WaitIpReady(uint32_t timeout_ms)
{
    uint8_t recvBuf[256];
    uint8_t tmp[64];
    int recvLen = 0;
    uint32_t startTick;

    memset(recvBuf, 0, sizeof(recvBuf));

    xSemaphoreTake(esp_tx_mutex, portMAX_DELAY);

    ESP8266_RxRestart();
    HAL_UART_Transmit(&huart1, (uint8_t *)"AT+CIPSTATUS\r\n", 14, 1000);

    startTick = xTaskGetTickCount();

    while((xTaskGetTickCount() - startTick) < pdMS_TO_TICKS(timeout_ms))
    {
        int len = ESP8266_Read(tmp, sizeof(tmp));
        if(len > 0)
        {
            if(recvLen + len > (int)sizeof(recvBuf))
            {
                recvLen = 0;
                memset(recvBuf, 0, sizeof(recvBuf));
            }

            memcpy(recvBuf + recvLen, tmp, len);
            recvLen += len;

            recvBuf[recvLen] = 0;

            /*
             * 有 IP 的常见状态：
             * STATUS:2  已获得IP，已连TCP
             * STATUS:3  已建立TCP传输
             * STATUS:4  TCP断开
             * STATUS:5  未建立TCP，但STA正常
             */
            if((strstr((char *)recvBuf, "STATUS:2") ||
                strstr((char *)recvBuf, "STATUS:3") ||
                strstr((char *)recvBuf, "STATUS:4") ||
                strstr((char *)recvBuf, "STATUS:5")) &&
               (strstr((char *)recvBuf, "no ip") == NULL))
            {
                xSemaphoreGive(esp_tx_mutex);
                return 0;
            }

            if(strstr((char *)recvBuf, "no ip"))
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

void ESP8266_Clear(void)
{
    ESP8266_RxRestart();
}

_Bool ESP8266_SendCmd(char *cmd, char *res)
{
    uint8_t recvBuf[768];
    uint8_t tmp[128];
    int recvLen = 0;
    uint32_t startTick;
    _Bool isCipStart = 0;
    _Bool isCwJap = 0;

    if(cmd == NULL || res == NULL)
        return 1;

    if(strstr(cmd, "AT+CIPSTART") != NULL)
        isCipStart = 1;

    if(strstr(cmd, "AT+CWJAP") != NULL)
        isCwJap = 1;

    memset(recvBuf, 0, sizeof(recvBuf));

    xSemaphoreTake(esp_tx_mutex, portMAX_DELAY);

    ESP8266_RxRestart();
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), 1000);

    startTick = xTaskGetTickCount();

    while((xTaskGetTickCount() - startTick) < pdMS_TO_TICKS(8000))
    {
        int len = ESP8266_Read(tmp, sizeof(tmp));
        if(len > 0)
        {
            ESP_DebugPrintChunk(tmp, len);

            if(recvLen + len > (int)sizeof(recvBuf) - 1)
            {
                recvLen = 0;
                memset(recvBuf, 0, sizeof(recvBuf));
            }

            memcpy(recvBuf + recvLen, tmp, len);
            recvLen += len;
            recvBuf[recvLen] = 0;

            /* 成功条件 */
            if(ESP_FindToken(recvBuf, recvLen, res) >= 0)
            {
                xSemaphoreGive(esp_tx_mutex);
                return 0;
            }

            if(isCipStart)
            {
                if(ESP_FindToken(recvBuf, recvLen, "CONNECT") >= 0 ||
                   ESP_FindToken(recvBuf, recvLen, "ALREADY CONNECTED") >= 0)
                {
                    xSemaphoreGive(esp_tx_mutex);
                    return 0;
                }

                /*
                 * CIPSTART 期间 busy p... 很常见，不立刻判失败
                 * ERROR 也先不急着死，等到超时再说
                 */
                if(ESP_FindToken(recvBuf, recvLen, "link is not valid") >= 0 ||
                   ESP_FindToken(recvBuf, recvLen, "no ip") >= 0)
                {
                    xSemaphoreGive(esp_tx_mutex);
                    return 1;
                }
            }
            else if(isCwJap)
            {
                /*
                 * 连WiFi阶段也别被 busy 立刻打断
                 */
                if(ESP_FindToken(recvBuf, recvLen, "+CWJAP:") >= 0 ||
                   ESP_FindToken(recvBuf, recvLen, "FAIL") >= 0)
                {
                    xSemaphoreGive(esp_tx_mutex);
                    return 1;
                }
            }
            else
            {
                if(ESP_FindToken(recvBuf, recvLen, "ERROR") >= 0 ||
                   ESP_FindToken(recvBuf, recvLen, "FAIL") >= 0 ||
                   ESP_FindToken(recvBuf, recvLen, "link is not valid") >= 0)
                {
                    xSemaphoreGive(esp_tx_mutex);
                    return 1;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    xSemaphoreGive(esp_tx_mutex);
    return 1;
}

_Bool ESP8266_SendData(unsigned char *data, unsigned short len)
{
    uint8_t recvBuf[768];
    uint8_t tmp[128];
    int recvLen = 0;
    uint32_t startTick;
    char cmd[32];

    if(data == NULL || len == 0)
        return 1;

    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d\r\n", len);

    xSemaphoreTake(esp_tx_mutex, portMAX_DELAY);

    /* 第1阶段：发送 CIPSEND 命令，等 '>' */
    ESP8266_Clear();
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), 1000);

    startTick = xTaskGetTickCount();
    recvLen = 0;
    memset(recvBuf, 0, sizeof(recvBuf));

    while((xTaskGetTickCount() - startTick) < pdMS_TO_TICKS(3000))
    {
        int r = ESP8266_Read(tmp, sizeof(tmp));
        if(r > 0)
        {
            ESP_DebugPrintChunk(tmp, r);

            if(recvLen + r > (int)sizeof(recvBuf))
            {
                recvLen = 0;
                memset(recvBuf, 0, sizeof(recvBuf));
            }

            memcpy(recvBuf + recvLen, tmp, r);
            recvLen += r;

            if(ESP_FindToken(recvBuf, recvLen, ">") >= 0)
                break;

            /* 这里只判断真正的失败，不能把 busy s... 当失败 */
            if(ESP_FindToken(recvBuf, recvLen, "ERROR") >= 0 ||
               ESP_FindToken(recvBuf, recvLen, "FAIL") >= 0 ||
               ESP_FindToken(recvBuf, recvLen, "link is not valid") >= 0)
            {
                xSemaphoreGive(esp_tx_mutex);
                return 1;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if(ESP_FindToken(recvBuf, recvLen, ">") < 0)
    {
        xSemaphoreGive(esp_tx_mutex);
        return 1;
    }

    /* 第2阶段：发送真实数据，等 SEND OK */
    recvLen = 0;
    memset(recvBuf, 0, sizeof(recvBuf));

    HAL_UART_Transmit(&huart1, data, len, 2000);

    startTick = xTaskGetTickCount();

    while((xTaskGetTickCount() - startTick) < pdMS_TO_TICKS(5000))
    {
        int r = ESP8266_Read(tmp, sizeof(tmp));
        if(r > 0)
        {
            ESP_DebugPrintChunk(tmp, r);

            if(recvLen + r > (int)sizeof(recvBuf))
            {
                recvLen = 0;
                memset(recvBuf, 0, sizeof(recvBuf));
            }

            memcpy(recvBuf + recvLen, tmp, r);
            recvLen += r;

            /* 真正成功 */
            if(ESP_FindToken(recvBuf, recvLen, "SEND OK") >= 0)
            {
                xSemaphoreGive(esp_tx_mutex);
                return 0;
            }

            /*
             * 注意：
             * busy s... / Recv xxx bytes 都不是失败
             * 这是发送过程中的正常提示
             */

            /* 真正失败 */
            if(ESP_FindToken(recvBuf, recvLen, "CLOSED") >= 0 ||
               ESP_FindToken(recvBuf, recvLen, "ERROR") >= 0 ||
               ESP_FindToken(recvBuf, recvLen, "FAIL") >= 0 ||
               ESP_FindToken(recvBuf, recvLen, "link is not valid") >= 0)
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

unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{
    static uint8_t streamBuf[1024];
    static int streamLen = 0;
    static uint8_t ipdPayload[768];

    uint8_t tmp[128];

    while(timeOut--)
    {
        int r = ESP8266_Read(tmp, sizeof(tmp));
        if(r > 0)
        {
            if(streamLen + r > (int)sizeof(streamBuf))
            {
                streamLen = 0;
                memset(streamBuf, 0, sizeof(streamBuf));
            }

            memcpy(streamBuf + streamLen, tmp, r);
            streamLen += r;

            int ipdPos = ESP_FindToken(streamBuf, streamLen, "+IPD,");
            if(ipdPos >= 0)
            {
                int dataLen = 0;
                int headLen = 0;

                for(int i = ipdPos + 5; i < streamLen; i++)
                {
                    if(streamBuf[i] == ':')
                    {
                        for(int j = ipdPos + 5; j < i; j++)
                        {
                            if(streamBuf[j] < '0' || streamBuf[j] > '9')
                                goto next_round;
                            dataLen = dataLen * 10 + (streamBuf[j] - '0');
                        }

                        headLen = i - ipdPos + 1;

                        if(dataLen > 0 && dataLen < (int)sizeof(ipdPayload))
                        {
                            if(streamLen - ipdPos - headLen >= dataLen)
                            {
                                memcpy(ipdPayload, &streamBuf[ipdPos + headLen], dataLen);
                                ipdPayload[dataLen] = 0;

                                int used = ipdPos + headLen + dataLen;
                                memmove(streamBuf, streamBuf + used, streamLen - used);
                                streamLen -= used;

                                return ipdPayload;
                            }
                        }
                        break;
                    }
                }
            }
        }

next_round:
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return NULL;
}

void ESP8266_Init(void)
{
    printf("1. AT\r\n");
    while(ESP8266_SendCmd("AT\r\n", "OK"))
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    printf("2. ATE0\r\n");
    while(ESP8266_SendCmd("ATE0\r\n", "OK"))
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    printf("3. CWMODE\r\n");
    while(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK"))
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    printf("4. CIPMUX\r\n");
    while(ESP8266_SendCmd("AT+CIPMUX=0\r\n", "OK"))
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    printf("5. CWDHCP\r\n");
    while(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK"))
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* 断开旧AP，给模块一点时间 */
    ESP8266_SendCmd("AT+CWQAP\r\n", "OK");
    vTaskDelay(pdMS_TO_TICKS(1500));

    printf("6. CWJAP\r\n");
    while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP"))
    {
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    /*
     * 关键：不要一看到 GOT IP 就立刻进 TCP
     * 给 ESP8266 的 WiFi / DHCP / 内部协议栈恢复时间
     */
    vTaskDelay(pdMS_TO_TICKS(5000));

    while(ESP8266_WaitIpReady(3000))
    {
        printf("WiFi not ready, reconnect...\r\n");
        ESP8266_SendCmd("AT+CWQAP\r\n", "OK");
        vTaskDelay(pdMS_TO_TICKS(1000));

        while(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP"))
        {
            vTaskDelay(pdMS_TO_TICKS(3000));
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    printf("WiFi OK\r\n");
    ESP8266_RxRestart();
    printf("ESP8266 Init OK\r\n");
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        for(uint16_t i = 0; i < Size; i++)
        {
            uint16_t next = esp_rx_write + 1;
            if(next >= ESP_RX_BUF_SIZE)
                next = 0;

            if(next == esp_rx_read)
            {
                esp_rx_read++;
                if(esp_rx_read >= ESP_RX_BUF_SIZE)
                    esp_rx_read = 0;
            }

            esp_rx_buf[esp_rx_write] = dma_rx_buf[i];
            esp_rx_write = next;
        }

        /* 清掉 DMA 缓冲，避免旧数据残留 */
        memset(dma_rx_buf, 0, sizeof(dma_rx_buf));

        xSemaphoreGiveFromISR(esp_rx_semaphore, &xHigherPriorityTaskWoken);

        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, ESP_DMA_BUF_SIZE);
        __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

