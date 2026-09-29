#include "w25q64.h"
#include "spi.h"
#include "Delay.h"
#include "usart.h"

#define W25Q64_CS_LOW()  GPIO_ResetBits(GPIOA,GPIO_Pin_4)
#define W25Q64_CS_HIGH() GPIO_SetBits(GPIOA,GPIO_Pin_4)

void W25Q64_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

    // CS
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
    GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	W25Q64_CS_HIGH();
	
	SPI1_Init();
}

uint32_t W25Q64_ReadID(void)
{
    uint8_t id1,id2,id3;
	
    W25Q64_CS_LOW();

    SPI1_ReadWriteByte(0x9F);

    id1 = SPI1_ReadWriteByte(0xFF);
    id2 = SPI1_ReadWriteByte(0xFF);
    id3 = SPI1_ReadWriteByte(0xFF);

    W25Q64_CS_HIGH();

    return(id1<<16) | (id2<<8) | id3;
}

uint8_t W25Q64_ReadStatus1(void)
{
    uint8_t status;

    W25Q64_CS_LOW();
    SPI1_ReadWriteByte(0x05);
    status = SPI1_ReadWriteByte(0xff);
    W25Q64_CS_HIGH();
    return status;
}

void W25Q64_WriteEnable(void)
{
    W25Q64_CS_LOW();
    SPI1_ReadWriteByte(0x06);
    W25Q64_CS_HIGH();
	Delay_us(1);
	
	if((W25Q64_ReadStatus1() & 0x02)==0)
    {
        USART2_SendString("WRITE ENABLE FAIL\r\n");
    }
    else
    {
        USART2_SendString("WEL OK\r\n");
    }
}

uint8_t W25Q64_WaitBusy(void)
{
    uint8_t status;
    uint32_t timeout = 0;

    do
    {
        W25Q64_CS_LOW();
        SPI1_ReadWriteByte(0x05);
        status = SPI1_ReadWriteByte(0xff);
        W25Q64_CS_HIGH();

        Delay_ms(1);
        timeout++;

        if(timeout > 10000)
        {
            USART2_SendString("W25Q64 BUSY TIMEOUT\r\n");
            return 0;
        }
    }while(status & 0x01);

    return 1;
}

uint8_t W25Q64_EraseSector(uint32_t addr)
{
    USART2_SendString("ERASE START\r\n");
    W25Q64_WriteEnable();
    USART2_SendString("WRITE ENABLE OK\r\n");
    W25Q64_CS_LOW();

    SPI1_ReadWriteByte(0x20);
    SPI1_ReadWriteByte(addr>>16);
    SPI1_ReadWriteByte(addr>>8);
    SPI1_ReadWriteByte(addr);

    W25Q64_CS_HIGH();
    USART2_SendString("CMD OK\r\n");

    return W25Q64_WaitBusy();
}

void W25Q64_Write(uint8_t *buf,uint32_t addr,uint16_t len)
{
    W25Q64_WriteEnable();
    W25Q64_CS_LOW();

    SPI1_ReadWriteByte(0x02);
    SPI1_ReadWriteByte(addr>>16);
    SPI1_ReadWriteByte(addr>>8);
    SPI1_ReadWriteByte(addr);

    while(len--)
    {
        SPI1_ReadWriteByte(*buf++);
    }

    W25Q64_CS_HIGH();
    if(W25Q64_WaitBusy()==0)
	{
		USART2_SendString("WRITE TIMEOUT\r\n");
	}
}

void W25Q64_Read(uint8_t *buf,uint32_t addr,uint16_t len)
{
    W25Q64_CS_LOW();

    SPI1_ReadWriteByte(0x03);
    SPI1_ReadWriteByte(addr>>16);
    SPI1_ReadWriteByte(addr>>8);
    SPI1_ReadWriteByte(addr);

    while(len--)
    {
        *buf++ =SPI1_ReadWriteByte(0xff);
    }

    W25Q64_CS_HIGH();
}
