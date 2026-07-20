#include "lighthz.h"
#include "stm32n6xx_hal.h"
#include <string.h>
#include <stdio.h>

/*
 * LightHz Click Board MIKROE-990 — TSL230BR
 * Broche sortie frequence : MikroBUS INT = PE9 (board mike1581-05)
 * PF7 etait du bruit PWM (~22 kHz constant), pas le capteur
 */
#define LIGHTHZ_PORT   GPIOE
#define LIGHTHZ_PIN    GPIO_PIN_9

/* Fenetre de mesure en ms */
#define LIGHTHZ_WINDOW_MS   100U

void lighthz_init(void)
{
    /* PF7 configure en INPUT par _light_sensor_init() dans periph_init.c */
}

/*
 * Compte les fronts montants sur LIGHTHZ_WINDOW_MS ms.
 */
uint32_t lighthz_read_freq_hz(void)
{
    uint32_t count = 0;
    uint32_t t_end  = HAL_GetTick() + LIGHTHZ_WINDOW_MS;
    uint8_t  prev   = (HAL_GPIO_ReadPin(LIGHTHZ_PORT, LIGHTHZ_PIN) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t  curr;

    while (HAL_GetTick() < t_end)
    {
        curr = (HAL_GPIO_ReadPin(LIGHTHZ_PORT, LIGHTHZ_PIN) == GPIO_PIN_SET) ? 1U : 0U;
        if (curr == 1U && prev == 0U)
            count++;
        prev = curr;
    }
    return count * (1000U / LIGHTHZ_WINDOW_MS);
}

uint8_t lighthz_read_percent(void)
{
    uint32_t freq = lighthz_read_freq_hz();
    if (freq >= LIGHTHZ_MAX_FREQ_HZ)
        return 100U;
    return (uint8_t)((freq * 100UL) / LIGHTHZ_MAX_FREQ_HZ);
}

/* ---------------------------------------------------------------------------
 * Scan de toutes les broches candidates pour trouver le MikroBUS INT GPIO.
 * A appeler UNE SEULE FOIS au demarrage (dans periph_init_all).
 * Affiche sur UART1 la frequence mesuree sur chaque pin.
 * Le pin qui donne freq > 0 Hz est le bon.
 * ---------------------------------------------------------------------------*/
extern UART_HandleTypeDef huart1;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    const char   *name;
    uint32_t      clk_bit;   /* inutilise, clock activee globalement */
} PinCandidate_t;

