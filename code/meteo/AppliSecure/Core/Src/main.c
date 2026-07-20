/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c  (AppliSecure)
  * @brief          : Main program body — Secure world
  *                   STM32N657x0-Q  |  TrustZone enabled
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <stdio.h>

/* USER CODE BEGIN PD */
#define VECT_TAB_NS_OFFSET        0x00400U
#define VTOR_TABLE_NS_START_ADDR  (SRAM2_AXI_BASE_NS | VECT_TAB_NS_OFFSET)

/* Flag de synchronisation Secure → Non-Secure
 * Écrit EN DERNIER dans SystemIsolation_Config()
 * Offset 0x000 dans SRAM2 — avant la table de vecteurs NS (offset 0x400) */
/* Write via NS alias (0x24100000) — Secure alias (0x34100000) is RAZWI-W
 * because RISAF3 is configured NSEC=1 (NS-owned region). Secure code must
 * use the NS alias to generate a NS bus transaction that RISAF allows.    */
#define NPU_RIF_READY_FLAG_ADDR    ((volatile uint32_t *)(SRAM2_AXI_BASE_NS + 0x0000U))
#define NPU_RIF_READY_MAGIC        0xCAFEBABEU
/* Helpers for writing diagnostic values via NS alias */
#define SRAM2_NS_WRITE(off, val)  (*((volatile uint32_t *)(SRAM2_AXI_BASE_NS + (off))) = (val))
/* USER CODE END PD */

static void NonSecure_Init(void);
static void SystemIsolation_Config(void);
static void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();      /* configure PLL1 + IC6 pour NPU — jamais Error_Handler */
    SystemIsolation_Config();  /* configure RIF + écrit 0xCAFEBABE dans SRAM2_NS */
    HAL_SuspendTick();
    NonSecure_Init();
    while (1) {}
}

/**
  * @brief  Configure les horloges système pour AppliSecure.
  *
  *  Pourquoi AppliSecure doit reconfigurer :
  *    Le FSBL configure les PLLs et horloges, puis jump à AppliSecure via
  *    un appel de fonction (pas un reset HW). SystemInit() de AppliSecure
  *    configure le TrustZone (SAU), ce qui peut invalider l'état des horloges
  *    hérité du FSBL. AppliSecure doit donc posséder sa propre config horloge.
  *
  *  Séquence obligatoire (différence avec FSBL) :
  *    1. Bascule EXPLICITE sur HSI AVANT de toucher aux PLLs.
  *       Sans ça, HAL_RCC_OscConfig ne peut pas désactiver/reconfigurer PLL1
  *       car il vérifie si PLL1 est source du SYSCLK.
  *    2. Configure PLL1/PLL2 via HAL_RCC_OscConfig.
  *    3. HAL_RCC_ClockConfig bascule sur IC2/IC6/IC11 et gère le séquencement
  *       disable→write→enable de IC6 (impossible à faire manuellement car
  *       IC6CFGR est en lecture seule quand la sortie IC6 est active).
  *
  *  Résultat :  CPU 600 MHz | HCLK 200 MHz | AHB5/NPU 800 MHz
  */
static void SystemClock_Config(void)
{
    /* RÈGLE CRITIQUE : RISAF11-14 (NPU SRAMs) ne peut être configuré
     * dans SystemIsolation_Config que si AHB5/IC6 est déjà actif.
     * Donc on démarre PLL1 et on configure IC6 ICI, avant SystemIsolation_Config.
     *
     * HAL_SuspendTick n'a pas encore été appelé → HAL_Delay disponible.
     * À ce stade AppliSecure vient de démarrer : XSPI est idle (FSBL a fini),
     * donc IC6_Disable est sûr même si FSBL l'avait activé. */

    /* ── Étape 1 : démarrer PLL1 si nécessaire ─────────────────────────── */
    if (!LL_RCC_PLL1_IsReady())
    {
        LL_RCC_HSI_Enable();
        { volatile uint32_t t = 200000U; while (!LL_RCC_HSI_IsReady() && t) t--; }

        LL_RCC_PLL1_Disable();
        { volatile uint32_t t = 50000U; while (LL_RCC_PLL1_IsReady() && t) t--; }

        /* PLL1 : HSI(64 MHz)/2 × 75 = 2400 MHz, P1=1 → 2400 MHz */
        LL_RCC_PLL1_SetSource(LL_RCC_PLLSOURCE_HSI);
        LL_RCC_PLL1_SetM(2U);
        LL_RCC_PLL1_SetN(75U);
        LL_RCC_PLL1_SetFRACN(0U);
        LL_RCC_PLL1_SetP1(1U);
        LL_RCC_PLL1P_Enable();
        LL_RCC_PLL1_Enable();
        { volatile uint32_t t = 500000U; while (!LL_RCC_PLL1_IsReady() && t) t--; }
    }

    /* ── Étape 2 : IC6 = PLL1/3 = 800 MHz (AHB5/NPU) ──────────────────── */
    /* IC6_Disable est sûr ici : XSPI idle (FSBL a terminé), AHB5 sans
     * transaction active. Il déverrouille IC6CFGR pour écriture. */
    LL_RCC_IC6_Disable();
    LL_RCC_IC6_SetSource(LL_RCC_ICCLKSOURCE_PLL1);
    LL_RCC_IC6_SetDivider(3U);
    LL_RCC_IC6_Enable();
    /* Stabilisation AHB5 avant que SystemIsolation_Config écrive RISAF11-14 */
    HAL_Delay(2);
}

