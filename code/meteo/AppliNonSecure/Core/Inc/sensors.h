#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  Structure partagée — remplie par sensors_read()                   */
/*                     — lue par display_data()                       */
/* ------------------------------------------------------------------ */
typedef struct {
    int32_t temperature;   /* 0.1 °C  → 235 = 23.5 °C           */
    int32_t humidity;      /* 0.1 %   → 612 = 61.2 %             */
    int32_t pressure;      /* 0.1 hPa → 10132 = 1013.2           */
    int32_t lux_percent;   /* 0-100 %  (LightHZ MIKROE-990)      */
    int32_t is_day;        /* 1 = jour (lux > 5 %), 0 = nuit     */
} SensorData_t;

extern SensorData_t g_sensors;

void sensors_read(void);   /* Lit tous les capteurs */

#endif
