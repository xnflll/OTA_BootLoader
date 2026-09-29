#ifndef __OTA_INFO_H
#define __OTA_INFO_H

#include "stm32f10x.h"

#define OTA_FLAG_UPDATE 0x5555AAAA

typedef struct
{
    uint32_t flag;          // OTA状态
    uint32_t file_size;     
    uint32_t crc32;
    uint32_t recv_size;     // 已接收大小
    uint32_t retry;         // 重试次数
    char version[16];
}OTA_Info_t;

void OTA_Info_Write(OTA_Info_t *info);
void OTA_Info_Read(OTA_Info_t *info);

#endif
