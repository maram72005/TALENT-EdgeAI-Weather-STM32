#include <stdio.h>
#include <string.h>
#include "lps22hh_reg.h"
#include "stm32n6xx_hal.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart1;
extern int32_t last_pres;  // en hPa × 10 (ex: 10132 = 1013.2 hPa)

#define LPS22HH_ADDR  (0x5D << 1)
static stmdev_ctx_t pres_ctx;

static int32_t pres_write(void *h, uint8_t reg, const uint8_t *buf, uint16_t len) {
    return HAL_I2C_Mem_Write((I2C_HandleTypeDef *)h, LPS22HH_ADDR, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)buf, len, 1000);
}

static int32_t pres_read(void *h, uint8_t reg, uint8_t *buf, uint16_t len) {
    return HAL_I2C_Mem_Read((I2C_HandleTypeDef *)h, LPS22HH_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, len, 1000);
}

int pressure_init(void) {
    pres_ctx.write_reg = pres_write;
    pres_ctx.read_reg  = pres_read;
    pres_ctx.handle    = &hi2c1;
    pres_ctx.mdelay    = HAL_Delay;

    uint8_t id = 0;
    HAL_UART_Transmit(&huart1, (uint8_t*)"[LPS22HH] Init...\r\n", 19, 100);

    if (lps22hh_device_id_get(&pres_ctx, &id) != 0 || id != LPS22HH_ID) {
        HAL_UART_Transmit(&huart1, (uint8_t*)"[LPS22HH] ERREUR ID\r\n", 21, 100);
        return -1;
    }

    HAL_UART_Transmit(&huart1, (uint8_t*)"[LPS22HH] ID OK. Config...\r\n", 28, 100);

    lps22hh_reset_set(&pres_ctx, PROPERTY_ENABLE);
    HAL_Delay(50);
    lps22hh_block_data_update_set(&pres_ctx, PROPERTY_ENABLE);
    lps22hh_data_rate_set(&pres_ctx, LPS22HH_10_Hz);

    HAL_UART_Transmit(&huart1, (uint8_t*)"[LPS22HH] Initialise OK (10Hz)\r\n", 32, 100);
    return 0;
}

void pressure_read(void) {
    uint8_t buf[3] = {0, 0, 0};
    last_pres = 0;

    // Lecture des 3 octets de pression (registres 0x28, 0x29, 0x2A)
    int32_t ret = HAL_I2C_Mem_Read(&hi2c1, LPS22HH_ADDR, 0x28 | 0x80, I2C_MEMADD_SIZE_8BIT, buf, 3, 500);

    if (ret == HAL_OK) {
        uint32_t raw_p = ((uint32_t)buf[2] << 16) | ((uint32_t)buf[1] << 8) | buf[0];
        // Le LPS22HH donne 1 LSB = 1/4096 hPa
        // On veut hPa × 10 : raw * 10 / 4096
        last_pres = (int32_t)(raw_p * 10 / 4096);
    } else {
        last_pres = -9990;
    }
}
