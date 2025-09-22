/**
 * @file display7seg.h
 * @brief API para display 7 segmentos (4 dígitos, ánodo común) con multiplexado.
 *
 * Inicializa el hardware, carga valores en el buffer y refresca el display.
 * El punto decimal (DP) se muestra solo en el dígito 0 para el formato s.mmm.
 *
 * @note Llama frecuentemente a display7seg_refresh_once() o usa
 *       display7seg_show_ms_block() para mantener visible el display.
 * 
 * @author Luis Castillo Chicaiza
 */

#include <stdint.h>
#include <pico/stdlib.h>

/**
 * @brief Inicializa pines y estado interno del display.
 */
void display7seg_init(void);

/**
 * @brief Carga cuatro dígitos crudos en el buffer.
 * @param d0 D0 (izquierda)  — válidos: 0..9, 10=BLANK
 * @param d1 D1              — válidos: 0..9, 10=BLANK
 * @param d2 D2              — válidos: 0..9, 10=BLANK
 * @param d3 D3 (derecha)    — válidos: 0..9, 10=BLANK
 * @note Requiere refrescar con display7seg_refresh_once().
 */
void display7seg_set_raw(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);

/**
 * @brief Carga en el buffer un tiempo en formato s.mmm.
 * @param ms Milisegundos (0..9999). Se satura a 9999.
 * @note DP solo en el dígito 0. Requiere refresco periódico.
 */
void display7seg_set_s_mmm(uint16_t ms);

/**
 * @brief Refresca un dígito (multiplexado). Llamar muy seguido.
 */
void display7seg_refresh_once(void);

/**
 * @brief Muestra s.mmm de forma bloqueante durante hold_ms.
 * @param ms      Valor (0..9999)
 * @param hold_ms Duración de visualización en ms
 */
void display7seg_show_ms_block(uint16_t ms, uint16_t hold_ms);
