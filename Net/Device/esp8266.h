#ifndef __ESP8266_H
#define __ESP8266_H

#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志

_Bool ESP8266_SendCmd(char *cmd, char *res);

_Bool ESP8266_WaitIpReady(uint32_t timeout_ms);

_Bool ESP8266_SendData(uint8_t *data, uint16_t len);

uint8_t *ESP8266_GetIPD(uint16_t timeOut);

void ESP8266_Clear(void);

void ESP8266_Init(void);

#endif
