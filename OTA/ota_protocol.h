#ifndef __OTA_PROTOCOL_H
#define __OTA_PROTOCOL_H

#include "stm32f10x.h"

#define OTA_PACKET_SIZE 128
//#define OTA_IDLE       0x00000000
//#define OTA_RECEIVING  0xAAAAAAAA
//#define OTA_SUCCESS    0x55555555
//#define OTA_FAILED     0xFFFFFFFF

typedef struct
{
    uint16_t seq;
    uint16_t len;
    uint8_t data[OTA_PACKET_SIZE];
    uint16_t crc;
}OTA_DataPacket_t;

typedef struct
{
    uint32_t file_size;    //固件大小
    uint32_t crc32;        //整包CRC
    uint8_t version[16];   //版本号
}OTA_StartPacket_t;

typedef enum
{
    OTA_IDLE = 0,        //等待升级
    OTA_RECEIVING,       //接收固件
    OTA_DONE,
    OTA_ERROR
}OTA_State_t;

typedef struct
{
    uint32_t crc32;
}OTA_EndPacket_t;

void OTA_Data_Process(OTA_DataPacket_t *packet);
uint16_t OTA_CRC16(uint8_t *data,uint16_t len);
void OTA_Start_Process(OTA_StartPacket_t *start);
void OTA_SetState(OTA_State_t state);
OTA_State_t OTA_GetState(void);
void OTA_End_Process(OTA_EndPacket_t *end);

#endif
