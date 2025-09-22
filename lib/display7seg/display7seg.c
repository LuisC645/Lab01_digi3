#include "display7seg.h"
#include <pico/time.h>

// =================== CONFIGURA TUS PINES AQUÍ ===================
// Segmentos en orden: a,b,c,d,e,f,g,dp
static const uint SEG_PINS[8] = {2,3,4,5,6,7,8,9};
// Dígitos en orden: DIG1,DIG2,DIG3,DIG4
static const uint DIG_PINS[4] = {10,11,12,13};
// ================================================================

// Lógica para ÁNODO COMÚN
#define SEG_ON   0   // activo en bajo
#define SEG_OFF  1
#define DIG_ON   1   // activo en alto
#define DIG_OFF  0

// Tabla de segmentos (abcdefg + dp) para dígitos 0–9
static const uint8_t SEG_MAP[10] = {
    // pgfedcba
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

// Buffer actual de dígitos a mostrar
static uint8_t digits[4] = {0,0,0,0};

/**
 * Inicialización
 */
void display7seg_init(void) {
    for (int i = 0; i < 8; i++) {
        gpio_init(SEG_PINS[i]);
        gpio_set_dir(SEG_PINS[i], GPIO_OUT);
        gpio_put(SEG_PINS[i], SEG_OFF);
    }
    for (int i = 0; i < 4; i++) {
        gpio_init(DIG_PINS[i]);
        gpio_set_dir(DIG_PINS[i], GPIO_OUT);
        gpio_put(DIG_PINS[i], DIG_OFF);
    }
}

/**
 * Convierte un número a 4 dígitos
 */
static void split_number(uint16_t value) {
    if (value > 9999) value = 9999;
    digits[0] = (value / 1000) % 10;
    digits[1] = (value / 100)  % 10;
    digits[2] = (value / 10)   % 10;
    digits[3] = (value / 1)    % 10;
}

/**
 * Activa un dígito y apaga los demás
 */
static void enable_digit(int idx) {
    for (int i = 0; i < 4; i++) {
        gpio_put(DIG_PINS[i], (i == idx) ? DIG_ON : DIG_OFF);
    }
}

/**
 * Muestra un dígito en segmentos
 */
static void set_segments(uint8_t num) {
    uint8_t map = SEG_MAP[num];
    for (int i = 0; i < 7; i++) {
        bool on = (map >> i) & 0x01;
        gpio_put(SEG_PINS[i], on ? SEG_ON : SEG_OFF);
    }
    // punto decimal apagado por defecto
    gpio_put(SEG_PINS[7], SEG_OFF);
}

/**
 * Refresca el display con un número (un ciclo por los 4 dígitos)
 */
void display7seg_show_number(uint16_t value) {
    split_number(value);
    for (int d = 0; d < 4; d++) {
        set_segments(digits[d]);
        enable_digit(d);
        sleep_us(1000); // ~1 ms por dígito
    }
}

/**
 * Muestra un número fijo en ms, durante cierto tiempo
 */
void display7seg_show_ms_block(uint16_t ms, uint16_t hold_ms) {
    absolute_time_t t_end = make_timeout_time_ms(hold_ms);
    while (absolute_time_diff_us(get_absolute_time(), t_end) > 0) {
        display7seg_show_number(ms);
    }
}
