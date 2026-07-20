#ifndef LIGHTHZ_H
#define LIGHTHZ_H

#include <stdint.h>

/*
 * Driver LightHZ Click Board MIKROE-990 (TSL230BR)
 * GPIO MikroBUS INT : PF7 (board mike1581-05)
 *
 * Calibrage mesure (jumpers defaut : 100x sensibilite, /100 divider) :
 *   Obscurite totale  →     0 Hz =   0%
 *   Flash telephone   → 36900 Hz =  92%
 *   Plein soleil ext. → ~80000 Hz = 100%
 */
#define LIGHTHZ_MAX_FREQ_HZ   5000UL

void     lighthz_init(void);
uint32_t lighthz_read_freq_hz(void);   /* frequence mesuree sur 100 ms */
uint8_t  lighthz_read_percent(void);   /* 0-100 %                       */
void     lighthz_scan_gpios(void);     /* scan GPIO (utilitaire debug)  */

#endif /* LIGHTHZ_H */
