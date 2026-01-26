#include "main.h"
#include "gpio.h"



int main(void)
{
    HAL_Init();
    extern void SystemClock_Config();
    MX_GPIO_Init();

    while (1)
    {
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        HAL_Delay(500);
    }
}
