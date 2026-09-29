#include "ota_protocol.h"
#include "w25q64.h"
#include "usart.h"
#include "ota_info.h"
#include "ota_update.h"
#include "string.h"

#define W25Q64_SECTOR_SIZE 4096UL

/* 当前W25Q64写入地址 */
static uint32_t OTA_Write_Address = 0;

/* OTA信息 */
static OTA_Info_t ota_info;

/* 期望数据包序号 */
static uint16_t OTA_Expect_Seq = 1;

/* OTA状态 */
static OTA_State_t OTA_State = OTA_IDLE;

/* =========================================================
   OTA DATA
   ========================================================= */
void OTA_Data_Process(OTA_DataPacket_t *packet)
{
    uint16_t crc;

    /* 状态检查 */
    if(OTA_GetState() != OTA_RECEIVING)
    {
        USART2_SendString("NOT READY\r\n");
        return;
    }

    /* 序号检查 */
    if(packet->seq != OTA_Expect_Seq)
    {
        USART2_SendString("SEQ ERROR\r\n");
        return;
    }

    /* 长度检查 */
    if(packet->len == 0 ||
       packet->len > OTA_PACKET_SIZE)
    {
        USART2_SendString("LEN ERROR\r\n");
        return;
    }

    /* CRC16 */
    crc = OTA_CRC16(packet->data,packet->len);

    USART2_SendString("LEN=");
    USART2_SendHex(packet->len >> 8);
    USART2_SendHex(packet->len);
    USART2_SendString("\r\n");
    USART2_SendString("CALC=");
    USART2_SendHex(crc >> 8);
    USART2_SendHex(crc);
    USART2_SendString("\r\n");
    USART2_SendString("RX=");
    USART2_SendHex(packet->crc >> 8);
    USART2_SendHex(packet->crc);
    USART2_SendString("\r\n");

    /* CRC错误 */
    if(crc != packet->crc)
    {
        USART2_SendString("CRC ERROR\r\n");
        return;
    }

    /* 写入W25Q64 */
    W25Q64_Write(packet->data,OTA_Write_Address,packet->len);
    USART2_SendString("FLASH WRITE OK\r\n");
    OTA_Write_Address += packet->len;
    OTA_Expect_Seq++;
    USART2_SendString("OTA DATA OK\r\n");
}

/* =========================================================
   CRC16
   ========================================================= */
