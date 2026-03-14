#ifndef _MQTTKIT_H_
#define _MQTTKIT_H_


#include "Common.h"


//=============================����==============================
//===========�����ṩRTOS���ڴ����������Ҳ����ʹ��C���=========
//RTOS
#include <stdlib.h>

#define MQTT_MallocBuffer	malloc

#define MQTT_FreeBuffer		free
//==========================================================


#define MOSQ_MSB(A)         (uint8)((A & 0xFF00) >> 8)
#define MOSQ_LSB(A)         (uint8)(A & 0x00FF)


/*--------------------------------�ڴ���䷽����־--------------------------------*/
#define MEM_FLAG_NULL		0
#define MEM_FLAG_ALLOC		1
#define MEM_FLAG_STATIC		2


typedef struct Buffer
{
	
	uint8	*_data;		//Э������
	
	uint32	_len;		//д������ݳ���
	
	uint32	_size;		//�����ܴ�С
	
	uint8	_memFlag;	//�ڴ�ʹ�õķ�����0-δ����	1-ʹ�õĶ�̬����		2-ʹ�õĹ̶��ڴ�
	
} MQTT_PACKET_STRUCTURE;


/*--------------------------------�̶�ͷ����Ϣ����--------------------------------*/
enum MqttPacketType
{
	
    MQTT_PKT_CONNECT = 1, /**< �����������ݰ� */
    MQTT_PKT_CONNACK,     /**< ����ȷ�����ݰ� */
    MQTT_PKT_PUBLISH,     /**< �����������ݰ� */
    MQTT_PKT_PUBACK,      /**< ����ȷ�����ݰ� */
    MQTT_PKT_PUBREC,      /**< ���������ѽ������ݰ���Qos 2ʱ���ظ�MQTT_PKT_PUBLISH */
    MQTT_PKT_PUBREL,      /**< ���������ͷ����ݰ��� Qos 2ʱ���ظ�MQTT_PKT_PUBREC */
    MQTT_PKT_PUBCOMP,     /**< ����������ݰ��� Qos 2ʱ���ظ�MQTT_PKT_PUBREL */
    MQTT_PKT_SUBSCRIBE,   /**< �������ݰ� */
    MQTT_PKT_SUBACK,      /**< ����ȷ�����ݰ� */
    MQTT_PKT_UNSUBSCRIBE, /**< ȡ���������ݰ� */
    MQTT_PKT_UNSUBACK,    /**< ȡ������ȷ�����ݰ� */
    MQTT_PKT_PINGREQ,     /**< ping ���ݰ� */
    MQTT_PKT_PINGRESP,    /**< ping ��Ӧ���ݰ� */
    MQTT_PKT_DISCONNECT,  /**< �Ͽ��������ݰ� */
	
	//����
	
	MQTT_PKT_CMD  		 /**< �����·����ݰ� */
	
};


/*--------------------------------MQTT QOS�ȼ�--------------------------------*/
enum MqttQosLevel
{
	
    MQTT_QOS_LEVEL0,  /**< ��෢��һ�� */
    MQTT_QOS_LEVEL1,  /**< ���ٷ���һ��  */
    MQTT_QOS_LEVEL2   /**< ֻ����һ�� */
	
};


/*--------------------------------MQTT ���������־λ���ڲ�ʹ��--------------------------------*/
enum MqttConnectFlag
{
	
    MQTT_CONNECT_CLEAN_SESSION  = 0x02,
    MQTT_CONNECT_WILL_FLAG      = 0x04,
    MQTT_CONNECT_WILL_QOS0      = 0x00,
    MQTT_CONNECT_WILL_QOS1      = 0x08,
    MQTT_CONNECT_WILL_QOS2      = 0x10,
    MQTT_CONNECT_WILL_RETAIN    = 0x20,
    MQTT_CONNECT_PASSORD        = 0x40,
    MQTT_CONNECT_USER_NAME      = 0x80
	
};


/*--------------------------------��Ϣ��packet ID�����Զ���--------------------------------*/
#define MQTT_PUBLISH_ID			10

