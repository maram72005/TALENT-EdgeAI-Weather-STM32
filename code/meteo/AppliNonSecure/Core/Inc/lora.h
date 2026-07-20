#ifndef LORA_H
#define LORA_H

#include "stm32n6xx_hal.h"
#include <stdint.h>

/* UART du module LoRa : USART3 sur D0(PD9=RX) et D1(PD8=TX) */
extern UART_HandleTypeDef huart3;

void lora_uart_init(void);
int  lora_at_cmd(const char *cmd, const char *expected_resp, uint32_t timeout_ms);
int  lora_join(void);
int  lora_send_data(int temp10, int hum10, int pres10, int alt_m, int pct_pluie, int is_day, int lux_pct);

#endif
