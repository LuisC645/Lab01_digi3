#include "display7seg.h"
#include <pico/time.h>
#include <hardware/gpio.h>

// Pines fijos: a..g y D0..D3
static const uint SEG_PINS[7] = {0, 1, 2, 3, 4, 5, 6};
static const uint DIG_PINS[4] = {28, 27, 26, 21};
static const uint DP_PIN = 7; 

// Polaridades: ánodo común + PNP
#define SEG_ON   0
#define SEG_OFF  1
#define DIG_ON   0
#define DIG_OFF  1

// Tabla lógica (bit0=a .. bit6=g): 1 = segmento encendido
static const uint8_t SEG_MAP[11] = {
    0x3F, /*0*/ 0x06, /*1*/ 0x5B, /*2*/ 0x4F, /*3*/
    0x66, /*4*/ 0x6D, /*5*/ 0x7D, /*6*/ 0x07, /*7*/
    0x7F, /*8*/ 0x67, /*9*/ 0x00  /*BLANK*/
};

static uint8_t digits[4] = {10,10,10,10}; // 10 = blank
static uint8_t cur_digit = 0;             // 0..3

#define DISPLAY7SEG_HOLD_US 800

static inline void dig_all_off(void){
    for (int i=0;i<4;i++) gpio_put(DIG_PINS[i], DIG_OFF);
}
static inline void dig_on(uint8_t idx){
    for (int i=0;i<4;i++) gpio_put(DIG_PINS[i], (i==idx)?DIG_ON:DIG_OFF);
}
static inline void seg_write(uint8_t pattern){
    for (int i=0;i<7;i++){
        bool on = (pattern >> i) & 1;
        gpio_put(SEG_PINS[i], on ? SEG_ON : SEG_OFF);
    }
}

void display7seg_init(void){
    for (int i=0;i<7;i++){ gpio_init(SEG_PINS[i]); gpio_set_dir(SEG_PINS[i], GPIO_OUT); gpio_put(SEG_PINS[i], SEG_OFF); }
    
    // DP como out (siempre encendido)
    gpio_init(DP_PIN);
    gpio_set_dir(DP_PIN, GPIO_OUT);
    gpio_put(DP_PIN, SEG_OFF);

    for (int i=0;i<4;i++){ gpio_init(DIG_PINS[i]); gpio_set_dir(DIG_PINS[i], GPIO_OUT); gpio_put(DIG_PINS[i], DIG_OFF); }
    for (int i=0;i<4;i++) digits[i]=10;
    cur_digit=0;
}

void display7seg_set_raw(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3){
    digits[0] = (d0<=10)?d0:10;
    digits[1] = (d1<=10)?d1:10;
    digits[2] = (d2<=10)?d2:10;
    digits[3] = (d3<=10)?d3:10;
}

void display7seg_set_s_mmm(uint16_t ms){
    if (ms>9999) ms=9999;
    uint8_t s  = (uint8_t)(ms/1000);
    uint16_t r = (uint16_t)(ms%1000);
    digits[0]= s;
    digits[1]= (uint8_t)(r/100);
    digits[2]= (uint8_t)((r/10)%10);
    digits[3]= (uint8_t)(r%10);
}

void display7seg_refresh_once(void){
    dig_all_off();

    uint8_t d = (digits[cur_digit]<=10)?digits[cur_digit]:10;
    seg_write(SEG_MAP[d]);

    // DP solo en el dígito 0 (s.mmm)
    gpio_put(DP_PIN, (cur_digit == 0) ? SEG_ON : SEG_OFF);

    dig_on(cur_digit);
    sleep_us(DISPLAY7SEG_HOLD_US);
    cur_digit = (cur_digit+1u)&0x03;
}

void display7seg_show_ms_block(uint16_t ms, uint16_t hold_ms){
    absolute_time_t t_end = make_timeout_time_ms(hold_ms);
    display7seg_set_s_mmm(ms);
    while (absolute_time_diff_us(t_end, get_absolute_time()) > 0){
        display7seg_refresh_once();
    }
}