static void NonSecure_Init(void)
{
    funcptr_NS NonSecure_ResetHandler;
    uint32_t nonSecureStack = (*(uint32_t *)VTOR_TABLE_NS_START_ADDR);
    uint32_t nonSecureReset = (*(uint32_t *)(VTOR_TABLE_NS_START_ADDR + 4U));

    SCB_NS->VTOR = VTOR_TABLE_NS_START_ADDR;
    __TZ_set_MSP_NS(nonSecureStack);
    NonSecure_ResetHandler = (funcptr_NS)nonSecureReset;
    __DSB();
    __ISB();
    if ((nonSecureReset & 1U) == 0U) { Error_Handler(); }
    NonSecure_ResetHandler();
}

static void SystemIsolation_Config(void)
{
    /* USER CODE BEGIN RIF_Init 0 */
    /* USER CODE END RIF_Init 0 */

    /* ------------------------------------------------------------------ */
    /* A. Horloge RIFSC                                                    */
    /* ------------------------------------------------------------------ */
    __HAL_RCC_RIFSC_CLK_ENABLE();

    /* ------------------------------------------------------------------ */
    /* B. RIMC — Maître ETH1                                              */
    /* ------------------------------------------------------------------ */
    RIMC_MasterConfig_t RIMC_master = {0};
    RIMC_master.MasterCID = RIF_CID_1;
    RIMC_master.SecPriv   = RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV;
    HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_ETH1, &RIMC_master);

    /* ------------------------------------------------------------------ */
    /* C. RISC — Périphériques Secure                                     */
    /* ------------------------------------------------------------------ */
    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_LPUART1,
                                          RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);
    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_ETH1,
                                          RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);
    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_ADC12,
                                          RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);
    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_XSPI2,
                                          RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);
    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_XSPIM,
                                          RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);
    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_CSI,
                                          RIF_ATTRIBUTE_SEC | RIF_ATTRIBUTE_NPRIV);

    /* ------------------------------------------------------------------ */
    /* D. GPIO                                                             */
    /* ------------------------------------------------------------------ */
    HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_10, GPIO_PIN_SEC  | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOA, GPIO_PIN_11, GPIO_PIN_SEC  | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_0,  GPIO_PIN_SEC  | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_3,  GPIO_PIN_SEC  | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_6,  GPIO_PIN_SEC  | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_7,  GPIO_PIN_SEC  | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOE, GPIO_PIN_3,  GPIO_PIN_SEC  | GPIO_PIN_NPRIV);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_10, GPIO_PIN_NSEC | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOB, GPIO_PIN_11, GPIO_PIN_NSEC | GPIO_PIN_NPRIV);

    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_1,  GPIO_PIN_NSEC | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_2,  GPIO_PIN_NSEC | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_10, GPIO_PIN_NSEC | GPIO_PIN_NPRIV);
    HAL_GPIO_ConfigPinAttributes(GPIOC, GPIO_PIN_11, GPIO_PIN_NSEC | GPIO_PIN_NPRIV);

    __HAL_RCC_GPIOD_CLK_ENABLE();
    HAL_GPIO_ConfigPinAttributes(GPIOD, GPIO_PIN_5,  GPIO_PIN_NSEC | GPIO_PIN_NPRIV);

    __HAL_RCC_GPIOH_CLK_ENABLE();
    HAL_GPIO_ConfigPinAttributes(GPIOH, GPIO_PIN_9,  GPIO_PIN_NSEC | GPIO_PIN_NPRIV);

    /* USER CODE BEGIN RIF_Init 1 */

    /* ================================================================== */
    /* ÉTAPE 1 : Horloge NPU + FORCE_RESET (NPU en reset = aucun DMA)     */
    /* ================================================================== */
    __HAL_RCC_NPU_CLK_ENABLE();
    __HAL_RCC_NPU_FORCE_RESET();
    HAL_Delay(2);
    /* ⚠ PAS de RELEASE_RESET ici : on configure RISAF pendant que NPU   */
    /* est en reset pour éviter le freeze AXI (CID1 + NPU actif = hang). */

    /* ================================================================== */
    /* ÉTAPE 2 : HORLOGES MÉMOIRES AXI                                    */
    /* ================================================================== */
    __HAL_RCC_AXISRAM2_MEM_CLK_ENABLE();
    RCC->MEMENR |= RCC_MEMENR_AXISRAM3EN
                 | RCC_MEMENR_AXISRAM4EN
                 | RCC_MEMENR_AXISRAM5EN
                 | RCC_MEMENR_AXISRAM6EN;
    __HAL_RCC_CACHEAXI_CLK_ENABLE();
    RCC->MEMENR |= RCC_MEMENR_CACHEAXIRAMEN;
    HAL_Delay(2);

    /* ================================================================== */
    /* ÉTAPE 3 : WORKAROUND SILICON — lever le bit shutdown des SRAMs     */
    /* ================================================================== */
    RAMCFG_SRAM3_AXI->CR &= ~(1U << 20);
    RAMCFG_SRAM4_AXI->CR &= ~(1U << 20);
    RAMCFG_SRAM5_AXI->CR &= ~(1U << 20);
    RAMCFG_SRAM6_AXI->CR &= ~(1U << 20);

    /* ================================================================== */
    /* ÉTAPE 3 : RISAF — ouverture des régions mémoire pour CID1 NS       */
    /* ================================================================== */

    /* SRAM2 — 512Ko (vecteurs NS + stack + code NS + blob réseau).
     * CID0 (CPU NS) + CID1 (EpochCtrl DMA) — configuré pendant NPU FORCE_RESET
     * → aucun DMA actif pendant cette config → pas de freeze AXI.
     * Sans CID1 ici, l'EpochCtrl DMA (CID1) vers SRAM2 reçoit SLVERR → deadlock. */
    RISAF2->REG[0].STARTR  = 0x00000000U;
    RISAF2->REG[0].ENDR    = 0x0007FFFFU;
    RISAF2->REG[0].CIDCFGR = (3U << 16) | (3U << 0); /* CID0+CID1 r/w */
    RISAF2->REG[0].CFGR    = (1U << 3)  | (1U << 0); /* NSEC | EN */

    /* SRAM NPU interne (si RISAF3 présent) */
