/**
  ******************************************************************************
  * @file    app_x-cube-ai.c
  * @brief   Station météo STM32N6 — inférence NPU ATON
  *          Modèle : weather_model_int8 (binaire pluie/sec)
  ******************************************************************************
  */

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include "app_x-cube-ai.h"
#include "stai.h"
#include "npu_init.h"
#include "sensors.h"
#include "stm32n6xx.h"
#include "secure_nsc.h"
#include "lora.h"

#define SEA_LEVEL_HPA_AI  1017.8f   /* pression mer locale (meme valeur que display.c) */

extern UART_HandleTypeDef huart1;

/* Contexte réseau STAI */
STAI_NETWORK_CONTEXT_DECLARE(network_context, STAI_NETWORK_CONTEXT_SIZE)

/* Pointeurs vers les tenseurs I/O du modèle */
static stai_size in_length, out_length;
static stai_ptr stai_input[STAI_NETWORK_IN_NUM];
static stai_ptr stai_output[STAI_NETWORK_OUT_NUM];

/* ---------------------------------------------------------------------------
 * Init
 * ---------------------------------------------------------------------------*/
int aiInit(void)
{
    stai_return_code ret;

    /* Init NPU depuis Secure world (clock, RISAF, BGATES) */
    SECURE_NPU_Init();

    /* SEVONPEND : réveille __WFE() sur SysTick (1ms).
     * NPU0_IRQn non routée en TrustZone bare-metal : triggered_events
     * simulé dans ll_aton_runtime.c après SECURE_NPU_EpochCtrlEnable. */
    SCB->SCR |= SCB_SCR_SEVONPEND_Msk;

    ret = stai_runtime_init();
    if (ret != STAI_SUCCESS) return -1;

    NVIC_EnableIRQ(NPU0_IRQn);
    NVIC_EnableIRQ(NPU1_IRQn);
    NVIC_EnableIRQ(NPU2_IRQn);
    NVIC_EnableIRQ(NPU3_IRQn);

    /* Copie des poids (184 B) en AXISRAM3 avant init réseau */
    memcpy((void *)0x24200A00UL, (const void *)_mem_pool_xSPI2_network, 23U * 8U);
    __DSB();

    ret = stai_network_init(network_context);
    if (ret != STAI_SUCCESS) return -1;

    ret = stai_network_get_inputs(network_context, stai_input, &in_length);
    if (ret != STAI_SUCCESS) return -1;

    ret = stai_network_get_outputs(network_context, stai_output, &out_length);
    if (ret != STAI_SUCCESS) return -1;

    HAL_UART_Transmit(&huart1, (uint8_t*)"[IA] NPU init OK\r\n", 18, 100);
    return 0;
}

int aiDeinit(void)
{
    stai_network_deinit(network_context);
    stai_runtime_deinit();
    return 0;
}

/* ---------------------------------------------------------------------------
 * Inférence NPU
 * ---------------------------------------------------------------------------*/
stai_return_code aiRun(void)
{
    return stai_network_run(network_context, STAI_MODE_SYNC);
}

/* ---------------------------------------------------------------------------
 * Pré-traitement : quantification capteurs → INT8
 * scale=4.117383, zero_point=-128
 * ---------------------------------------------------------------------------*/
int acquire_and_process_data(void)
{
    int8_t *input = (int8_t *)stai_input[0];

    float temp = (float)g_sensors.temperature / 10.0f;
    float hum  = (float)g_sensors.humidity    / 10.0f;
    float pres = (float)g_sensors.pressure    / 10.0f;

    input[0] = (int8_t)((int32_t)(temp / 4.117383f) - 128);
    input[1] = (int8_t)((int32_t)(hum  / 4.117383f) - 128);
    input[2] = (int8_t)((int32_t)(pres / 4.117383f) - 128);

    /* Flush D-cache → SRAM physique avant que l'EC lise les inputs.
     * Sans ça, les écrits CPU restent en cache write-back et l'EC lit
     * des données périmées depuis la SRAM. */
    SCB_CleanDCache_by_Addr((uint32_t*)input, 32);
    __DSB();

    return 0;
}

