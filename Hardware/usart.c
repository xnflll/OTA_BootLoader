#include "usart.h"

/* =========================================================
   旧版缓冲区
   暂时保留，兼容你现在的OTA代码
   ========================================================= */
uint8_t USART1_RX_BUF[256];
uint16_t USART1_RX_LEN = 0;

/* =========================================================
   USART1环形缓冲区
   4096字节
   ========================================================= */
#define USART1_RING_SIZE 4096

static volatile uint8_t
USART1_RingBuf[USART1_RING_SIZE];

static volatile uint16_t
USART1_RingHead = 0;

static volatile uint16_t
USART1_RingTail = 0;

/* =========================================================
   USART1初始化
   PA9  TX
   PA10 RX
   ========================================================= */
void USART1_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 开启时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1,ENABLE);

    /* PA9 TX */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA,&GPIO_InitStructure);

    /* PA10 RX */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

    GPIO_Init(GPIOA,&GPIO_InitStructure);

    /* USART参数 */
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1,&USART_InitStructure);

    USART_Cmd(USART1,ENABLE);

    /* 开启RX中断 */
    USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);

    /* NVIC */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
}

/* =========================================================
   USART1发送1字节
   ========================================================= */
void USART1_SendChar(uint8_t ch)
{
    USART_SendData(USART1,ch);
    while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
}

/* =========================================================
   USART1发送字符串
   ========================================================= */
void USART1_SendString(char *str)
{
    while(*str)
    {
        USART1_SendChar(*str++);
    }
}

/* =========================================================
   USART1发送HEX
   ========================================================= */
void USART1_SendHex(uint8_t data)
{
    uint8_t temp;
    temp = (data >> 4) & 0x0F;

    if(temp < 10)
    {
        USART1_SendChar(temp + '0');
    }
    else
    {
        USART1_SendChar(temp - 10 + 'A');
    }

    temp = data & 0x0F;

    if(temp < 10)
    {
        USART1_SendChar(temp + '0');
    }
    else
    {
        USART1_SendChar(temp - 10 + 'A');
    }
}


/* =========================================================
   USART2初始化
   PA2 TX
   PA3 RX
   ========================================================= */
void USART2_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    /* GPIOA */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);

    /* USART2 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);

    /* PA2 TX */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA,&GPIO_InitStructure);

    /* PA3 RX */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

    GPIO_Init(GPIOA,&GPIO_InitStructure);

    /* 参数 */
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART2,&USART_InitStructure);

    USART_Cmd(USART2,ENABLE);
}


/* =========================================================
   USART2发送
   ========================================================= */
void USART2_SendChar(uint8_t ch)
{
    USART_SendData(USART2,ch);
    while(USART_GetFlagStatus(USART2,USART_FLAG_TXE) == RESET);
}


/* =========================================================
   USART2发送字符串
   ========================================================= */
void USART2_SendString(char *str)
{
    while(*str)
    {
        USART2_SendChar(*str++);
    }
}

void USART2_SendHex(uint8_t data)
{
    uint8_t temp;
    temp = (data >> 4) & 0x0F;

    if(temp < 10)
    {
        USART2_SendChar(temp + '0');
    }
    else
    {
        USART2_SendChar(temp - 10 + 'A');
    }

    temp = data & 0x0F;

    if(temp < 10)
    {
        USART2_SendChar(temp + '0');
    }
    else
    {
        USART2_SendChar(temp - 10 + 'A');
    }
}

/* =========================================================
   获取环形缓冲区中的数据数量
   ========================================================= */
uint16_t USART1_RxAvailable(void)
{
    uint16_t head;
    uint16_t tail;

    head = USART1_RingHead;
    tail = USART1_RingTail;

    if(head >= tail)
    {
        return head - tail;
    }
    else
    {
        return USART1_RING_SIZE - tail + head;
    }
}

/* =========================================================
   从环形缓冲区读取一个字节
   ========================================================= */
uint8_t USART1_ReadByte(void)
{
    uint8_t data;
    /* 没数据 */

    if(USART1_RingHead == USART1_RingTail)
    {
        return 0;
    }

    data = USART1_RingBuf[USART1_RingTail];

    USART1_RingTail++;

    if(USART1_RingTail >= USART1_RING_SIZE)
    {
        USART1_RingTail = 0;
    }
    return data;
}

/* =========================================================
   清空环形缓冲区
   ========================================================= */
void USART1_ClearRxBuffer(void)
{
    USART1_RingHead = 0;
    USART1_RingTail = 0;
    USART1_RX_LEN = 0;
}

/* =========================================================
   USART1中断
   ========================================================= */
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1,USART_IT_RXNE) != RESET)
    {
        uint8_t data;
        uint16_t nextHead;

        /* 读取数据 */

        data = USART_ReceiveData(USART1);

        /* =================================
           写入环形缓冲区
           ================================= */

        nextHead = USART1_RingHead + 1;

        if(nextHead >= USART1_RING_SIZE)
        {
            nextHead = 0;
        }

        /*
           缓冲区满了：
           丢掉最旧的一个字节，
           保证最新数据能够继续进入
        */

        if(nextHead == USART1_RingTail)
        {
            USART1_RingTail++;

            if(USART1_RingTail >= USART1_RING_SIZE)
            {
                USART1_RingTail = 0;
            }
        }

        USART1_RingBuf[USART1_RingHead] = data;
        USART1_RingHead = nextHead;

        /* =================================
           兼容旧OTA代码
           ================================= */

        if(USART1_RX_LEN < 256)
        {
            USART1_RX_BUF[USART1_RX_LEN++] = data;
        }

        /* 清除中断 */
        USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
}
