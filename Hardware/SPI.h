#ifndef __SPI_H
#define __SPI_H

#include "stm32f10x.h"

void SPI1_Init(void);
uint8_t SPI1_ReadWriteByte(uint8_t TxData);

#endif
