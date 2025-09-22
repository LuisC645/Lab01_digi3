#include <stdint.h>
#include <pico/stdlib.h>


void display7seg_init(void);
void display7seg_set_raw(uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3); // 0..9, 10=blank
void display7seg_set_s_mmm(uint16_t ms); // 0..9999 -> s.mmm
void display7seg_refresh_once(void); // 1 dígito por llamada
void display7seg_show_ms_block(uint16_t ms, uint16_t hold_ms); // bloqueante