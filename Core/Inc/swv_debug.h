/**
 * @file swv_debug.h
 * @brief Serial Wire Viewer (SWV) debug output via ITM
 * 
 * Usage:
 *   1. Include this header: #include "swv_debug.h"
 *   2. Call SWV_Init() once at startup (after SystemClock_Config)
 *   3. Use SWV_Print() for strings, SWV_PrintHex() for hex values
 * 
 * View output in VS Code: 
 *   - Open "SWO: ITM" panel (View -> Output -> select "SWO: ITM [port 0]")
 *   - Or use the "Cortex Debug" SWO viewer
 */

#ifndef SWV_DEBUG_H
#define SWV_DEBUG_H

#include "stm32g4xx_hal.h"
#include <stdint.h>


/* ITM Stimulus Port 0 register */
#define ITM_STIM_PORT0  (*(volatile uint32_t*)0xE0000000UL)
#define ITM_TER         (*(volatile uint32_t*)0xE0000E00UL)
#define ITM_TCR         (*(volatile uint32_t*)0xE0000E80UL)

/**
 * @brief Initialize SWV/ITM for debug output
 * @note  Call this after SystemClock_Config()
 */
static inline void SWV_Init(void)
{
    /* Enable TRCENA (Trace Enable) in CoreDebug DEMCR */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    /* ITM Unlock */
    ITM->LAR = 0xC5ACCE55;
    
    /* ITM Trace Control: Enable ITM, sync packets, DWT packets */
    ITM->TCR = ITM_TCR_ITMENA_Msk | ITM_TCR_SYNCENA_Msk | ITM_TCR_DWTENA_Msk;
    
    /* Enable stimulus port 0 */
    ITM->TER = 0x1;
}

/**
 * @brief Send a single character via ITM port 0
 * @param ch Character to send
 */
static inline void SWV_SendChar(char ch)
{
    /* Wait until ITM port 0 is ready */
    while ((ITM->PORT[0].u32 == 0) && ((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0));
    
    /* Write character */
    ITM->PORT[0].u8 = (uint8_t)ch;
}

/**
 * @brief Print a string via SWV
 * @param str Null-terminated string
 */
static inline void SWV_Print(const char *str)
{
    while (*str)
    {
        SWV_SendChar(*str++);
    }
}

/**
 * @brief Print a string with newline via SWV
 * @param str Null-terminated string
 */
static inline void SWV_PrintLn(const char *str)
{
    SWV_Print(str);
    SWV_SendChar('\r');
    SWV_SendChar('\n');
}

/**
 * @brief Print a byte array as hex (useful for SPI data)
 * @param label Description label
 * @param data  Pointer to data buffer
 * @param len   Number of bytes to print
 */
static inline void SWV_PrintHex(const char *label, const uint8_t *data, uint16_t len)
{
    static const char hex[] = "0123456789ABCDEF";
    
    SWV_Print(label);
    SWV_Print(": ");
    
    for (uint16_t i = 0; i < len; i++)
    {
        SWV_SendChar(hex[(data[i] >> 4) & 0x0F]);
        SWV_SendChar(hex[data[i] & 0x0F]);
        SWV_SendChar(' ');
    }
    SWV_SendChar('\r');
    SWV_SendChar('\n');
}

/**
 * @brief Print a 32-bit value as hex
 * @param label Description label
 * @param value 32-bit value
 */
static inline void SWV_PrintVal32(const char *label, uint32_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    
    SWV_Print(label);
    SWV_Print(": 0x");
    
    for (int i = 7; i >= 0; i--)
    {
        SWV_SendChar(hex[(value >> (i * 4)) & 0x0F]);
    }
    SWV_SendChar('\r');
    SWV_SendChar('\n');
}

#endif /* SWV_DEBUG_H */