#if defined(RISAF3)
    RISAF3->REG[0].STARTR  = 0x00000000U;
    RISAF3->REG[0].ENDR    = 0x0000FFFFU;
    RISAF3->REG[0].CIDCFGR = (1U << 16) | (1U << 0);
    RISAF3->REG[0].CFGR    = (1U << 3)  | (1U << 0);
#endif

    /* RISAF11-14 : NON configurés (BREN=0 = pas de filtrage).
     * Sur STM32N6 après reset, BREN=0 = accès ouvert par défaut.
     * Cela permet au DMA ATON (n'importe quel CID) d'accéder à AXISRAM3-6
     * sans blocage RISAF. CPU NS peut aussi y accéder (test AXISRAM3_rb).
     * Si le test AXISRAM3_rb retourne 0 (RAZWI), il faudra reconfigurer. */




    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_CRC,
                                          RIF_ATTRIBUTE_NSEC | RIF_ATTRIBUTE_NPRIV);
    /*HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RCC_PERIPH_INDEX_RAMCFG,
                                          RIF_ATTRIBUTE_NSEC | RIF_ATTRIBUTE_NPRIV);*/


    /* ================================================================== */
    /* ÉTAPE 4 : RIFSC — périphérique NPU → Non-Secure                   */
    /* ================================================================== */
