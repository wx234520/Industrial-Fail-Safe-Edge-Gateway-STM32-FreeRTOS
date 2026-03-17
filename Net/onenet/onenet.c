#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "esp8266.h"
#include "onenet.h"
#include "mqttkit.h"
#include "base64.h"
#include "hmac_sha1.h"
#include "usart.h"
#include "typedefs.h"
#include "safety_service.h"
#include <string.h>
#include <stdio.h>
#include "cJSON.h"

#define PROID			"Y6n1JN3291"
#define ACCESS_KEY		"dVZ5bXRtdUJ2RnV5elJtUFlWRnBiMXpiSHBXUERUcTY="
#define DEVICE_NAME		"d1"


char devid[16];

char key[48];

/*
************************************************************
*	函数名称：	OTA_UrlEncode
*
*	函数功能：	sign需要进行URL编码
*
*	入口参数：	sign：加密结果
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		+			%2B
*				空格		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
static unsigned char OTA_UrlEncode(char *sign)
{

	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);
	
	if(sign == (void *)0 || sign_len < 28)
		return 1;
	
	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;
	
	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;
			
			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;
			
			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;
			
			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;
			
			case '%':
				strcat(sign + j, "%25");j += 3;
			break;
			
			case '#':
				strcat(sign + j, "%23");j += 3;
			break;
			
			case '&':
				strcat(sign + j, "%26");j += 3;
			break;
			
			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;
			
			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}
	
	sign[j] = 0;
	
	return 0;

}

/*
************************************************************
*	函数名称：	OTA_Authorization
*
*	函数功能：	计算Authorization
*
*	入口参数：	ver：参数组版本号，日期格式，目前仅支持格式"2018-10-31"
*				res：产品id
*				et：过期时间，UTC秒值
*				access_key：访问密钥
*				dev_name：设备名
*				authorization_buf：缓存token的指针
*				authorization_buf_len：缓存区长度(字节)
*
*	返回参数：	0-成功	其他-失败
*
*	说明：		当前仅支持sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//保存签名的Base64编码结果 和 URL编码结果
	char hmac_sha1_buf[64];							//保存签名
	char access_key_base64[64];						//保存access_key的Base64编码结合
	char string_for_signature[72];					//保存string_for_signature，这个是加密的key

//----------------------------------------------------参数合法性--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------将access_key进行Base64解码----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
//	printf("access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------计算string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
//	printf("string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------加密-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
//	printf("hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------将加密结果进行Base64编码------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------将Base64编码结果进行URL编码---------------------------------------------------
	OTA_UrlEncode(sign_buf);
//	printf("sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------计算Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
//	printf("Token: %s\r\n", authorization_buf);
	
	return 0;

}

//==========================================================
//	函数名称：	OneNET_RegisterDevice
//
//	函数功能：	在产品中注册一个设备
//
//	入口参数：	access_key：访问密钥
//				pro_id：产品ID
//				serial：唯一设备号
//				devid：保存返回的devid
//				key：保存返回的key
//
//	返回参数：	0-成功		1-失败
//
//	说明：		
//==========================================================
_Bool OneNET_RegisterDevice(void)
{

	_Bool result = 1;
	unsigned short send_len = 11 + strlen(DEVICE_NAME);
	char *send_ptr = NULL, *data_ptr = NULL;
	
	char authorization_buf[144];													//加密的key
	
	send_ptr = malloc(send_len + 240);
	if(send_ptr == NULL)
		return result;
	
	while(ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"183.230.40.33\",80\r\n", "CONNECT"))
		vTaskDelay(500);
	
	OneNET_Authorization("2018-10-31", PROID, 1893456000, ACCESS_KEY, NULL,
							authorization_buf, sizeof(authorization_buf), 1);
	
	snprintf(send_ptr, 280 + send_len, "POST /mqtt/v1/devices/reg HTTP/1.1\r\n"
					"Authorization:%s\r\n"
					"Host:ota.heclouds.com\r\n"
					"Content-Type:application/json\r\n"
					"Content-Length:%d\r\n\r\n"
					"{\"name\":\"%s\"}",
	
					authorization_buf, 11 + strlen(DEVICE_NAME), DEVICE_NAME);
	
	ESP8266_SendData((unsigned char *)send_ptr, strlen(send_ptr));
	
	/*
	{
	  "request_id" : "f55a5a37-36e4-43a6-905c-cc8f958437b0",
	  "code" : "onenet_common_success",
	  "code_no" : "000000",
	  "message" : null,
	  "data" : {
		"device_id" : "589804481",
		"name" : "mcu_id_43057127",
		
	"pid" : 282932,
		"key" : "indu/peTFlsgQGL060Gp7GhJOn9DnuRecadrybv9/XY="
	  }
	}
	*/
	
	data_ptr = (char *)ESP8266_GetIPD(250);							//等待平台响应
	
	if(data_ptr)
	{
		data_ptr = strstr(data_ptr, "device_id");
	}
	
	if(data_ptr)
	{
		char name[16];
		int pid = 0;
		
		if(sscanf(data_ptr, "device_id\" : \"%[^\"]\",\r\n\"name\" : \"%[^\"]\",\r\n\r\n\"pid\" : %d,\r\n\"key\" : \"%[^\"]\"", devid, name, &pid, key) == 4)
		{
			printf("create device: %s, %s, %d, %s\r\n", devid, name, pid, key);
			result = 0;
		}
	}
	
	free(send_ptr);
	ESP8266_SendCmd("AT+CIPCLOSE\r\n", "OK");
	
	return result;

}

