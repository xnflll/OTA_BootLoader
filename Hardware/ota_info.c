#include "ota_info.h"
#include "w25q64.h"

#define OTA_INFO_ADDR 0x7FF000

void OTA_Info_Write(OTA_Info_t *info)
{
    W25Q64_EraseSector(OTA_INFO_ADDR);
    W25Q64_Write((uint8_t *)info,OTA_INFO_ADDR,sizeof(OTA_Info_t));
}

void OTA_Info_Read(OTA_Info_t *info)
{
    W25Q64_Read((uint8_t *)info,OTA_INFO_ADDR,sizeof(OTA_Info_t));
}
