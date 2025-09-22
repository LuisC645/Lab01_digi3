#pragma once
#include <stdint.h>
#include <pico/stdlib.h>

/* Inicializa pines del display (ánodo común, 4 dígitos). */
void display7seg_init(void);

/* Carga el buffer con s.mmm a partir de ms (0..9999). */
void display7seg_set_s_mmm(uint16_t ms);

/* Refresca UN dígito (multiplexado). Llamar muy seguido en los bucles. */
void display7seg_refresh_once(void);

/* Muestra s.mmm de forma bloqueante durante hold_ms. */
void display7seg_show_ms_block(uint16_t ms, uint16_t hold_ms);
