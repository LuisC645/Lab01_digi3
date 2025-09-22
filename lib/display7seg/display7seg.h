#pragma once
#include <stdint.h>
#include <pico/stdlib.h>

/**
 * Inicializa los pines del display de 7 segmentos (4 dígitos).
 * Configura segmentos como salida y dígitos como salida.
 */
void display7seg_init(void);

/**
 * Muestra un número entero (0–9999) en el display.
 * Refresca un ciclo completo de los 4 dígitos (se debe llamar rápido, en loop).
 */
void display7seg_show_number(uint16_t value);

/**
 * Muestra un valor de tiempo en milisegundos (0–9999 ms).
 * No bloquea, solo carga los dígitos.
 */
static inline void display7seg_show_ms(uint16_t ms) {
    display7seg_show_number(ms);
}

/**
 * Muestra un valor en milisegundos durante un tiempo fijo (bloqueante).
 * @param ms valor a mostrar (0–9999)
 * @param hold_ms tiempo en ms a mantenerlo visible
 */
void display7seg_show_ms_block(uint16_t ms, uint16_t hold_ms);
