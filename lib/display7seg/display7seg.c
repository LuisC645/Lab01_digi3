/**
 * @file display7seg.c
 * @brief Driver para display de 7 segmentos (4 dígitos, ánodo común) con multiplexación.
 *
 * - Segmentos a..g en GPIO 0..6 y punto decimal (DP) en GPIO 7.
 * - Dígitos habilitados por transistores PNP en alto-lado (GPIO 28,27,26,21).
 * - Formato s.mmm: el DP se enciende **solo** cuando se refresca el dígito 0.
 * - Multiplexado por software
 * @author Luis Castillo Chicaiza
 */

#include "display7seg.h"
#include <pico/time.h>
#include <hardware/gpio.h>

/** @name Mapeo de pines del display
 *  @brief Pines fijos de segmentos (a..g), dígitos (D0..D3) y punto decimal.
 *  @{
 */
static const uint SEG_PINS[7] = {0, 1, 2, 3, 4, 5, 6}; /**< a..g */
static const uint DIG_PINS[4] = {28, 27, 26, 21};     /**< D0..D3 (PNP alto-lado) */
static const uint DP_PIN      = 7;                    /**< Punto decimal (dp) */
/** @} */

/** @name Polaridades (ánodo común + PNP)
 *  @note En ánodo común, los segmentos se encienden con nivel bajo.
 *  @{
 */
#define SEG_ON   0  /**< Segmento activo (LED encendido) */
#define SEG_OFF  1  /**< Segmento inactivo (LED apagado) */
#define DIG_ON   0  /**< Dígito habilitado (PNP conduce con bajo) */
#define DIG_OFF  1  /**< Dígito deshabilitado */
/** @} */

/**
 * @brief Tabla de segmentos por dígito (bit0=a .. bit6=g). 1 = segmento encendido.
 * Índices: 0..9 y 10 = BLANK.
 */
static const uint8_t SEG_MAP[11] = {
    0x3F, /*0*/ 0x06, /*1*/ 0x5B, /*2*/ 0x4F, /*3*/
    0x66, /*4*/ 0x6D, /*5*/ 0x7D, /*6*/ 0x07, /*7*/
    0x7F, /*8*/ 0x67, /*9*/ 0x00  /*BLANK*/
};

/** Buffer de dígitos (0..9, 10=BLANK). */
static uint8_t digits[4] = {10,10,10,10};
/** Índice del dígito actual (0..3) en el barrido de multiplexado. */
static uint8_t cur_digit = 0;

/** @brief Tiempo de retención por dígito (µs) para brillo/Hz del refresco. */
#define DISPLAY7SEG_HOLD_US 800

/* -------------------- Helpers internos -------------------- */

/**
 * @brief Apaga todos los dígitos (corta la corriente del alto-lado).
 */
static inline void dig_all_off(void){
    for (int i=0;i<4;i++) gpio_put(DIG_PINS[i], DIG_OFF);
}

/**
 * @brief Habilita solo el dígito @p idx y deshabilita los demás.
 * @param idx Índice de dígito (0..3).
 */
static inline void dig_on(uint8_t idx){
    for (int i=0;i<4;i++) gpio_put(DIG_PINS[i], (i==idx)?DIG_ON:DIG_OFF);
}

/**
 * @brief Escribe el patrón de segmentos a..g según @p pattern.
 * @param pattern Bits (bit0=a .. bit6=g). 1 = encendido.
 * @note El DP se controla aparte en ::display7seg_refresh_once().
 */
static inline void seg_write(uint8_t pattern){
    for (int i=0;i<7;i++){
        bool on = (pattern >> i) & 1;
        gpio_put(SEG_PINS[i], on ? SEG_ON : SEG_OFF);
    }
}

/* -------------------- API pública -------------------- */

void display7seg_init(void){
    /* Segmentos a..g */
    for (int i=0;i<7;i++){
        gpio_init(SEG_PINS[i]);
        gpio_set_dir(SEG_PINS[i], GPIO_OUT);
        gpio_put(SEG_PINS[i], SEG_OFF);
    }

    /* Punto decimal (dp) */
    gpio_init(DP_PIN);
    gpio_set_dir(DP_PIN, GPIO_OUT);
    gpio_put(DP_PIN, SEG_OFF);

    /* Dígitos D0..D3 (alto-lado) */
    for (int i=0;i<4;i++){
        gpio_init(DIG_PINS[i]);
        gpio_set_dir(DIG_PINS[i], GPIO_OUT);
        gpio_put(DIG_PINS[i], DIG_OFF);
    }

    /* Estado inicial */
    for (int i=0;i<4;i++) digits[i]=10; /* BLANK */
    cur_digit=0;
}

void display7seg_set_raw(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3){
    /* Guarda valores 0..9 o BLANK(10) en el buffer interno. */
    digits[0] = (d0<=10)?d0:10;
    digits[1] = (d1<=10)?d1:10;
    digits[2] = (d2<=10)?d2:10;
    digits[3] = (d3<=10)?d3:10;
}

void display7seg_set_s_mmm(uint16_t ms){
    /* Convierte ms (0..9999) a s.mmm en los cuatro dígitos. */
    if (ms>9999) ms=9999;
    uint8_t  s = (uint8_t)(ms/1000);
    uint16_t r = (uint16_t)(ms%1000);
    digits[0] = s;
    digits[1] = (uint8_t)(r/100);
    digits[2] = (uint8_t)((r/10)%10);
    digits[3] = (uint8_t)(r%10);
}

void display7seg_refresh_once(void){
    /* Apaga todos los dígitos para evitar “ghosting”. */
    dig_all_off();

    /* Escribe segmentos del dígito actual. */
    uint8_t d = (digits[cur_digit]<=10)?digits[cur_digit]:10;
    seg_write(SEG_MAP[d]);

    /* DP solo en el dígito 0 (formato s.mmm). */
    gpio_put(DP_PIN, (cur_digit == 0) ? SEG_ON : SEG_OFF);

    /* Habilita el dígito actual y mantiene por HOLD_US. */
    dig_on(cur_digit);
    sleep_us(DISPLAY7SEG_HOLD_US);

    /* Avanza al siguiente dígito (0..3). */
    cur_digit = (cur_digit+1u)&0x03;
}

void display7seg_show_ms_block(uint16_t ms, uint16_t hold_ms){
    /* Muestra s.mmm de forma bloqueante durante hold_ms, refrescando el display. */
    absolute_time_t t_end = make_timeout_time_ms(hold_ms);
    display7seg_set_s_mmm(ms);
    while (absolute_time_diff_us(t_end, get_absolute_time()) > 0){
        display7seg_refresh_once();
    }
}
