#include "main.h"
#include "periph_init.h"
#include "sensors.h"
#include "display.h"
#include "app_x-cube-ai.h"
#include "lora.h"
#include <stdio.h>
#include <string.h>

I2C_HandleTypeDef  hi2c1;
UART_HandleTypeDef huart1;
UART_HandleTypeDef* UartHandle = &huart1;

int main(void)
{
    HAL_Init();

    /* Clocks + RIF + RAMCFG workaround already handled:
     *  - Secure world: NPU clock, MEMENR, RISAF, RISC, IRQ routing
     *  - SystemInit_POST() (called from startup): RAMCFG bit-20 cleared */

    // 1. Initialisation de l'UART et des autres périphériques
    periph_init_all();

    /* Activer le FPU NS : écriture mémoire directe sur CPACR (0xE000ED88).
     * Ce n'est PAS une instruction coprocesseur → pas de NOCP si NSACR=0.
     * Si system_stm32n6xx_s.c a bien positionné NSACR[10-11], cette écriture
     * prend effet et autorise les instructions FPU en mode NS. */
    *((volatile uint32_t *)0xE000ED88UL) |= (3UL << 20U) | (3UL << 22U);
    __DSB();
    __ISB();

    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n[DEBUG] Tentative init IA...\r\n", 31, 100);
    STM32CubeAI_Studio_AI_Init();

    char start_msg[] = "\r\n--- STATION METEO STM32N6 AVEC IA DEMARREE ---\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)start_msg, strlen(start_msg), 200);

    /* JOIN LoRaWAN */
    lora_join();

    while (1)
    {
        sensors_read();
        display_data();
        STM32CubeAI_Studio_AI_Process();
        HAL_Delay(2000);
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1);
}
