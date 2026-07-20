/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    Secure_nsclib/secure_nsc.h
  * @author  MCD Application Team
  * @brief   Header for secure non-secure callable APIs list
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

/* USER CODE BEGIN Non_Secure_CallLib_h */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SECURE_NSC_H
#define SECURE_NSC_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  non-secure callback ID enumeration definition
  */
typedef enum
{
SECURE_FAULT_CB_ID     = 0x00U, /*!< System secure fault callback ID */
  IAC_ERROR_CB_ID       = 0x01U  /*!< Illegal access secure error callback ID */
} SECURE_CallbackIDTypeDef;
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func);

/**
 * @brief  Initialize NPU clocks and RIF from Non-Secure world.
 *         Called by AppliNonSecure at AI startup. Enables NPU clock (RCC),
 *         configures RISC/RISAF/RIMC for NPU access, routes NPU IRQs to NS.
 *         Safe to call even if AppliSecure already ran (idempotent).
 * @retval 0 on success, -1 if NPU clock could not be enabled
 */
int32_t SECURE_NPU_Init(void);

/**
 * @brief  Active l'EpochCtrl NPU depuis le monde Secure (évite le deadlock CACHEAXI NS).
 *         En mode NS, ATON_ENABLE cause un deadlock AXI car CACHEAXI ne sert pas SRAM/AXISRAM.
 *         Depuis Secure, les transactions AXI passent correctement.
 * @param  blob_addr Adresse du blob EpochCtrl (ex: 0x24200000 pour AXISRAM3)
 * @retval 0 succès
 */
int32_t SECURE_NPU_EpochCtrlEnable(uint32_t blob_addr);

/**
 * @brief  Attend la fin d'exécution de l'EpochCtrl (polling depuis Secure world).
 *         À appeler APRÈS SECURE_NPU_EpochCtrlEnable pour savoir quand l'EC a terminé.
 * @retval 0 = OK, -1 = EC jamais démarré (timeout), -2 = EC jamais terminé (timeout)
 */
int32_t SECURE_NPU_EpochCtrlWait(void);

#endif /* SECURE_NSC_H */
/* USER CODE END Non_Secure_CallLib_h */

