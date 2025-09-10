#include "pico/stdlib.h"
#include "pico/time.h"
#include "display7seg.h"

// =================== AJUSTA TUS PINES AQUÍ ===================
// Segmentos en orden: a,b,c,d,e,f,g,dp
static const uint SEG[8] = {2,3,4,5,6,7,8,9};

// Dígitos en orden: DIG1,DIG2,DIG3,DIG4
static const uint DIG[4] = {0,1,27,26};
// =============================================================

// Lógica para CÁTODO COMÚN (BSR)
#define SEG_ON   1
#define SEG_OFF  0
#define DIG_ON   1
#define DIG_OFF  0

// Mapa 0–9 (a b c d e f g) para CC: 1=encendido
static const uint8_t NUM[10][7] = {
  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}  // 9
};

static inline void seg_all_off(void){
  for (int s = 0; s < 8; s++) gpio_put(SEG[s], SEG_OFF);
}
static inline void dig_all_off(void){
  for (int d = 0; d < 4; d++) gpio_put(DIG[d], DIG_OFF);
}

void disp_init(void){
  for (int s = 0; s < 8; s++){
    gpio_init(SEG[s]);
    gpio_set_dir(SEG[s], GPIO_OUT);
    gpio_put(SEG[s], SEG_OFF);
  }
  for (int d = 0; d < 4; d++){
    gpio_init(DIG[d]);
    gpio_set_dir(DIG[d], GPIO_OUT);
    gpio_put(DIG[d], DIG_OFF);
  }
}

// Pinta un dígito val(0..9) en idx(0..3); dot=true enciende el punto
static inline void disp_show_digit_once(int val, int idx, bool dot){
  dig_all_off();
  for (int s = 0; s < 7; s++) {
    gpio_put(SEG[s], NUM[val][s] ? SEG_ON : SEG_OFF);
  }
  gpio_put(SEG[7], dot ? SEG_ON : SEG_OFF);   // dp
  gpio_put(DIG[idx], DIG_ON);
}

void disp_show_ms_block(int ms_total, int hold_ms){
  if (ms_total < 0)     ms_total = 0;
  if (ms_total > 9999)  ms_total = 9999;

  int s    = ms_total / 1000;      // 0..9
  int mmm  = ms_total % 1000;      // 0..999
  int d2   = (mmm/100) % 10;
  int d3   = (mmm/10)  % 10;
  int d4   =  mmm      % 10;

  absolute_time_t until = make_timeout_time_ms(hold_ms);
  while (absolute_time_diff_us(get_absolute_time(), until) > 0) {
    disp_show_digit_once(s,  0, true);  sleep_ms(2); // s.***
    disp_show_digit_once(d2, 1, false); sleep_ms(2);
    disp_show_digit_once(d3, 2, false); sleep_ms(2);
    disp_show_digit_once(d4, 3, false); sleep_ms(2);
  }
  dig_all_off(); seg_all_off();
}

void disp_show_raw_block(const int digits[4], const bool dots[4], int hold_ms){
  absolute_time_t until = make_timeout_time_ms(hold_ms);
  while (absolute_time_diff_us(get_absolute_time(), until) > 0) {
    for (int i = 0; i < 4; i++){
      int v = digits[i]; if (v < 0) v = 0; if (v > 9) v = 9;
      disp_show_digit_once(v, i, dots[i]);
      sleep_ms(2);
    }
  }
  dig_all_off(); seg_all_off();
}
