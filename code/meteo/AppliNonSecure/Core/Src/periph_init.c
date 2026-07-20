#include "periph_init.h"
#include "main.h"
#include "lora.h"
#include "lighthz.h"
#include <string.h>
#include <stdio.h>

/* Capteurs existants */
extern int hts221_init(void);
extern int pressure_init(void);

/* Handles partagés (définis dans main.c) */
extern I2C_HandleTypeDef  hi2c1;
extern UART_HandleTypeDef huart1;

/* LEDs */
#define LED_PORT       GPIOG
#define LED_BLUE_PIN   GPIO_PIN_8
#define LED_RED_PIN    GPIO_PIN_10
#define LED_GREEN_PIN  GPIO_PIN_0
#define ALL_LEDS_OFF() HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN|LED_GREEN_PIN|LED_BLUE_PIN, GPIO_PIN_SET)

/* ------------------------------------------------------------------ */
/*  Inits matérielles (privées à ce fichier)                          */
/* ------------------------------------------------------------------ */
static void _gpio_init(void)
{
    __HAL_RCC_GPIOG_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = LED_RED_PIN | LED_GREEN_PIN | LED_BLUE_PIN;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &g);
    ALL_LEDS_OFF();
}

/* LightHZ Click Board MIKROE-990 (TSL230BR) — MikroBUS INT = PE9        */
static void _light_sensor_init(void)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin   = GPIO_PIN_9;
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOE, &g);
    lighthz_init();
}

static void _uart_init(void)
{
    huart1.Instance        = USART1;
    huart1.Init.BaudRate   = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits   = UART_STOPBITS_1;
    huart1.Init.Parity     = UART_PARITY_NONE;
    huart1.Init.Mode       = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart1);

    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_5 | GPIO_PIN_6;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOE, &g);
}

static void _i2c_init(void)
{
    hi2c1.Instance            = I2C1;
    hi2c1.Init.Timing         = 0x00B03FDB;
    hi2c1.Init.OwnAddress1    = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    HAL_I2C_Init(&hi2c1);

    GPIO_InitTypeDef g = {0};
    g.Mode      = GPIO_MODE_AF_OD;
    g.Pull      = GPIO_PULLUP;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF4_I2C1;
    g.Pin       = GPIO_PIN_1;   /* PC1 SDA */
    HAL_GPIO_Init(GPIOC, &g);
    g.Pin       = GPIO_PIN_9;   /* PH9 SCL */
    HAL_GPIO_Init(GPIOH, &g);
}

/* ------------------------------------------------------------------ */
/*  Init capteurs (privée)                                            */
/* ------------------------------------------------------------------ */
static void _sensors_init(void)
{
    char msg[64];

    /* HTS221 — Temp + Humidité */
    if (hts221_init() != 0) {
        HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN, GPIO_PIN_RESET); /* rouge fixe */
        sprintf(msg, "[FAIL] HTS221\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
        while (1);
    }
    sprintf(msg, "[OK] HTS221\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

    /* LPS22HH — Pression */
    /* → Remplace pressure_init() ici si tu changes de capteur */
    if (pressure_init() != 0) {
        HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN, GPIO_PIN_RESET);
        sprintf(msg, "[FAIL] LPS22HH\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
        while (1);
    }
    sprintf(msg, "[OK] LPS22HH\r\n");
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
}

/* ------------------------------------------------------------------ */
/*  FONCTION PUBLIQUE : appelée une seule fois dans main.c            */
/*  → Pour ajouter un périphérique : ajouter son init ici            */
/* ------------------------------------------------------------------ */
void periph_init_all(void)
{
    /* Clocks */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    /* Périphériques */
    _gpio_init();
    _uart_init();           /* UART en premier pour pouvoir logger les inits suivantes */
    _light_sensor_init();
    _i2c_init();

    /* Capteurs */
    _sensors_init();

    /* Module LoRa sur USART2 (D0=PC2=RX, D1=PD5=TX) */
    lora_uart_init();
}