uint16_t OTA_CRC16(uint8_t *data,uint16_t len)
{
    uint16_t crc = 0xFFFF;

    while(len--)
    {
        crc ^= *data++;

        for(uint8_t i = 0; i < 8; i++)
        {
            if(crc & 0x0001)
            {
                crc = (crc >> 1) ^ 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* =========================================================
   OTA START
   ========================================================= */
void OTA_Start_Process(OTA_StartPacket_t *start)
{
    uint32_t erase_addr;
    uint32_t erase_end;

    USART2_SendString("OTA START\r\n");

    /* 文件大小检查 */
    if(start->file_size == 0)
    {
        USART2_SendString("FILE SIZE ERROR\r\n");
        OTA_SetState(OTA_ERROR);
        return;
    }

    /* APP空间检查 */
    if(start->file_size > (APP_END_ADDR - APP_START_ADDR))
    {
        USART2_SendString("APP TOO LARGE\r\n");
        OTA_SetState(OTA_ERROR);
        return;
    }

    /*
     * 每次OTA重新开始
     */
    OTA_Write_Address = OTA_APP_ADDR;
    OTA_Expect_Seq = 1;

    /*
     * 保存升级信息
     */
    ota_info.flag = 0xAAAAAAAA;
    ota_info.file_size = start->file_size;
    ota_info.crc32 = start->crc32;

    memcpy(ota_info.version,start->version,16);

    /*
     * =====================================================
     * 擦除W25Q64旧固件
     *
     * OTA_INFO在0x7FF000
     * APP缓存从0开始
     * 两者不会冲突
     * =====================================================
     */
    USART2_SendString("ERASE W25Q64...\r\n");

    erase_addr = OTA_APP_ADDR;
    erase_end = OTA_APP_ADDR + start->file_size;

	while(erase_addr < erase_end)
	{
		USART2_SendString("ERASE:");
		USART2_SendHex(erase_addr>>16);
		USART2_SendHex(erase_addr>>8);
		USART2_SendHex(erase_addr);
		USART2_SendString("\r\n");

		if(W25Q64_EraseSector(erase_addr)==0)
		{
			USART2_SendString("ERASE FAIL\r\n");
			OTA_SetState(OTA_ERROR);
			return;
		}
		USART2_SendString("OK\r\n");
		erase_addr += W25Q64_SECTOR_SIZE;
	}

    USART2_SendString("ERASE W25Q64 OK\r\n");

    /*
     * 保存OTA信息
     */
    OTA_Info_Write(&ota_info);

    /*
     * 进入接收状态
     */
    OTA_SetState(OTA_RECEIVING);

    USART2_SendString("OTA READY\r\n");
}

/* =========================================================
   OTA状态
   ========================================================= */
void OTA_SetState(OTA_State_t state)
{
    OTA_State = state;
}

OTA_State_t OTA_GetState(void)
{
    return OTA_State;
}

/* =========================================================
   CRC32
   ========================================================= */
uint32_t OTA_CRC32(uint8_t *data,uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    while(len--)
    {
        crc ^= *data++;

        for(uint8_t i = 0; i < 8; i++)
        {
            if(crc & 1)
            {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/* =========================================================
   OTA END
   ========================================================= */
void OTA_End_Process(OTA_EndPacket_t *end)
{
    uint8_t buffer[128];
    uint32_t addr = 0;
    uint32_t crc = 0xFFFFFFFF;
    uint16_t len;

    USART2_SendString("OTA END\r\n");

    if(OTA_GetState() != OTA_RECEIVING)
    {
        USART2_SendString("OTA NOT READY\r\n");
        return;
    }

    /*
     * 从W25Q64重新计算整包CRC32
     */
    while(addr < ota_info.file_size)
    {
        if(ota_info.file_size - addr > 128)
        {
            len = 128;
        }
        else
        {
            len = ota_info.file_size - addr;
        }

        W25Q64_Read(buffer,OTA_APP_ADDR + addr,len);

        for(uint16_t i = 0;i < len;i++)
        {
            crc ^= buffer[i];

            for(uint8_t j = 0;j < 8;j++)
            {
                if(crc & 1)
                {
                    crc = (crc >> 1) ^ 0xEDB88320;
                }
                else
                {
                    crc >>= 1;
                }
            }
        }
        addr += len;
    }

    crc ^= 0xFFFFFFFF;

    USART2_SendString("CALC CRC=");
    USART2_SendHex((crc >> 24) & 0xFF);
    USART2_SendHex((crc >> 16) & 0xFF);
    USART2_SendHex((crc >> 8) & 0xFF);
    USART2_SendHex(crc & 0xFF);
    USART2_SendString("\r\n");

    /*
     * 注意：
     * 这里使用OTA_END携带的CRC32。
     *
     * 因为HTTP下载开始时我们还不知道CRC32。
     */
    if(crc != end->crc32)
    {
        USART2_SendString("CRC32 ERROR\r\n");
        OTA_SetState(OTA_ERROR);
        return;
    }

    USART2_SendString("CRC32 OK\r\n");

    /*
     * 把最终CRC保存下来
     */
    ota_info.crc32 = end->crc32;

    /*
     * W25Q64 → STM32内部Flash
     */
    if(OTA_Update_APP(ota_info.file_size))
    {
        ota_info.flag = 0x55555555;
        OTA_Info_Write(&ota_info);
        OTA_SetState(OTA_DONE);
        USART2_SendString("OTA UPDATE SUCCESS\r\n");
    }
    else
    {
        USART2_SendString("OTA UPDATE FAILED\r\n");
        OTA_SetState(OTA_ERROR);
    }
}