#define MQTT_SUBSCRIBE_ID		20

#define MQTT_UNSUBSCRIBE_ID		30


/*--------------------------------ɾ��--------------------------------*/
void MQTT_DeleteBuffer(MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------���--------------------------------*/
uint8 MQTT_UnPacketRecv(uint8 *dataPtr);

/*--------------------------------��¼���--------------------------------*/
uint8 MQTT_PacketConnect(const int8 *user, const int8 *password, const int8 *devid,
						uint16 cTime, uint1 clean_session, uint1 qos,
						const int8 *will_topic, const int8 *will_msg, int32 will_retain,
						MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------�Ͽ��������--------------------------------*/
uint1 MQTT_PacketDisConnect(MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------������Ӧ���--------------------------------*/
uint8 MQTT_UnPacketConnectAck(uint8 *rev_data);

/*--------------------------------���ݵ��ϴ����--------------------------------*/
uint1 MQTT_PacketSaveData(const int8 *pro_id, const char *dev_name,
								int16 send_len, int8 *type_bin_head, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------�������ļ��ϴ����--------------------------------*/
uint1 MQTT_PacketSaveBinData(const int8 *name, int16 file_len, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------�����·����--------------------------------*/
uint8 MQTT_UnPacketCmd(uint8 *rev_data, int8 **cmdid, int8 **req, uint16 *req_len);

/*--------------------------------����ظ����--------------------------------*/
uint1 MQTT_PacketCmdResp(const int8 *cmdid, const int8 *req, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------�����������--------------------------------*/
uint8 MQTT_PacketSubscribe(uint16 pkt_id, enum MqttQosLevel qos, const int8 *topics[], uint8 topics_cnt, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------��������ظ����--------------------------------*/
uint8 MQTT_UnPacketSubscribe(uint8 *rev_data);

/*--------------------------------ȡ���������--------------------------------*/
uint8 MQTT_PacketUnSubscribe(uint16 pkt_id, const int8 *topics[], uint8 topics_cnt, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------ȡ�����Ļظ����--------------------------------*/
uint1 MQTT_UnPacketUnSubscribe(uint8 *rev_data);

/*--------------------------------�����������--------------------------------*/
uint8 MQTT_PacketPublish(uint16 pkt_id, const int8 *topic,
						const int8 *payload, uint32 payload_len,
						enum MqttQosLevel qos, int32 retain, int32 own,
						MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------������Ϣ�ظ����--------------------------------*/
uint8 MQTT_UnPacketPublish(uint8 *rev_data, int8 **topic, uint16 *topic_len, int8 **payload, uint16 *payload_len, uint8 *qos, uint16 *pkt_id);

/*--------------------------------������Ϣ��Ack���--------------------------------*/
uint1 MQTT_PacketPublishAck(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------������Ϣ��Ack���--------------------------------*/
uint1 MQTT_UnPacketPublishAck(uint8 *rev_data);

/*--------------------------------������Ϣ��Rec���--------------------------------*/
uint1 MQTT_PacketPublishRec(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------������Ϣ��Rec���--------------------------------*/
uint1 MQTT_UnPacketPublishRec(uint8 *rev_data);

/*--------------------------------������Ϣ��Rel���--------------------------------*/
uint1 MQTT_PacketPublishRel(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------������Ϣ��Rel���--------------------------------*/
uint1 MQTT_UnPacketPublishRel(uint8 *rev_data, uint16 pkt_id);

/*--------------------------------������Ϣ��Comp���--------------------------------*/
uint1 MQTT_PacketPublishComp(uint16 pkt_id, MQTT_PACKET_STRUCTURE *mqttPacket);

/*--------------------------------������Ϣ��Comp���--------------------------------*/
uint1 MQTT_UnPacketPublishComp(uint8 *rev_data);

/*--------------------------------�����������--------------------------------*/
uint1 MQTT_PacketPing(MQTT_PACKET_STRUCTURE *mqttPacket);


#endif