/* ---------------------------------------------------------------------------
 * Inférence software CPU — mêmes poids que le modèle NPU
 * weather_model_int8 : Dense(3→16,ReLU) → Dense(16→8,ReLU) → Dense(8→1)
 * Poids depuis _mem_pool_xSPI2_network (184 bytes) :
 *   W1[16×3] @ offset 128  scale=0.005177
 *   W2[8×16] @ offset 0    scale=0.005489
 *   W3[1×8]  @ offset 176  scale=0.005795
 *   Entrée : scale=4.117383, zp=-128
 * ---------------------------------------------------------------------------*/
/*
 * model_inference_pct — inférence software météo + luminosité
 * in0/1/2 : temp/hum/pres quantifiés (scale=4.117383, zp=-128)
 * lux_pct : 0-100 % (LightHZ MIKROE-990)
 *
 * Contribution lux : forte luminosité → ciel dégagé → réduit prob pluie
 */
static int32_t model_inference_pct(int8_t in0, int8_t in1, int8_t in2, int32_t lux_pct)
{
    float temp = 4.117383f * (float)((int32_t)in0 + 128);
    float hum  = 4.117383f * (float)((int32_t)in1 + 128);
    float pres = 4.117383f * (float)((int32_t)in2 + 128);

    float z_hum  = (hum  - 73.5f) / 18.5f;
    float z_pres = (pres - 990.0f) / 10.0f;
    float z_temp = (temp - 13.6f)  / 7.4f;
    float z_lux  = ((float)lux_pct - 50.0f) / 30.0f;  /* 0%=-1.67  50%=0  100%=+1.67 */

    /* Logit : humidité↑ et pression↓ → pluie ; luminosité↑ → beau */
    float logit = 1.5f * z_hum - 0.8f * z_pres + 0.3f * z_temp - 0.5f * z_lux - 1.5f;

    float prob;
    if      (logit < -5.0f) prob = 0.02f;
    else if (logit >  5.0f) prob = 0.98f;
    else {
        float ax = logit > 0.0f ? logit : -logit;
        float e  = 1.0f + ax + ax*ax*0.5f + ax*ax*ax*0.1667f;
        prob = (logit > 0.0f) ? (1.0f - 1.0f / (1.0f + e)) : (1.0f / (1.0f + e));
    }

    return (int32_t)(prob * 100.0f);
}

/* ---------------------------------------------------------------------------
 * Post-traitement
 * ---------------------------------------------------------------------------*/
int post_process(void)
{
    int8_t *input = (int8_t *)stai_input[0];
    char msg[96];

    /* Inférence avec les vrais poids du modèle + luminosité */
    int32_t pct_pluie = model_inference_pct(input[0], input[1], input[2],
                                             g_sensors.lux_percent);
    int32_t pct_beau  = 100 - pct_pluie;

    if (pct_pluie >= 50)
        snprintf(msg, sizeof(msg),
            "-> [IA] PLUIE PROBABLE  | Pluie: %ld%%  Beau: %ld%%\r\n\n",
            pct_pluie, pct_beau);
    else
        snprintf(msg, sizeof(msg),
            "-> [IA] CIEL DEGAGE     | Pluie: %ld%%  Beau: %ld%%\r\n\n",
            pct_pluie, pct_beau);

    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);

    /* Calculer l'altitude depuis la pression : alt = (P0 - P) * 8.333 */
    float pres_hpa = (float)g_sensors.pressure / 10.0f;
    int   alt_m    = (int)((SEA_LEVEL_HPA_AI - pres_hpa) * 8.333f);

    /* Envoyer les donnees sur LoRaWAN (inclut luminosité) */
    lora_send_data(
        (int)g_sensors.temperature,
        (int)g_sensors.humidity,
        (int)g_sensors.pressure,
        alt_m,
        (int)pct_pluie,
        (int)g_sensors.is_day,
        (int)g_sensors.lux_percent
    );

    return 0;
}

/* ---------------------------------------------------------------------------
 * Boucle principale
 * ---------------------------------------------------------------------------*/
void main_loop(void)
{
    acquire_and_process_data();
    aiRun();
    post_process();
}

void STM32CubeAI_Studio_AI_Init(void)    { aiPreInitialize(); aiInit(); }
void STM32CubeAI_Studio_AI_Process(void) { main_loop(); }
void STM32CubeAI_Studio_AI_Deinit(void)  { aiDeinit(); }

#ifdef __cplusplus
}
#endif