//==========================================================
//	函数名称：	OneNet_DevLink
//
//	函数功能：	与onenet创建连接
//
//	入口参数：	无
//
//	返回参数：	1-成功	0-失败
//
//	说明：		与onenet平台建立连接
//==========================================================
_Bool OneNet_DevLink(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};
    unsigned char *dataPtr;
    char authorization_buf[160];
    _Bool status = 1;

    OneNET_Authorization("2018-10-31", PROID, 1893456000, ACCESS_KEY, DEVICE_NAME,
                         authorization_buf, sizeof(authorization_buf), 0);

    printf("OneNET_DevLink\r\n"
           "NAME: %s,\tPROID: %s,\tKEY:%s\r\n",
           DEVICE_NAME, PROID, authorization_buf);

    if(MQTT_PacketConnect(PROID, authorization_buf, DEVICE_NAME, 256, 1,
                          MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) != 0)
    {
        printf("WARN:\tMQTT_PacketConnect Failed\r\n");
        return 1;
    }

    if(ESP8266_SendData(mqttPacket._data, mqttPacket._len) != 0)
    {
        printf("WARN:\tESP8266_SendData CONNECT packet failed\r\n");
        MQTT_DeleteBuffer(&mqttPacket);
        return 1;
    }

    MQTT_DeleteBuffer(&mqttPacket);

    dataPtr = ESP8266_GetIPD(300);
    if(dataPtr == NULL)
    {
        printf("WARN:\tNo CONNACK from server\r\n");
        return 1;
    }

    if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
    {
        switch(MQTT_UnPacketConnectAck(dataPtr))
        {
            case 0:
                printf("Tips:\tConnection successful\r\n");
                status = 0;
                break;
            case 1:
                printf("WARN:\tConnection failed: Protocol error\r\n");
                break;
            case 2:
                printf("WARN:\tConnection failed: Invalid clientid\r\n");
                break;
            case 3:
                printf("WARN:\tConnection failed: Server error\r\n");
                break;
            case 4:
                printf("WARN:\tConnection failed: Wrong username or password\r\n");
                break;
            case 5:
                printf("WARN:\tConnection failed: Token invalid\r\n");
                break;
            default:
                printf("ERR:\tConnection failed: Unknown error\r\n");
                break;
        }
    }
    else
    {
        printf("WARN:\tServer reply is not CONNACK\r\n");
    }

    return status;
}

