#include "lora.h"
#include "main.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart1;

/* Handle USART3 — Arduino D1(PD8=TX) et D0(PD9=RX) */
UART_HandleTypeDef huart3;

/* Baud rates a tester automatiquement */
static const uint32_t LORA_BAUDRATES[] = {9600, 115200, 57600, 38400, 19200};
#define LORA_NB_BAUDRATES  5

/* ------------------------------------------------------------------ */
/*  Init USART3 a un baud rate specifique (MSP gere PD8/PD9)          */
/* ------------------------------------------------------------------ */
static int _uart3_init_baud(uint32_t baud)
{
    if (huart3.Instance != NULL)
        HAL_UART_DeInit(&huart3);

    huart3.Instance        = USART3;
    huart3.Init.BaudRate   = baud;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits   = UART_STOPBITS_1;
    huart3.Init.Parity     = UART_PARITY_NONE;
    huart3.Init.Mode       = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl  = UART_HWCONTROL_NONE;

    return (HAL_UART_Init(&huart3) == HAL_OK) ? 1 : 0;
}

/* ------------------------------------------------------------------ */
/*  Init USART3 (Arduino D0=PD9=RX, D1=PD8=TX)                       */
/* ------------------------------------------------------------------ */
void lora_uart_init(void)
{
    if (!_uart3_init_baud(9600))
    {
        HAL_UART_Transmit(&huart1,
            (uint8_t*)"[LORA] ERREUR init USART3\r\n", 27, 100);
        return;
    }
    HAL_UART_Transmit(&huart1,
        (uint8_t*)"[LORA] USART3 init OK (D0=PD9=RX, D1=PD8=TX)\r\n", 47, 100);
}

