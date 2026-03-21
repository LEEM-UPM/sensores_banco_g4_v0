#include "main.h"
#include "gpio.h"
#include "stm32g4xx_hal_gpio.h"
#include "swv_debug.h"
#include "stm32g4xx_hal.h"

extern void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    extern void HAL_GPIO_Init();
    SystemClock_Config();
    
    // Initialize SWV after clocks are configured
    SWV_Init();
    SWV_PrintLn("Hello, SWV Debugging!");
    
    while (1)
    {
        HAL_Delay(1000);
        SWV_PrintLn("SWV tick");
        HAL_GPIO_TogglePin(GPIOA, LED1_Pin);
    }
}
