#include "ota_update.h"
#include "w25q64.h"
#include "usart.h"
#include "stm32f10x_flash.h"

#define OTA_READ_BUFFER_SIZE    256

static uint8_t OTA_Read_Buffer[OTA_READ_BUFFER_SIZE];

/*
 * 检查APP向量表是否有效
 */
static uint8_t OTA_Check_APP(void)
{
    uint32_t app_msp;
    uint32_t app_reset;

    app_msp = *(__IO uint32_t *)APP_START_ADDR;
    app_reset = *(__IO uint32_t *)(APP_START_ADDR + 4);

    /*
     * 检查MSP是否位于SRAM区域
     */
    if((app_msp & 0xFF000000UL) != 0x20000000UL)
    {
        return 0;
    }

    /*
     * 检查Reset_Handler是否位于APP区域
     */
    if(app_reset < APP_START_ADDR ||
       app_reset >= APP_END_ADDR)
    {
        return 0;
    }

    return 1;
}

/*
 * 将W25Q64中的APP固件
 * 更新到STM32内部Flash
 */
uint8_t OTA_Update_APP(uint32_t file_size)
{
    uint32_t i;
    uint32_t addr;
    uint32_t w25_addr;

    uint32_t app_end_addr;
    uint32_t erase_addr;

    uint16_t len;
    uint16_t data16;

    uint8_t result = 0;

    USART2_SendString("APP UPDATE START\r\n");

    /*
     * 文件大小检查
     */
    if(file_size == 0)
    {
        USART2_SendString("FILE SIZE ERROR\r\n");
        return 0;
    }

    /*
     * 检查是否超过APP空间
     */
    if(file_size > (APP_END_ADDR - APP_START_ADDR))
    {
        USART2_SendString("APP TOO LARGE\r\n");
        return 0;
    }

    /*
     * 计算APP实际结束地址
     */
    app_end_addr = APP_START_ADDR + file_size;

    /*
     * 关闭中断
     *
     * 防止Flash擦写过程中进入中断
     */
    __disable_irq();

    /*
     * 解锁内部Flash
     */
    FLASH_Unlock();

    /*
     * 清除Flash状态标志
     */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    /*
     * ==========================
     * 1. 擦除APP区域
     * ==========================
     */
    USART2_SendString("ERASE APP...\r\n");
    erase_addr = APP_START_ADDR;

    while(erase_addr < app_end_addr)
    {
        if(FLASH_ErasePage(erase_addr) != FLASH_COMPLETE)
        {
            USART2_SendString("ERASE ERROR\r\n");
            FLASH_Lock();
            __enable_irq();
            return 0;
        }
        erase_addr += APP_PAGE_SIZE;
    }

    USART2_SendString("ERASE OK\r\n");

    /*
     * ==========================
     * 2. 从W25Q64读取
     * ==========================
     */
    addr = APP_START_ADDR;
    w25_addr = OTA_APP_ADDR;

    while((addr - APP_START_ADDR) < file_size)
    {
        /*
         * 计算本次读取长度
         */
        if((file_size - (addr - APP_START_ADDR)) > OTA_READ_BUFFER_SIZE)
        {
            len = OTA_READ_BUFFER_SIZE;
        }
        else
        {
            len = file_size - (addr - APP_START_ADDR);
        }

        /*
         * 从W25Q64读取APP数据
         */
        W25Q64_Read(OTA_Read_Buffer,w25_addr,len);

        /*
         * ==========================
         * 3. 写入内部Flash
         * ==========================
         */
        for(i = 0; i < len; i += 2)
        {
            /*
             * 组成16位数据
             *
             * STM32F1 Flash半字编程
             */
            if((i + 1) < len)
            {
                data16 = ((uint16_t)OTA_Read_Buffer[i + 1] << 8)|OTA_Read_Buffer[i];
            }
            else
            {
                /*
                 * 如果最后只剩1个字节
                 * 高8位补FF
                 */
                data16 = ((uint16_t)0xFF << 8)|OTA_Read_Buffer[i];
            }

            /*
             * 写入内部Flash
             */
            if(FLASH_ProgramHalfWord(addr + i,data16) != FLASH_COMPLETE)
            {
                USART2_SendString("FLASH WRITE ERROR\r\n");
                FLASH_Lock();
                __enable_irq();
                return 0;
            }

            /*
             * 写入后立即校验
             */
            if(*(__IO uint16_t *)(addr + i)!= data16)
            {
                USART2_SendString("FLASH VERIFY ERROR\r\n");
                FLASH_Lock();
                __enable_irq();
                return 0;
            }
        }


        /*
         * 更新地址
         */
        addr += len;
        w25_addr += len;
    }


    /*
     * 锁定Flash
     */
    FLASH_Lock();

    /*
     * 开启中断
     */
    __enable_irq();

    /*
     * ==========================
     * 4. APP向量表检查
     * ==========================
     */

    if(OTA_Check_APP() == 0)
    {
        USART2_SendString("APP VECTOR ERROR\r\n");
        return 0;
    }

    USART2_SendString("APP UPDATE OK\r\n");
    result = 1;

    return result;
}