//==========================================================
//	函数名称：	OneNet_SendData
//
//	函数功能：	上传数据到平台
//
//	入口参数：	type：发送数据的格式
//
//	返回参数：	无
//
//	说明：		
//==========================================================
extern system_data_t comm_data;
uint8 OneNet_FillBuf_1(char *buf)
{
    char text[64];
    alarm_threshold_t threshold;

    memset(text, 0, sizeof(text));
    SafetyService_GetThreshold(&threshold);

    strcpy(buf, "{\"id\":\"123\",\"params\":{");

    memset(text, 0, sizeof(text));
    sprintf(text, "\"Vol\":{\"value\":%.3f},", comm_data.voltage);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"Cur\":{\"value\":%.3f},", comm_data.current);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"Temp\":{\"value\":%d},", comm_data.temperature);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"Humi\":{\"value\":%d},", comm_data.humidity);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"TrackState\":{\"value\":%d},", comm_data.track_state);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"SymState\":{\"value\":%d},", comm_data.system_state);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"VolLowWarn\":{\"value\":%.3f},", threshold.voltage_low_warning);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"VolHighWarn\":{\"value\":%.3f}", threshold.voltage_high_warning);
    strcat(buf, text);

    strcat(buf, "}}");

    return strlen(buf);
}

uint8 OneNet_FillBuf_2(char *buf)
{
    char text[64];
    alarm_threshold_t threshold;

    memset(text, 0, sizeof(text));
    SafetyService_GetThreshold(&threshold);

    strcpy(buf, "{\"id\":\"123\",\"params\":{");

    memset(text, 0, sizeof(text));
    sprintf(text, "\"CurHighWarn\":{\"value\":%.3f},", threshold.current_high_warning);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"TempHighWarn\":{\"value\":%d},", threshold.temp_high_warning);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"HumiLowWarn\":{\"value\":%d},", threshold.humi_low_warning);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"HumiHighWarn\":{\"value\":%d},", threshold.humi_high_warning);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"VolHyst\":{\"value\":%.2f},", threshold.vol_hysteresis);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"CurHyst\":{\"value\":%.2f},", threshold.cur_hysteresis);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"TempHyst\":{\"value\":%d},", threshold.temp_hysteresis);
    strcat(buf, text);

    memset(text, 0, sizeof(text));
    sprintf(text, "\"HumiHyst\":{\"value\":%d}", threshold.humi_hysteresis);
    strcat(buf, text);

    strcat(buf, "}}");

    return strlen(buf);
}

int OneNet_SendData(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};
    char buf[512];
    short body_len;
    static uint8_t upload_index = 0;

    memset(buf, 0, sizeof(buf));
    
    if(upload_index == 0)
    {
        body_len = OneNet_FillBuf_1(buf);
        upload_index = 1;
    }
    else
    {
        body_len = OneNet_FillBuf_2(buf);
        upload_index = 0;
    }

    if(body_len <= 0)
        return 1;

    printf("MQTT JSON: %s\r\n", buf);

    if(MQTT_PacketSaveData(PROID, DEVICE_NAME, body_len, NULL, &mqttPacket) != 0)
        return 1;

    memcpy(&mqttPacket._data[mqttPacket._len], buf, body_len);
    mqttPacket._len += body_len;

    if(ESP8266_SendData(mqttPacket._data, mqttPacket._len) != 0)
    {
        MQTT_DeleteBuffer(&mqttPacket);
        return 1;
    }

    MQTT_DeleteBuffer(&mqttPacket);
    return 0;
}

//==========================================================
//	函数名称：	OneNET_Publish
//
//	函数功能：	发布消息
//
//	入口参数：	topic：发布的主题
//				msg：消息内容
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Publish(const char *topic, const char *msg)
{
    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};

    printf("Publish Topic: %s, Msg: %s\r\n", topic, msg);

    if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg),
                          MQTT_QOS_LEVEL0, 0, 0, &mqtt_packet) == 0)
    {
        if(ESP8266_SendData(mqtt_packet._data, mqtt_packet._len) != 0)
        {
            printf("WARN:\tPublish send failed\r\n");
        }

        MQTT_DeleteBuffer(&mqtt_packet);
    }
}

//==========================================================
//	函数名称：	OneNET_Subscribe
//
//	函数功能：	订阅
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNET_Subscribe(void)
{
    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};
    char topic_buf[64];
    const char *topic = topic_buf;

    snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set", PROID, DEVICE_NAME);

    printf("Subscribe Topic: %s\r\n", topic_buf);

    if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
    {
        if(ESP8266_SendData(mqtt_packet._data, mqtt_packet._len) != 0)
        {
            printf("WARN:\tSubscribe send failed\r\n");
        }

        MQTT_DeleteBuffer(&mqtt_packet);
    }
}

