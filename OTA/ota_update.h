#ifndef __OTA_UPDATE_H
#define __OTA_UPDATE_H

#include "stm32f10x.h"

/* APP起始地址 */
#define APP_START_ADDR      0x08005000UL

/* STM32F103C8T6按64KB Flash计算 */
#define APP_END_ADDR        0x08010000UL

/* STM32F1中等密度器件页大小 */
#define APP_PAGE_SIZE       1024U

/* APP在W25Q64中的存储起始地址 */
#define OTA_APP_ADDR        0x00000000UL

/*
 * 把W25Q64中的APP固件
 * 更新到STM32内部Flash
 *
 * file_size:
 * APP.bin实际大小
 *
 * 返回：
 * 1 = 成功
 * 0 = 失败
 */
uint8_t OTA_Update_APP(uint32_t file_size);

#endif