#ifdef RIFSC_PERIPH_INDEX_NPU
    HAL_RIF_RIFSC_SetPeripheralSecureAttributes(RIFSC_PERIPH_INDEX_NPU,
                                                RIF_ATTRIBUTE_NSEC | RIF_ATTRIBUTE_NPRIV);
#endif

    /* ================================================================== */
    /* ÉTAPE 5 : RIMC — maître NPU → CID0 (même compartiment que CPU NS) */
    /* NPU est encore en FORCE_RESET → safe pour changer CID.            */
    /* RISAF11-14 autorise CID0 → NPU DMA (CID0) peut accéder AXISRAM3-6 */
    /* ================================================================== */
    RIMC_MasterConfig_t NPU_master = {0};
    NPU_master.MasterCID = RIF_CID_0;   /* CID0 = même que CPU NS */
    NPU_master.SecPriv   = RIF_ATTRIBUTE_NSEC | RIF_ATTRIBUTE_NPRIV;
    HAL_RIF_RIMC_ConfigMasterAttributes(RIF_MASTER_INDEX_NPU, &NPU_master);

    /* ================================================================== */
    /* ÉTAPE 5b : RELEASE NPU reset — RIMC et RISAF sont maintenant prêts */
    /* ================================================================== */
    __HAL_RCC_NPU_RELEASE_RESET();
    HAL_Delay(10);

    /* ================================================================== */
    /* ÉTAPE 6 : RISC — esclave NPU → Non-Secure                         */
    /* ================================================================== */
    HAL_RIF_RISC_SetSlaveSecureAttributes(RIF_RISC_PERIPH_INDEX_NPU,
                                          RIF_ATTRIBUTE_NSEC | RIF_ATTRIBUTE_NPRIV);

    HAL_Delay(5);

    /* ================================================================== */
    /* ÉTAPE 7 : ROUTAGE IRQ NPU → monde Non-Secure                      */
    /* NPU0 est celui utilisé par LL_ATON_RT_RuntimeInit.                 */
    /* On route les 4 au cas où la lib en utilise plusieurs.              */
    /* ================================================================== */
    NVIC_SetTargetState(NPU0_IRQn);
    NVIC_SetTargetState(NPU1_IRQn);
    NVIC_SetTargetState(NPU2_IRQn);
    NVIC_SetTargetState(NPU3_IRQn);

    /* ================================================================== */
    /* ÉTAPE 8 : BARRIÈRES MÉMOIRE                                        */
    /* ================================================================== */
    __DSB();
    __DMB();
    __ISB();

    /* ================================================================== */
    /* ÉTAPE 9a : DUMP DIAGNOSTIQUE → SRAM2 (lisible depuis NS)          */
    /* ================================================================== */
    *((volatile uint32_t *)(SRAM2_AXI_BASE_NS + 0x0004U)) =
        RIFSC->RISC_SECCFGRx[3];    /* NPU = bit 10 : 0=NS, 1=S */
    *((volatile uint32_t *)(SRAM2_AXI_BASE_NS + 0x0008U)) =
        RIFSC->RISC_RCFGLOCKRx[3]; /* NPU lock bit 10 : 1=locked */

    /* TEST DÉCISIF : write 1 → readback depuis Secure
     * Si readback=1 : ATON core clock présent, write-latch fonctionne.
     * Si readback=0 : ATON core clock ABSENT → horloge NPU non configurée. */
    /* ── ÉTAPE 9 : Ecrire le flag de sync dans SRAM2 (NS alias) ──────────
     * NPU_BASE_NS / NPU_BASE_S probe-writes supprimés : si AHB5 n'a pas
     * de clock (IC6=0), tout accès au NPU AXI hang → flag jamais écrit.
     * Le NS sera notifié par le flag, sans preuve que le NPU répond.     */
    __DSB();
    __DMB();
    __ISB();
    *NPU_RIF_READY_FLAG_ADDR = NPU_RIF_READY_MAGIC;
    __DSB();
    __ISB();

    /* USER CODE END RIF_Init 1 */
    /* USER CODE BEGIN RIF_Init 2 */
    /* USER CODE END RIF_Init 2 */
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file; (void)line;
}
#endif
