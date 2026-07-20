#ifndef PERIPH_INIT_H
#define PERIPH_INIT_H

/* ------------------------------------------------------------------ */
/*  Une seule fonction à appeler dans main.c                          */
/*  Elle initialise : GPIO, UART, I2C, HTS221, LPS22HH               */
/*  → Pour ajouter un périphérique : l'ajouter dans periph_init.c    */
/* ------------------------------------------------------------------ */
void periph_init_all(void);

#endif /* PERIPH_INIT_H */
