#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f10x.h"

void W25Q64_Init(void);
uint32_t W25Q64_ReadID(void);
void W25Q64_WriteEnable(void);
uint8_t W25Q64_WaitBusy(void);
uint8_t W25Q64_EraseSector(uint32_t addr);
void W25Q64_Write(uint8_t *buf,uint32_t addr,uint16_t len);
void W25Q64_Read(uint8_t *buf,uint32_t addr,uint16_t len);

#endif
