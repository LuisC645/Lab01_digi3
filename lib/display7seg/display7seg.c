#include "display7seg.h"
#include <pico/time.h>

/* ========= AJUSTA ESTOS PINES A TU CONEXIÓN =========
   Segmentos en orden: a,b,c,d,e,f,g,dp
   Dígitos en orden:   D0 (izq) .. D3 (der) */
static const uint SEG_PINS[8] = {0, 1, 2, 3, 4, 5, 6};
static const uint DIG_PINS[4] = {28, 27, 26, 22};
/* ==================================================== */

/* Ánodo común: segmentos activos en BAJO, dígitos activos en ALTO */
#define SEG_ON   0
#define SEG_OFF  1
#define DIG_ON   1
#define DIG_OFF  0

/* Mapa de segmentos (a..g) invertido para ánodo común; dp se maneja aparte.
   Índices: 0..9, y 10 = BLANK */
static const uint8_t SEG_MAP_AC[11] = {
    (uint8_t)(~0x3F) & 0x7F, // 0
    (uint8_t)(~0x06) & 0x7F, // 1
    (uint8_t)(~0x5B) & 0x7F, // 2
    (uint8_t)(~0x4F) & 0x7F, // 3
    (uint8_t)(~0x66) & 0x7F, // 4
    (uint8_t)(~0x6D) & 0x7F, // 5
    (uint8_t)(~0x7D) & 0x7F, // 6
    (uint8_t)(~0x07) & 0x7F, // 7
    (uint8_t)(~0x7F) & 0x7F, // 8
    (uint8_t)(~0x67) & 0x7F, // 9
    0x00                     // 10 = BLANK
};

/* Buffer y estado interno */
static uint8_t digits[4] = {10,10,10,10};  // 10 = blank
static uint8_t dp_mask   = 0x00;           // bit i -> DP del dígito i
static uint8_t cur_digit = 0;              // 0..3
static const uint16_t HOLD_US = 1000;      // ~1 ms por dígito (~250 Hz total)

static inline void seg_write_raw(uint8_t pattern, bool dp_on) {
    /* a..g */
    for (int i = 0; i < 7; ++i) {
        bool on = (pattern >> i) & 0x01;
        gpio_put(SEG_PINS[i], on ? SEG_ON : SEG_OFF);
    }
    /* dp */
    gpio_put(SEG_PINS[7], dp_on ? SEG_ON : SEG_OFF);
}

static inline void dig_all_off(void) {
    for (int i = 0; i < 4; ++i) gpio_put(DIG_PINS[i], DIG_OFF);
}

static inline void dig_on(uint8_t idx) {
    for (int i = 0; i < 4; ++i) gpio_put(DIG_PINS[i], (i == idx) ? DIG_ON : DIG_OFF);
}

/* API mínima */
void display7seg_init(void) {
    for (int i = 0; i < 8; ++i) {
        gpio_init(SEG_PINS[i]);
        gpio_set_dir(SEG_PINS[i], GPIO_OUT);
        gpio_put(SEG_PINS[i], SEG_OFF);
    }
    for (int i = 0; i < 4; ++i) {
        gpio_init(DIG_PINS[i]);
        gpio_set_dir(DIG_PINS[i], GPIO_OUT);
        gpio_put(DIG_PINS[i], DIG_OFF);
    }
    for (int i = 0; i < 4; ++i) digits[i] = 10;
    dp_mask = 0x00;
    cur_digit = 0;
}

/* ms: 0..9999 -> s.mmm (dp en dígito 0) */
void display7seg_set_s_mmm(uint16_t ms) {
    if (ms > 9999) ms = 9999;
    uint8_t s    = (uint8_t)(ms / 1000);       /* 0..9 */
    uint16_t rem = (uint16_t)(ms % 1000);      /* 0..999 */
    digits[0]    = s;
    digits[1]    = (uint8_t)(rem / 100);
    digits[2]    = (uint8_t)((rem / 10) % 10);
    digits[3]    = (uint8_t)(rem % 10);
    dp_mask      = 0b0001;                     /* DP en el dígito 0 => s.mmm */
}

void display7seg_refresh_once(void) {
    dig_all_off();

    uint8_t d = digits[cur_digit];
    if (d > 10) d = 10;
    uint8_t pat = SEG_MAP_AC[d];
    bool dp_on  = ((dp_mask >> cur_digit) & 0x01) != 0;

    seg_write_raw(pat, dp_on);
    dig_on(cur_digit);

    /* mantener ~1 ms para brillo */
    sleep_us(HOLD_US);

    cur_digit = (cur_digit + 1u) & 0x03;  /* 0..3 */
}

void display7seg_show_ms_block(uint16_t ms, uint16_t hold_ms) {
    absolute_time_t t_end = make_timeout_time_ms(hold_ms);
    display7seg_set_s_mmm(ms);
    while (absolute_time_diff_us(get_absolute_time(), t_end) > 0) {
        display7seg_refresh_once();
    }
}
