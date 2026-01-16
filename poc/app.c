#include "main.h"
#include "gpio.h"


void SystemClock_Config(void);

int main(void)
{
    // configuracion de HAL
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    while (1)
    {
        // 
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        HAL_Delay(500);
    }
}
