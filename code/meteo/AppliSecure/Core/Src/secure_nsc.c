/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    Secure/Src/secure_nsc.c
  * @author  MCD Application Team
  * @brief   This file contains the non-secure callable APIs (secure world)
  ******************************************************************************
    * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* USER CODE BEGIN Non_Secure_CallLib */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "secure_nsc.h"
/** @addtogroup STM32N6xx_HAL_Examples

  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Global variables ----------------------------------------------------------*/
void *pSecureFaultCallback = NULL;   /* Pointer to secure fault callback in Non-secure */
void *pSecureErrorCallback = NULL;   /* Pointer to secure error callback in Non-secure */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Secure registration of non-secure callback.
  * @param  CallbackId  callback identifier
  * @param  func        pointer to non-secure function
  * @retval None
  */
  CMSE_NS_ENTRY void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func)
  {
      if(func != NULL)
      {
        switch(CallbackId)
        {
          case SECURE_FAULT_CB_ID:           /* SecureFault Interrupt occurred */
          pSecureFaultCallback = func;
          break;
          case IAC_ERROR_CB_ID:             /* Illegal Access Interrupt occurred */
          pSecureErrorCallback = func;
          break;
          default:
          /* unknown */
          break;
        }
      }
  }

/**
  * @brief  NSC: Initialize NPU clocks and RIF — callable from NonSecure world.
  *         Enables NPU AHB5 clock (Secure RCC), configures RISC/RIMC for NPU
  *         as NonSecure, and routes NPU0-3 IRQs to NonSecure NVIC.
  *         Safe to call multiple times (writes are idempotent).
  * @retval 0 success, -1 PLL1 not running (IC6 = 0, NPU will have no clock)
  */
CMSE_NS_ENTRY int32_t SECURE_NPU_Init(void)
{
    /* PLL1 n'est pas prêt → IC6 = 0 Hz → AHB5/XSPI à l'arrêt.
     * Dans cet état, LL_RCC_IC6_Disable() est sûr (aucune transaction
     * AXI active sur AHB5). On démarre PLL1 et IC6 ici.
     *
     * Si PLL1 EST déjà prêt (FSBL complet), on se contente de
     * garantir qu'IC6 et le clock gate NPU sont actifs. */

    /* PLL1 et IC6 sont configurés par SystemClock_Config d'AppliSecure.
     * RISAF11-14 sont ouverts par SystemIsolation_Config.
     * Cette fonction NSC est appelée après HAL_SuspendTick → pas de HAL_Delay.
     * Elle ne fait que s'assurer que les clock gates NPU sont actifs. */
    LL_RCC_IC6_Enable();                  /* idempotent si déjà actif */
    __HAL_RCC_NPU_CLK_ENABLE();
    __HAL_RCC_AXISRAM3_MEM_CLK_ENABLE();
    __HAL_RCC_AXISRAM4_MEM_CLK_ENABLE();
    __HAL_RCC_AXISRAM5_MEM_CLK_ENABLE();
    __HAL_RCC_AXISRAM6_MEM_CLK_ENABLE();

    __DSB(); __ISB();

    /* BGATES reset=0x0 (tous clocks gated off) ; NS ne peut pas écrire BGATES
     * (RAZWI). LL_ATON_Init() avec CLOCK_GATING=0 essaie 0xFFFFFFFF → RAZWI.
     * On force tous les block clocks ON depuis Secure avant toute init ATON. */
    volatile uint32_t *bgates = (volatile uint32_t *)(0x580E0000UL + 0x10UL);
    *bgates = 0xFFFFFFFFU;
    __DSB();

    return LL_RCC_PLL1_IsReady() ? 0 : -1;
}

/**
 * @brief  Active l'EpochCtrl NPU depuis Secure world.
 *
 * Depuis NS, ATON_ENABLE(EPOCHCTRL) provoque un deadlock AXI BRESP :
 * le hardware tient BRESP pendant que le DMA EPOCHCTRL essaie d'accéder
 * au blob via CACHEAXI (qui ne sert pas les adresses SRAM/AXISRAM).
 * Depuis Secure, les transactions AXI ont des attributs différents qui
 * évitent ce deadlock (Secure a accès direct à toutes les mémoires).
 *
 * Registres EPOCHCTRL via alias Secure NPU (NPU_BASE_S=0x580E0000) :
 *   EPOCHCTRL_BASE(0) = NPU_BASE_S + 0x1E000 = 0x581E0000
 *   CTRL offset = 0x0, ADDR offset = 0x8
 */
CMSE_NS_ENTRY int32_t SECURE_NPU_EpochCtrlEnable(uint32_t blob_addr)
{
    /* Registres EPOCHCTRL via alias Secure (NPU_BASE_S=0x580E0000) :
     *   EPOCHCTRL.CTRL @ 0x581E0000 : EN=bit0
     *   EPOCHCTRL.ADDR @ 0x581E0008
     *   EPOCHCTRL.IRQ  @ 0x581E000C : SM_trigger=bit16
     *
     * EN=1 depuis NS → deadlock CACHEAXI BRESP.
     * SM trigger depuis NS → même deadlock.
     * Depuis Secure : pas de deadlock. */
    volatile uint32_t *ec_ctrl = (volatile uint32_t *)0x581E0000UL;
    volatile uint32_t *ec_addr = (volatile uint32_t *)0x581E0008UL;
    volatile uint32_t *ec_irq  = (volatile uint32_t *)0x581E000CUL;

    /* Convertir l'adresse NS du blob (0x24xxxxxx) en alias Secure (0x34xxxxxx).
     * Le DMA de l'EpochCtrl est un maître Secure : il accède directement à
     * 0x34200000 (AXISRAM3 Secure) sans passer par CACHEAXI.
     * Avec l'alias NS (0x24200000), le DMA Secure pourrait ne pas voir la mémoire. */
    uint32_t secure_blob_addr = blob_addr | 0x10000000U;

    *ec_ctrl = 0x00000000U;        /* EN=0 : reset */
    __DSB();
    *ec_addr = secure_blob_addr;   /* blob à l'adresse Secure AXISRAM3 */
    __DSB();
    *ec_ctrl = 0x00000001U;        /* EN=1 : arme l'EpochCtrl */
    __DSB();
    *ec_irq  = (1U << 16);         /* SM trigger : déclenche l'exécution */
    __DSB();
    __ISB();

    return 0;
}

/**
 * @brief  Attend la fin d'exécution de l'EpochCtrl depuis Secure world.
 * @retval 0 = OK, -1 = EC jamais démarré, -2 = EC jamais terminé (timeout)
 */
CMSE_NS_ENTRY int32_t SECURE_NPU_EpochCtrlWait(void)
{
    volatile uint32_t *ec_ctrl = (volatile uint32_t *)0x581E0000UL;
    volatile uint32_t timeout;

    /* Attendre RUNNING=1 (EC démarré) — timeout ~5ms */
    timeout = 3000000U;
    while (!((*ec_ctrl >> 31) & 1U) && timeout) { timeout--; }
    if (timeout == 0U) { return -1; } /* EC jamais démarré */

    /* Attendre RUNNING=0 (EC terminé) — timeout ~50ms */
    timeout = 30000000U;
    while (((*ec_ctrl >> 31) & 1U) && timeout) { timeout--; }
    if (timeout == 0U) { return -2; } /* EC jamais terminé */

    __DSB();
    return 0;
}

/**
  * @}
  */

/**
  * @}
  */
/* USER CODE END Non_Secure_CallLib */

