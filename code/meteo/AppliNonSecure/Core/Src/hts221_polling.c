#include <stdio.h>
#include <string.h>
#include "stm32n6xx_hal.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern int32_t last_temp;  // en °C × 10 (ex: 215 = 21.5°C)
extern int32_t last_rhum;  // en %RH × 10 (ex: 456 = 45.6%)

#define HTS221_ADDR  (0x5F << 1)

static int16_t H0_rH_x2, H1_rH_x2;
static int16_t H0_T0_OUT, H1_T0_OUT;
static int16_t T0_degC_x8, T1_degC_x8;
static int16_t T0_OUT, T1_OUT;

int hts221_init(void) {
    uint8_t id = 0;
    uint8_t buf[2];
    char msg[100];

    HAL_UART_Transmit(&huart1, (uint8_t*)"[HTS221] Init...\r\n", 18, 100);

    if (HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x0F, I2C_MEMADD_SIZE_8BIT, &id, 1, 1000) != HAL_OK || id != 0xBC) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"[HTS221] ERREUR ID\r\n", 20, 100);
        return -1;
    }
    HAL_UART_Transmit(&huart1, (uint8_t*)"[HTS221] ID OK. Calibration...\r\n", 32, 100);

    /* CALIBRATION HUMIDITE */
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x30, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    H0_rH_x2 = buf[0];
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x31, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    H1_rH_x2 = buf[0];
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x36, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    H0_T0_OUT = buf[0];
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x37, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    H0_T0_OUT |= (int16_t)(buf[0] << 8);
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x3A, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    H1_T0_OUT = buf[0];
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x3B, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    H1_T0_OUT |= (int16_t)(buf[0] << 8);

    sprintf(msg, "   H0=%d/2 %%RH  H1=%d/2 %%RH\r\n", H0_rH_x2, H1_rH_x2);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

    /* CALIBRATION TEMPERATURE */
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x32, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    T0_degC_x8 = buf[0];
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x33, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    T1_degC_x8 = buf[0];

    uint8_t msb;
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x35, I2C_MEMADD_SIZE_8BIT, &msb, 1, 1000);
    T0_degC_x8 |= ((msb & 0x03) << 8);
    T1_degC_x8 |= ((msb & 0x0C) << 6);

    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x3C, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    T0_OUT = buf[0];
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x3D, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    T0_OUT |= (int16_t)(buf[0] << 8);
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x3E, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    T1_OUT = buf[0];
    HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x3F, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);
    T1_OUT |= (int16_t)(buf[0] << 8);

    sprintf(msg, "   T0=%d/8 C  T1=%d/8 C\r\n", T0_degC_x8, T1_degC_x8);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

    /* ACTIVATION : Power ON + 1Hz */
    buf[0] = 0x85;
    HAL_I2C_Mem_Write(&hi2c1, HTS221_ADDR, 0x20, I2C_MEMADD_SIZE_8BIT, buf, 1, 1000);

    HAL_UART_Transmit(&huart1, (uint8_t*)"[HTS221] Initialise OK\r\n", 24, 100);
    return 0;
}

void hts221_read_temp(void) {
    uint8_t lsb = 0, msb = 0;

    if (HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x2A, I2C_MEMADD_SIZE_8BIT, &lsb, 1, 500) != HAL_OK ||
        HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x2B, I2C_MEMADD_SIZE_8BIT, &msb, 1, 500) != HAL_OK) {
        last_temp = -9990;
        return;
    }

    int16_t raw = (int16_t)((msb << 8) | lsb);
    int32_t den = (int32_t)(T1_OUT - T0_OUT);
    if (den == 0) { last_temp = -9980; return; }

    int32_t num    = (int32_t)(raw - T0_OUT) * (int32_t)(T1_degC_x8 - T0_degC_x8);
    int32_t temp_x8 = (int32_t)T0_degC_x8 + (num / den);

    // temp_x8 en °C×8 → on veut °C×10 : ×10/8
    last_temp = (temp_x8 * 10) / 8;
}

void hts221_read_hum(void) {
    uint8_t lsb = 0, msb = 0;

    if (HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x28, I2C_MEMADD_SIZE_8BIT, &lsb, 1, 500) != HAL_OK ||
        HAL_I2C_Mem_Read(&hi2c1, HTS221_ADDR, 0x29, I2C_MEMADD_SIZE_8BIT, &msb, 1, 500) != HAL_OK) {
        last_rhum = -9990;
        return;
    }

    int16_t raw = (int16_t)((msb << 8) | lsb);
    int32_t den = (int32_t)(H1_T0_OUT - H0_T0_OUT);
    if (den == 0) { last_rhum = -9980; return; }

    int32_t num    = (int32_t)(raw - H0_T0_OUT) * (int32_t)(H1_rH_x2 - H0_rH_x2);
    int32_t hum_x2 = (int32_t)H0_rH_x2 + (num / den);

    // hum_x2 en %RH×2 → on veut %RH×10 : ×10/2
    last_rhum = (hum_x2 * 10) / 2;

    if (last_rhum < 0)    last_rhum = 0;
    if (last_rhum > 1000) last_rhum = 1000;
}
