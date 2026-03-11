#include "stm32f1xx_hal.h"

/**********************************************************************
 * 函数名称： udelay
 * 功能描述： us级别的延时函数
 * 输入参数： us - 延时多少us
 * 输出参数： 无
 * 返 回 值： 无
 ***********************************************************************/
void udelay(int us)
{
    extern TIM_HandleTypeDef htim4; // 声明定时器句柄
    TIM_HandleTypeDef *hHalTim = &htim4; // 获取定时器句柄 

    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    uint32_t reload = hHalTim->Instance->ARR; // 获取定时器的自动重装载值

    ticks = us * reload / 1000; // 计算需要的计数值,reload对应1ms的计数值
    told = hHalTim->Instance->CNT; // 获取当前计数器的值
    while (1)
    {
        tnow = hHalTim->Instance->CNT;
        if(tnow != told)
        {
            
        }
    }
    
}