void lighthz_scan_gpios(void)
{
    char msg[80];

    /* Activer les clocks de tous les ports testes */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    HAL_UART_Transmit(&huart1,
        (uint8_t*)"\r\n[SCAN] Recherche GPIO MikroBUS INT (LightHz OUT)...\r\n",
        54, 200);

    /* Liste des candidats probables pour Arduino D2/D5/D9 sur NUCLEO-N657X0-Q */
    static const struct { GPIO_TypeDef *port; uint16_t pin; const char *name; } cands[] = {
        { GPIOA, GPIO_PIN_0,  "PA0"  },
        { GPIOA, GPIO_PIN_1,  "PA1"  },
        { GPIOA, GPIO_PIN_2,  "PA2"  },
        { GPIOA, GPIO_PIN_3,  "PA3"  },
        { GPIOB, GPIO_PIN_1,  "PB1"  },
        { GPIOB, GPIO_PIN_2,  "PB2"  },
        { GPIOB, GPIO_PIN_4,  "PB4"  },
        { GPIOB, GPIO_PIN_8,  "PB8"  },
        { GPIOB, GPIO_PIN_9,  "PB9"  },
        { GPIOC, GPIO_PIN_6,  "PC6"  },
        { GPIOC, GPIO_PIN_7,  "PC7"  },
        { GPIOC, GPIO_PIN_8,  "PC8"  },
        { GPIOC, GPIO_PIN_9,  "PC9"  },
        { GPIOC, GPIO_PIN_12, "PC12" },
        { GPIOD, GPIO_PIN_0,  "PD0"  },
        { GPIOD, GPIO_PIN_1,  "PD1"  },
        { GPIOD, GPIO_PIN_3,  "PD3"  },
        { GPIOD, GPIO_PIN_4,  "PD4"  },
        { GPIOD, GPIO_PIN_7,  "PD7"  },
        { GPIOD, GPIO_PIN_11, "PD11" },
        { GPIOD, GPIO_PIN_12, "PD12" },
        { GPIOD, GPIO_PIN_13, "PD13" },
        { GPIOD, GPIO_PIN_14, "PD14" },
        { GPIOD, GPIO_PIN_15, "PD15" },
        { GPIOE, GPIO_PIN_1,  "PE1"  },
        { GPIOE, GPIO_PIN_2,  "PE2"  },
        { GPIOE, GPIO_PIN_4,  "PE4"  },
        { GPIOE, GPIO_PIN_7,  "PE7"  },
        { GPIOE, GPIO_PIN_8,  "PE8"  },
        { GPIOE, GPIO_PIN_9,  "PE9"  },
        { GPIOE, GPIO_PIN_10, "PE10" },
        { GPIOE, GPIO_PIN_11, "PE11" },
        { GPIOE, GPIO_PIN_12, "PE12" },
        { GPIOE, GPIO_PIN_13, "PE13" },
        { GPIOE, GPIO_PIN_14, "PE14" },
        { GPIOE, GPIO_PIN_15, "PE15" },
        { GPIOF, GPIO_PIN_0,  "PF0"  },
        { GPIOF, GPIO_PIN_1,  "PF1"  },
        { GPIOF, GPIO_PIN_6,  "PF6"  },
        { GPIOF, GPIO_PIN_7,  "PF7"  },
        { GPIOF, GPIO_PIN_8,  "PF8"  },
        { GPIOF, GPIO_PIN_9,  "PF9"  },
        { GPIOF, GPIO_PIN_10, "PF10" },
        { GPIOF, GPIO_PIN_11, "PF11" },
        { GPIOF, GPIO_PIN_12, "PF12" },
        { GPIOF, GPIO_PIN_13, "PF13" },
        { GPIOF, GPIO_PIN_14, "PF14" },
        { GPIOF, GPIO_PIN_15, "PF15" },
        { GPIOG, GPIO_PIN_1,  "PG1"  },
        { GPIOG, GPIO_PIN_2,  "PG2"  },
        { GPIOG, GPIO_PIN_3,  "PG3"  },
        { GPIOG, GPIO_PIN_4,  "PG4"  },
        { GPIOG, GPIO_PIN_5,  "PG5"  },
        { GPIOG, GPIO_PIN_6,  "PG6"  },
        { GPIOG, GPIO_PIN_7,  "PG7"  },
        { GPIOG, GPIO_PIN_9,  "PG9"  },
    };

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    uint32_t best_freq = 0;
    const char *best_name = "???";

    int n = (int)(sizeof(cands) / sizeof(cands[0]));
    for (int i = 0; i < n; i++)
    {
        /* Configurer la broche en entree */
        g.Pin = cands[i].pin;
        HAL_GPIO_Init(cands[i].port, &g);

        /* Mesurer la frequence sur 100 ms */
        uint32_t count = 0;
        uint32_t t_end = HAL_GetTick() + 100U;
        uint8_t  prev  = (HAL_GPIO_ReadPin(cands[i].port, cands[i].pin) == GPIO_PIN_SET) ? 1U : 0U;
        uint8_t  curr;
        while (HAL_GetTick() < t_end)
        {
            curr = (HAL_GPIO_ReadPin(cands[i].port, cands[i].pin) == GPIO_PIN_SET) ? 1U : 0U;
            if (curr == 1U && prev == 0U) count++;
            prev = curr;
        }
        uint32_t freq = count * 10U; /* *10 car fenetre=100ms */

        /* N'afficher que les pins qui recoivent un signal */
        if (freq > 50)
        {
            snprintf(msg, sizeof(msg), "[SCAN] *** %s : %lu Hz ***\r\n",
                     cands[i].name, freq);
            HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 200);
            if (freq > best_freq) { best_freq = freq; best_name = cands[i].name; }
        }
    }

    if (best_freq > 0)
    {
        snprintf(msg, sizeof(msg),
            "[SCAN] -> GPIO trouve : %s (%lu Hz)\r\n"
            "[SCAN] Mets LIGHTHZ_PORT=GPIOX et LIGHTHZ_PIN=GPIO_PIN_Y dans lighthz.c\r\n",
            best_name, best_freq);
    }
    else
    {
        snprintf(msg, sizeof(msg),
            "[SCAN] Aucun signal detecte. Verifie que la lumiere eclaire le capteur.\r\n"
            "[SCAN] Aussi verifier alim 3.3V sur le LightHz Click.\r\n");
    }
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 200);
    HAL_UART_Transmit(&huart1, (uint8_t*)"[SCAN] Scan termine.\r\n\r\n", 22, 100);
}