/* ------------------------------------------------------------------ */
/*  Envoie une commande AT et attend une reponse                       */
/* ------------------------------------------------------------------ */
int lora_at_cmd(const char *cmd, const char *expected_resp, uint32_t timeout_ms)
{
    uint8_t rx_buf[256];
    uint8_t rx_byte;
    uint16_t idx = 0;
    uint32_t t_start = HAL_GetTick();
    char dbg[80];

    if (strlen(cmd) > 0)
    {
        HAL_UART_Transmit(&huart3, (uint8_t*)cmd, strlen(cmd), 200);
        HAL_UART_Transmit(&huart3, (uint8_t*)"\r\n", 2, 100);
        snprintf(dbg, sizeof(dbg), "[LORA] >> %s\r\n", cmd);
        HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);
    }

    memset(rx_buf, 0, sizeof(rx_buf));
    while ((HAL_GetTick() - t_start) < timeout_ms)
    {
        if (HAL_UART_Receive(&huart3, &rx_byte, 1, 10) == HAL_OK)
        {
            if (idx < 255) rx_buf[idx++] = rx_byte;
            if (expected_resp && strstr((char*)rx_buf, expected_resp))
            {
                HAL_UART_Transmit(&huart1, (uint8_t*)"[LORA] << ", 10, 50);
                HAL_UART_Transmit(&huart1, rx_buf, idx, 200);
                HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 50);
                return 1;
            }
        }
    }

    if (idx > 0) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"[LORA] << ", 10, 50);
        HAL_UART_Transmit(&huart1, rx_buf, idx, 200);
        HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 50);
    } else {
        HAL_UART_Transmit(&huart1,
            (uint8_t*)"[LORA] << (aucune reponse)\r\n", 28, 100);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Detection automatique du baud rate                                 */
/* ------------------------------------------------------------------ */
static uint32_t _lora_detect_baudrate(void)
{
    char dbg[64];
    HAL_UART_Transmit(&huart1,
        (uint8_t*)"[LORA] Detection baud rate sur D0/D1 (PD9/PD8)...\r\n", 51, 100);

    for (int i = 0; i < LORA_NB_BAUDRATES; i++)
    {
        snprintf(dbg, sizeof(dbg), "[LORA] Test %lu baud...\r\n", LORA_BAUDRATES[i]);
        HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);

        if (!_uart3_init_baud(LORA_BAUDRATES[i])) continue;
        HAL_Delay(100);

        if (lora_at_cmd("AT", "OK", 1000))
        {
            snprintf(dbg, sizeof(dbg),
                "[LORA] Baud rate trouve : %lu\r\n", LORA_BAUDRATES[i]);
            HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);
            return LORA_BAUDRATES[i];
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Procedure JOIN LoRaWAN                                            */
/* ------------------------------------------------------------------ */
int lora_join(void)
{
    char dbg[64];
    HAL_UART_Transmit(&huart1,
        (uint8_t*)"\r\n[LORA] Demarrage JOIN LoRaWAN...\r\n", 36, 100);

    uint32_t baud = _lora_detect_baudrate();
    if (baud == 0)
    {
        HAL_UART_Transmit(&huart1,
            (uint8_t*)"[LORA] Module ne repond pas.\r\n"
                      "[LORA] Branche ton module LoRa :\r\n"
                      "  Module TX  --> D0 (PD9) sur la carte\r\n"
                      "  Module RX  --> D1 (PD8) sur la carte\r\n"
                      "  Module GND --> GND\r\n"
                      "  Module 3V3 --> 3V3\r\n",
            160, 300);
        return 0;
    }

    snprintf(dbg, sizeof(dbg), "[LORA] Module OK a %lu baud\r\n", baud);
    HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);

    HAL_UART_Transmit(&huart1,
        (uint8_t*)"[LORA] Envoi AT+JOIN (max 30s par tentative)...\r\n", 50, 100);

    /* Module peut repondre "Network joined" OU "Joined already" (J majuscule) */
    const int MAX_JOIN_ATTEMPTS = 3;
    for (int attempt = 1; attempt <= MAX_JOIN_ATTEMPTS; attempt++)
    {
        snprintf(dbg, sizeof(dbg), "[LORA] Tentative JOIN %d/%d...\r\n", attempt, MAX_JOIN_ATTEMPTS);
        HAL_UART_Transmit(&huart1, (uint8_t*)dbg, strlen(dbg), 100);

        /* Cherche "oined" pour matcher "joined" ET "Joined" (insensible a la casse 1er char) */
        if (lora_at_cmd("AT+JOIN", "oined", 30000))
        {
            HAL_UART_Transmit(&huart1,
                (uint8_t*)"[LORA] *** JOINED - Reseau LoRaWAN rejoint ! ***\r\n",
                50, 100);
            return 1;
        }

        HAL_Delay(2000);
    }

    HAL_UART_Transmit(&huart1,
        (uint8_t*)"[LORA] JOIN echoue apres plusieurs tentatives. Verif DevEUI/AppKey sur le serveur.\r\n",
        86, 100);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Envoie les donnees meteo via AT+MSG                               */
/*  Format : "T:29.1,H:76.0,P:988.8,A:241,R:33"                     */
/*  Parametres : temp10 = temp * 10, hum10 = hum * 10, etc.          */
/* ------------------------------------------------------------------ */
int lora_send_data(int temp10, int hum10, int pres10, int alt_m, int pct_pluie, int is_day, int lux_pct)
{
    char payload[96];
    char cmd[120];

    /* Construire le payload lisible */
    int pct_beau = 100 - pct_pluie;
    snprintf(payload, sizeof(payload),
        "T:%d.%d,H:%d.%d,P:%d.%d,A:%d,R:%d,B:%d,D:%d,L:%d",
        temp10 / 10, abs(temp10 % 10),
        hum10  / 10, abs(hum10  % 10),
        pres10 / 10, abs(pres10 % 10),
        alt_m,
        pct_pluie,
        pct_beau,
        is_day,
        lux_pct);

    /* Commande AT+MSG pour envoi non-confirme (Seeed LoRa-E5) */
    snprintf(cmd, sizeof(cmd), "AT+MSG=\"%s\"", payload);

    HAL_UART_Transmit(&huart1, (uint8_t*)"[LORA] Envoi : ", 15, 50);
    HAL_UART_Transmit(&huart1, (uint8_t*)payload, strlen(payload), 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 20);

    /* Envoyer et attendre +MSG: Done (timeout 10s) */
    if (lora_at_cmd(cmd, "Done", 10000))
    {
        HAL_UART_Transmit(&huart1,
            (uint8_t*)"[LORA] Donnees envoyees OK\r\n", 28, 100);
        return 1;
    }

    HAL_UART_Transmit(&huart1,
        (uint8_t*)"[LORA] Echec envoi (module non connecte ?)\r\n", 44, 100);
    return 0;
}
