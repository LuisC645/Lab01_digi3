#ifndef DISPLAY7SEG_H
#define DISPLAY7SEG_H

#include <stdbool.h>
#include <stdint.h>

// Inicializa GPIO de segmentos y dígitos
void disp_init(void);

// Muestra un valor en milisegundos (0..9999) como x.xxx (s.mmm)
// manteniéndolo visible con multiplexado durante hold_ms milisegundos.
void disp_show_ms_block(int ms_total, int hold_ms);

// (Opcional) Muestra crudo cuatro dígitos y control de punto decimal por dígito.
// digits[0..3] = 0..9 ; dots[0..3] = true/false
void disp_show_raw_block(const int digits[4], const bool dots[4], int hold_ms);

#endif // DISPLAY7SEG_H