static int OneNet_GetNumberFromParam(cJSON *params, const char *key, float *out_val)
{
    cJSON *item = NULL;
    cJSON *value = NULL;

    if(params == NULL || key == NULL || out_val == NULL)
        return -1;

    item = cJSON_GetObjectItem(params, key);
    if(item == NULL)
        return -1;

    /* 情况1：平台直接下发数字，如 "CurHighWarn": 7 */
    if(item->type == cJSON_Number)
    {
        *out_val = (float)item->valuedouble;
        return 0;
    }

    /* 情况2：平台下发对象，如 "CurHighWarn": {"value":7} */
    value = cJSON_GetObjectItem(item, "value");
    if(value != NULL && value->type == cJSON_Number)
    {
        *out_val = (float)value->valuedouble;
        return 0;
    }

    return -1;
}

static void OneNet_ParseThresholdParams(cJSON *params)
{
    alarm_threshold_t threshold;
    uint8_t threshold_changed = 0;
    float val = 0.0f;

    if(params == NULL)
        return;

    SafetyService_GetThreshold(&threshold);

    if(OneNet_GetNumberFromParam(params, "VolLowWarn", &val) == 0)
    {
        threshold.voltage_low_warning = val;
        threshold_changed = 1;
        printf("Set VolLowWarn = %.2f\r\n", threshold.voltage_low_warning);
    }

    if(OneNet_GetNumberFromParam(params, "VolHighWarn", &val) == 0)
    {
        threshold.voltage_high_warning = val;
        threshold_changed = 1;
        printf("Set VolHighWarn = %.2f\r\n", threshold.voltage_high_warning);
    }

    if(OneNet_GetNumberFromParam(params, "CurHighWarn", &val) == 0)
    {
        threshold.current_high_warning = val;
        threshold_changed = 1;
        printf("Set CurHighWarn = %.2f\r\n", threshold.current_high_warning);
    }

    if(OneNet_GetNumberFromParam(params, "TempHighWarn", &val) == 0)
    {
        threshold.temp_high_warning = val;
        threshold_changed = 1;
        printf("Set TempHighWarn = %d\r\n", threshold.temp_high_warning);
    }

    if(OneNet_GetNumberFromParam(params, "HumiLowWarn", &val) == 0)
    {
        threshold.humi_low_warning = val;
        threshold_changed = 1;
        printf("Set HumiLowWarn = %d\r\n", threshold.humi_low_warning);
    }

    if(OneNet_GetNumberFromParam(params, "HumiHighWarn", &val) == 0)
    {
        threshold.humi_high_warning = val;
        threshold_changed = 1;
        printf("Set HumiHighWarn = %d\r\n", threshold.humi_high_warning);
    }

    if(OneNet_GetNumberFromParam(params, "VolHyst", &val) == 0)
    {
        threshold.vol_hysteresis = val;
        threshold_changed = 1;
        printf("Set VolHyst = %.2f\r\n", threshold.vol_hysteresis);
    }

    if(OneNet_GetNumberFromParam(params, "CurHyst", &val) == 0)
    {
        threshold.cur_hysteresis = val;
        threshold_changed = 1;
        printf("Set CurHyst = %.2f\r\n", threshold.cur_hysteresis);
    }

    if(OneNet_GetNumberFromParam(params, "TempHyst", &val) == 0)
    {
        threshold.temp_hysteresis = val;
        threshold_changed = 1;
        printf("Set TempHyst = %d\r\n", threshold.temp_hysteresis);
    }

    if(OneNet_GetNumberFromParam(params, "HumiHyst", &val) == 0)
    {
        threshold.humi_hysteresis = val;
        threshold_changed = 1;
        printf("Set HumiHyst = %d\r\n", threshold.humi_hysteresis);
    }

    if(threshold_changed)
    {
        SafetyService_SetThreshold(&threshold);
        printf("Threshold updated from cloud\r\n");
    }
}

