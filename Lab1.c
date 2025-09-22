#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"

#include "lib/debounce/debounce.h"
#include "lib/display7seg/display7seg.h"

// Pines
#define LED1 11
#define BTN1 20

#define LED2 12
#define BTN2 18

#define LED3 13
#define BTN3 19

#define RST  22

// Globales
#define MAX_ON_MS        5000   // 5s max
#define MAX_TOTAL_MS     9999   // max en disp
#define PENALTY_INC_MS   1000   // penalización
#define WAIT_MIN_MS      1000
#define WAIT_MAX_MS      5000
#define ROUND_PAUSE_MS   1000

// Utilidad time (contador el tiempo)
static inline int64_t ms_since(absolute_time_t t0) {
    return absolute_time_diff_us(t0, get_absolute_time()) / 1000;
}

// Random
static uint32_t rng(uint32_t min_incl, uint32_t max_incl) {
    uint32_t now  = time_us_32();                  // cambia todo el tiempo
    uint32_t span = (max_incl - min_incl + 1u);    // tamaño del rango
    return min_incl + (now % span);
}

// Esperar hasta
static bool wait_until_or_rst(uint32_t ms) {
    absolute_time_t t_end = make_timeout_time_ms(ms); //t_end + now + ms
    while (absolute_time_diff_us(t_end, get_absolute_time()) > 0) {
        if (btn_pressed(RST)) return true;
        display7seg_refresh_once();
    }
    return false;
}

// Espera aleatoria
static bool wait_rand_or_rst(uint32_t min_ms, uint32_t max_ms) {
    uint32_t wait_ms = min_ms + (time_us_32() % (max_ms - min_ms + 1u));
    uint32_t t0 = to_ms_since_boot(get_absolute_time());
    for (;;) {
        if (btn_pressed(RST)) return true;
        display7seg_refresh_once();
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((uint32_t)(now - t0) >= wait_ms) return false;
    }
}

// Leds
static void led_init(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}
static void leds_off_all(void) {
    gpio_put(LED1, 0);
    gpio_put(LED2, 0);
    gpio_put(LED3, 0);
}

// Espera al inicio
static void wait_for_start(void) {
    absolute_time_t t0 = get_absolute_time();
    bool phase = false;
    for (;;) {
        if (btn_pressed(RST)) { leds_off_all(); sleep_ms(10); return; }
        if (ms_since(t0) >= 300) {
            t0 = get_absolute_time();
            phase = !phase;
            gpio_put(LED1, phase); gpio_put(LED2, phase); gpio_put(LED3, phase);
        }
        display7seg_refresh_once();
    }
}

// Espera con refresco del display
static void hold_with_refresh(uint32_t ms) {
    absolute_time_t t_end = make_timeout_time_ms(ms);
    while (absolute_time_diff_us(t_end, get_absolute_time()) > 0) {
        display7seg_refresh_once();
    }
}

static void start_sequence(void) {

    display7seg_set_s_mmm(0);
    display7seg_refresh_once();

    gpio_put(LED1, 0); gpio_put(LED2, 0); gpio_put(LED3, 0);
    sleep_ms(1000);

    gpio_put(LED1, 1); gpio_put(LED2, 1); gpio_put(LED3, 1);
    sleep_ms(1000);
        
    gpio_put(LED1, 0); gpio_put(LED2, 1); gpio_put(LED3, 1);
    sleep_ms(1000);

    gpio_put(LED1, 0); gpio_put(LED2, 0); gpio_put(LED3, 1);
    sleep_ms(1000);

    gpio_put(LED1, 0); gpio_put(LED2, 0); gpio_put(LED3, 0);
    sleep_ms(1000);

    display7seg_set_s_mmm(0);
    display7seg_refresh_once();

}

