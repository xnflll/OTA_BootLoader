#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"

/* =========================
   USART1旧接口
   保留这些变量，避免你现在的OTA代码报错
   ========================= */
extern uint8_t USART1_RX_BUF[256];
extern uint16_t USART1_RX_LEN;

/* =========================
   USART1
   PA9  -> TX
   PA10 -> RX
   用于ESP-01S
   ========================= */
void USART1_Init(uint32_t baudrate);
void USART1_SendChar(uint8_t ch);
void USART1_SendString(char *str);
void USART1_SendHex(uint8_t data);

/* =========================
   USART1环形缓冲区
   ========================= */
uint16_t USART1_RxAvailable(void);
uint8_t USART1_ReadByte(void);
void USART1_ClearRxBuffer(void);

/* =========================
   USART2
   PA2 -> TX
   PA3 -> RX
   用于串口助手
   ========================= */
void USART2_Init(uint32_t baudrate);
void USART2_SendChar(uint8_t ch);
void USART2_SendString(char *str);
void USART2_SendHex(uint8_t data);
void USART1_IRQHandler(void);

#endif
