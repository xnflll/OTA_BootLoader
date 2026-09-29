#include "ota_uart.h"
#include "usart.h"
#include "ota_protocol.h"
#include <string.h>

void OTA_UART_Parse(void)
{
    //没有数据
    if(USART1_RX_LEN == 0)
        return;

    uint8_t cmd;
    cmd = USART1_RX_BUF[0];

    /**********************
     * OTA_START
     **********************/
    if(cmd == 0x01)
    {
        //START总长度:
        //CMD 1
        //FILE_SIZE 4
        //CRC32 4
        //VERSION 16
        //共25字节

        if(USART1_RX_LEN < 25)
            return;

        OTA_StartPacket_t start;
        start.file_size = ((uint32_t)USART1_RX_BUF[1]<<24) | ((uint32_t)USART1_RX_BUF[2]<<16) | ((uint32_t)USART1_RX_BUF[3]<<8) | USART1_RX_BUF[4];
        start.crc32 = ((uint32_t)USART1_RX_BUF[5]<<24) | ((uint32_t)USART1_RX_BUF[6]<<16) | ((uint32_t)USART1_RX_BUF[7]<<8) | USART1_RX_BUF[8];
        memcpy(start.version,&USART1_RX_BUF[9],16);
        OTA_Start_Process(&start);

        //清空接收缓存
        USART1_RX_LEN = 0;
        return;
    }

    /**********************
     * OTA_DATA
     **********************/
    if(cmd == 0x02)
    {
		uint16_t len;

		/* 至少收到 CMD + SEQ + LEN */
		if(USART1_RX_LEN < 5)
			return;

		len = ((uint16_t)USART1_RX_BUF[3] << 8) | USART1_RX_BUF[4];

		/* 等待整个数据包接收完成 */
		if(USART1_RX_LEN < len + 7)
			return;

		OTA_DataPacket_t packet;
		packet.seq = ((uint16_t)USART1_RX_BUF[1] << 8) | USART1_RX_BUF[2];
		packet.len = len;
		memcpy(packet.data,&USART1_RX_BUF[5],packet.len);
		packet.crc = ((uint16_t)USART1_RX_BUF[5 + packet.len] << 8)|USART1_RX_BUF[6 + packet.len];
		OTA_Data_Process(&packet);
		USART1_RX_LEN = 0;

		return;
    }
	
	if(cmd==0x03)
	{
		OTA_EndPacket_t end;
		end.crc32 =((uint32_t)USART1_RX_BUF[1]<<24)|((uint32_t)USART1_RX_BUF[2]<<16)|((uint32_t)USART1_RX_BUF[3]<<8)|USART1_RX_BUF[4];
		OTA_End_Process(&end);
		USART1_RX_LEN=0;
	}
}
