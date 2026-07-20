#include "display.h"
#include "sensors.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart1;

#define LED_RED_ON()  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET)
#define LED_RED_OFF() HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET)

/* Pression de reference au niveau de la mer (hPa) */
#define SEA_LEVEL_HPA  1017.8f   /* pression mer Aix-les-Bains 08/06/2026 14h10 UTC */

/* ------------------------------------------------------------------ */
/*  Affichage UART                                                     */
/* ------------------------------------------------------------------ */
void display_data(void)
{
    char msg[160];

    LED_RED_ON();

    int t_ent = (int)(g_sensors.temperature / 10);
    int t_dec = abs((int)(g_sensors.temperature % 10));

    int h_ent = (int)(g_sensors.humidity / 10);
    int h_dec = abs((int)(g_sensors.humidity % 10));

    int p_ent = (int)(g_sensors.pressure / 10);
    int p_dec = abs((int)(g_sensors.pressure % 10));

    /* Altitude : alt (m) = (P0 - P) * 8.333 */
    float pres_hpa = (float)g_sensors.pressure / 10.0f;
    int   alt_m    = (int)((SEA_LEVEL_HPA - pres_hpa) * 8.333f);

    const char *jour_nuit = g_sensors.is_day ? "JOUR" : "NUIT";

    sprintf(msg,
        "| Temp : %3d.%d C | Humi : %3d.%d %% | Pres : %4d.%d hPa | Alt : %4d m | Lum : %3ld %% | %s |\r\n",
        t_ent, t_dec, h_ent, h_dec, p_ent, p_dec, alt_m,
        (long)g_sensors.lux_percent, jour_nuit);

    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 200);

    LED_RED_OFF();
}
