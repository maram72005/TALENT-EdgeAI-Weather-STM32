/**
  ******************************************************************************
  * @file    npu_init.c
  * @author  GPM/AIS Application Team
  * @brief   Collection of functions to perform main configurations in main.c
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include <string.h>     // Used for memset

#include "npu_init.h"
#include "app_config.h"
#include "npu_cache.h"  // Used in NPU_config

#define USE_NPU_CACHE  /* Requis : EpochCtrl DMA passe par CACHEAXI (ATON_EPOCHCTRL_CACHE_ENABLED=1) */

void NPU_Config(void)
{
#ifdef USE_NPU_CACHE
  npu_cache_enable();
#endif
}

void Set_RISAF_Default(RISAF_TypeDef *risaf)
{
  /* Supprimé : Non-Secure n'a pas accès à la configuration RISAF */
  UNUSED(risaf);
}

void RIF_Config(void)
{
  /* Supprimé : Géré en amont par le FSBL Secure */
}

void SystemInit_POST(void)
{
    /* RCC (0x40090000 Secure) is already configured by the Secure world:
     *   - __HAL_RCC_SYSCFG_CLK_ENABLE()
     *   - __HAL_RCC_CRC_CLK_ENABLE()
     *   - __HAL_RCC_NPU_CLK_ENABLE()
     *   - RCC->MEMENR AXISRAM3-6 + CACHEAXIRAM
     * Any attempt to write RCC from NonSecure raises BusFault. */

    /* RAMCFG workaround: lever le bit shutdown des SRAMs NPU.
     * RAMCFG est accessible depuis NonSecure. */
    RAMCFG_SRAM3_AXI->CR &= ~(1U << 20);
    RAMCFG_SRAM4_AXI->CR &= ~(1U << 20);
    RAMCFG_SRAM5_AXI->CR &= ~(1U << 20);
    RAMCFG_SRAM6_AXI->CR &= ~(1U << 20);

    __DSB(); __ISB();
    /* NPU_Config() retiré d'ici : il appelle HAL_CACHEAXI_Init() qui
     * nécessite SysTick (HAL_GetTick). Appelé depuis startup.s avant
     * HAL_Init(), HAL_GetTick() retourne toujours 0 → hang infini. */
}

void aiPreInitialize(void)
{
    /* SYSCFG et CRC : accessibles via RCC_NS (0x46028000). */
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_CRC_CLK_ENABLE();

    /* Workaround RAMCFG (sans NPU_Config). */
    SystemInit_POST();

    /* NPU_Config ici : HAL_Init() a déjà été appelé, SysTick actif,
     * HAL_CACHEAXI_Init() peut utiliser HAL_GetTick() normalement. */
    NPU_Config();

    __DSB();
    __ISB();
    HAL_Delay(5);
}
