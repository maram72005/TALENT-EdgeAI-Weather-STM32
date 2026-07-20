#include "sensors.h"
#include "lighthz.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Variables partagées avec hts221_polling.c et pressure_polling.c  */
int32_t last_temp = 0;
int32_t last_rhum = 0;
int32_t last_pres = 0;

extern void hts221_read_temp(void);
extern void hts221_read_hum(void);
extern void pressure_read(void);

SensorData_t g_sensors = {0};

extern UART_HandleTypeDef huart1;

#define LED_GREEN_ON()  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_RESET)
#define LED_GREEN_OFF() HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_SET)

void sensors_read(void)
{
    LED_GREEN_ON();

    hts221_read_temp();
    hts221_read_hum();
    pressure_read();

    g_sensors.temperature = last_temp;
    g_sensors.humidity    = last_rhum;
    g_sensors.pressure    = last_pres;

    /* LightHZ MIKROE-990 : mesure de fréquence sur PE0 (MikroBUS INT) */
    uint32_t freq_hz      = lighthz_read_freq_hz();
    g_sensors.lux_percent = (int32_t)((freq_hz >= LIGHTHZ_MAX_FREQ_HZ) ? 100UL
                                       : (freq_hz * 100UL / LIGHTHZ_MAX_FREQ_HZ));
    g_sensors.is_day      = (g_sensors.lux_percent > 5) ? 1 : 0;

    char dbg[80];
    snprintf(dbg, sizeof(dbg), "[LUX] freq=%lu Hz -> %ld%%\r\n",
             (unsigned long)freq_hz, (long)g_sensors.lux_percent);
    HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);

    LED_GREEN_OFF();
}
