#include "stm32f10x.h"

typedef void (*pFunction)(void);

void Jump_To_App(uint32_t app_addr)
{
    uint32_t app_msp;
    uint32_t app_reset;
    pFunction JumpToApplication;

    app_msp   = *(__IO uint32_t *)app_addr;
    app_reset = *(__IO uint32_t *)(app_addr + 4);

    /* 关闭中断 */
    __disable_irq();

    /* 关闭 SysTick */
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    /* 设置中断向量表 */
    SCB->VTOR = app_addr;

    /* 设置 APP 的主栈地址 */
    __set_MSP(app_msp);

    /* 设置 APP Reset_Handler */
    JumpToApplication = (pFunction)app_reset;

    /* 跳转 */
    JumpToApplication();
}