int main(void) {
    stdio_init_all();

    // Display
    display7seg_init();
    display7seg_set_s_mmm(0);

    // LEDs
    led_init(LED1);
    led_init(LED2);
    led_init(LED3);

    // Botones (activo-bajo)
    btn_init(BTN1);
    btn_init(BTN2);
    btn_init(BTN3);
    btn_init(RST);

    const uint LEDS[3] = { LED1, LED2, LED3 };

    for (;;) {
        // 1) Esperar RST
        wait_for_start();

        // 2) Secuencia
        start_sequence();

        // 3) Espera aleatoria (cancelable) — al terminar, se prende el LED objetivo
        if (wait_rand_or_rst(0, 10000)) {
            wait_until_or_rst(ROUND_PAUSE_MS);
            leds_off_all();
            hold_with_refresh(300);
            continue;
        }

        // 4) Ronda: prender LED objetivo y medir
        uint target = rng(0, 2);
        uint led_pin = LEDS[target];
        gpio_put(led_pin, 1);

        absolute_time_t t0 = get_absolute_time();
        int64_t penalty_ms = 0;
        int64_t total_ms   = 0;
        bool finished      = false;

        int64_t last_report_ms = -1000;
        const int64_t report_step_ms = 100;

        for (;;) {
            // Cancelación global
            if (btn_pressed(RST)) { 
                gpio_put(led_pin, 0); 
                finished=false; 
                total_ms=0; 
                penalty_ms=0; 
                break;
            }

            int64_t elapsed = ms_since(t0);

            // Timeout
            if (elapsed >= MAX_ON_MS) {
                total_ms = MAX_ON_MS;
                finished = false;
                // printf("\rLED%d encendido. Tiempo: %lld ms\n", (int)target+1, (long long)MAX_ON_MS);
                display7seg_set_s_mmm(MAX_ON_MS > 9999 ? 9999 : (uint16_t)MAX_ON_MS);
                break;
            }

            // Tiempo en vivo (cada 100 ms)
            if (elapsed - last_report_ms >= report_step_ms) {
                last_report_ms = elapsed;
                // printf("\rLED%d encendido. Tiempo: %lld ms", (int)target+1, (long long)elapsed);
                display7seg_set_s_mmm((uint16_t)(elapsed > 9999 ? 9999 : (elapsed < 0 ? 0 : elapsed)));
                fflush(stdout);
            }

            // Botones
            bool pressed = false;

            if (target == 0 && btn_pressed(BTN1)) { pressed = true; total_ms = elapsed + penalty_ms; finished = true; }
            else if (target == 1 && btn_pressed(BTN2)) { pressed = true; total_ms = elapsed + penalty_ms; finished = true; }
            else if (target == 2 && btn_pressed(BTN3)) { pressed = true; total_ms = elapsed + penalty_ms; finished = true; }
            else if (btn_pressed(BTN1) || btn_pressed(BTN2) || btn_pressed(BTN3)) {
                pressed = true;
                penalty_ms += PENALTY_INC_MS;
                printf("\rLED%d encendido. Tiempo: %lld ms (+%lld ms penal.)",
                       (int)target+1, (long long)elapsed, (long long)PENALTY_INC_MS);
                if (elapsed + penalty_ms >= MAX_TOTAL_MS) { total_ms = MAX_TOTAL_MS; finished = false; }
            }

            if (pressed) {
                if (finished) {
                    if (total_ms > MAX_TOTAL_MS) total_ms = MAX_TOTAL_MS;
                    display7seg_set_s_mmm((uint16_t)((total_ms - penalty_ms) < 0 ? 0 : (total_ms - penalty_ms) > 9999 ? 9999 : (total_ms - penalty_ms)));
                    break;
                } else if (total_ms == MAX_TOTAL_MS) {
                    display7seg_set_s_mmm((uint16_t)MAX_TOTAL_MS);
                    break;
                }
            }
            display7seg_refresh_once();
        }

        // 5) Resultado final
        gpio_put(led_pin, 0);
        bool timed_out = (total_ms >= MAX_ON_MS && !finished);

        if (finished) {
            uint16_t show = (uint16_t) total_ms;
            printf("LED%d -> Reaction: %u.%03u s", (int)target+1, show/1000, show%1000);
            if (penalty_ms > 0) printf(" (penalty +%lld ms)", penalty_ms);
            printf("\n");
            display7seg_show_ms_block(show, 1000);

        } else if (timed_out) {
            printf("LED%d -> Timeout: %d.%03d s\n", (int)target+1, (int)(MAX_ON_MS/1000), (int)(MAX_ON_MS%1000));
            display7seg_show_ms_block(9999, 1000);

        } else {
            uint16_t show = (uint16_t) total_ms;
            printf("LED%d -> Cancelado\n");
            display7seg_show_ms_block(show, 5000);
        }

        // 6) Pausa entre rondas (cancelable)
        (void)wait_until_or_rst(ROUND_PAUSE_MS);
        leds_off_all();
        hold_with_refresh(1000);
    }
}