static void OneNET_SetReply(cJSON *id_item, int code)
{
    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};
    char topic_buf[64];
    char msg_buf[64];
    static uint16_t reply_pkt_id = 1;
    int ret;

    if(id_item == NULL)
        return;

    snprintf(topic_buf, sizeof(topic_buf),
             "$sys/%s/%s/thing/property/set_reply",
             PROID, DEVICE_NAME);

    /* 同时兼容 string 和 number 两种 id */
    if(id_item->type == cJSON_String && id_item->valuestring != NULL)
    {
        snprintf(msg_buf, sizeof(msg_buf),
                 "{\"id\":\"%s\",\"code\":%d}",
                 id_item->valuestring, code);
    }
    else if(id_item->type == cJSON_Number)
    {
        snprintf(msg_buf, sizeof(msg_buf),
                 "{\"id\":%d,\"code\":%d}",
                 id_item->valueint, code);
    }
    else
    {
        printf("WARN:\tset_reply id type invalid\r\n");
        return;
    }

    printf("SetReply Topic: %s\r\n", topic_buf);
    printf("SetReply Msg  : %s\r\n", msg_buf);

    /* 先用 QoS0，保持最简单，避免 QoS1/packet id 干扰排查 */
    ret = MQTT_PacketPublish(reply_pkt_id++,
                             topic_buf,
                             msg_buf,
                             strlen(msg_buf),
                             MQTT_QOS_LEVEL0,
                             0,
                             0,
                             &mqtt_packet);

    if(ret == 0)
    {
        if(ESP8266_SendData(mqtt_packet._data, mqtt_packet._len) != 0)
        {
            printf("WARN:\tSetReply send failed\r\n");
        }
        else
        {
            printf("SetReply send ok\r\n");
        }

        MQTT_DeleteBuffer(&mqtt_packet);
    }
    else
    {
        printf("WARN:\tMQTT_PacketPublish set_reply failed\r\n");
    }
}

//==========================================================
//	函数名称：	OneNet_RevPro
//
//	函数功能：	平台返回数据检测
//
//	入口参数：	dataPtr：平台返回的数据
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
    char *req_payload = NULL;
    char *cmdid_topic = NULL;

    unsigned short topic_len = 0;
    unsigned short req_len = 0;
    unsigned char qos = 0;
    static unsigned short pkt_id = 0;

    unsigned char type = 0;
    short result = 0;

    cJSON *raw_json = NULL;

    type = MQTT_UnPacketRecv(cmd);

    switch(type)
    {
        case MQTT_PKT_PUBLISH:
        {
            cJSON *id_item = NULL;
            cJSON *params = NULL;

            result = MQTT_UnPacketPublish(cmd,
                                          &cmdid_topic,
                                          &topic_len,
                                          &req_payload,
                                          &req_len,
                                          &qos,
                                          &pkt_id);

            if(result == 0)
            {
                printf("topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
                       cmdid_topic, topic_len, req_payload, req_len);

                raw_json = cJSON_Parse(req_payload);
                if(raw_json != NULL)
                {
                    id_item = cJSON_GetObjectItem(raw_json, "id");
                    params  = cJSON_GetObjectItem(raw_json, "params");

                    /* 单独函数处理阈值下发 */
                    OneNet_ParseThresholdParams(params);

                    if(id_item != NULL)
                    {
                        OneNET_SetReply(id_item, 200);
                    }
                    else
                    {
                        printf("WARN:\tset payload has no valid id\r\n");
                    }

                    cJSON_Delete(raw_json);
                }
                else
                {
                    printf("WARN:\tcJSON_Parse failed\r\n");
                }
            }
            break;
        }

        case MQTT_PKT_PUBACK:
        {
            if(MQTT_UnPacketPublishAck(cmd) == 0)
                printf("Tips:\tMQTT Publish Send OK\r\n");
            break;
        }

        case MQTT_PKT_SUBACK:
        {
            if(MQTT_UnPacketSubscribe(cmd) == 0)
                printf("Tips:\tMQTT Subscribe OK\r\n");
            else
                printf("Tips:\tMQTT Subscribe Err\r\n");
            break;
        }

        default:
        {
            result = -1;
            break;
        }
    }

    if(result == -1)
        return;

    if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
    {
        MQTT_FreeBuffer(cmdid_topic);
        MQTT_FreeBuffer(req_payload);
    }
}
