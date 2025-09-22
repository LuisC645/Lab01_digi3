#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/time.h"

#include "lib/debounce/debounce.h"
#include "lib/display7seg/display7seg.h"

// ===== Pines =====
#define LED1 11
#define BTN1 20

#define LED2 12
#define BTN2 18

#define LED3 13
#define BTN3 19

#define RST  22

// ===== Parámetros =====
#define MAX_ON_MS        10000   // 10 s de ventana para acertar
#define MAX_TOTAL_MS     9999    // tope que mostramos
#define PENALTY_INC_MS   1000    // penalización por fallo
#define WAIT_MIN_MS      1000
#define WAIT_MAX_MS      5000
#define START_BLINK_MS   80
#define ROUND_PAUSE_MS   600
#define SCAN_SLEEP_US    250

// ===== Utilidad tiempo =====
static inline int64_t ms_since(absolute_time_t t0) {
    return absolute_time_diff_us(t0, get_absolute_time()) / 1000;
}

// ===== LEDs =====
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

// ===== RNG sencillo =====
static uint32_t rng_uniform(uint32_t min_incl, uint32_t max_incl) {
    uint32_t now = time_us_32();                   // cambia todo el tiempo
    uint32_t span = (max_incl - min_incl + 1u);
    return min_incl + (now % span);
}

// Espera a que se presione RST mostrando un "latido" con los 3 LEDs.
static void wait_for_start(void) {
    absolute_time_t t0 = get_absolute_time();
    bool phase = false;

    for (;;) {
        if (btn_pressed(RST)) { 
            leds_off_all(); 
            sleep_ms(10); 
            return; 
        }
        if (ms_since(t0) >= START_BLINK_MS) {
            t0 = get_absolute_time();
            phase = !phase;
            gpio_put(LED1, phase);
            gpio_put(LED2, phase);
            gpio_put(LED3, phase);
        }
        sleep_us(SCAN_SLEEP_US);
    }
}

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0);

    // Display
    display7seg_init();

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
        // ===== 1) Pantalla de inicio: esperar RST con “latido” =====
        wait_for_start();

        // ===== 2) Secuencia breve =====
        const uint seq[5] = {0,1,2,1,0};
        for (int i = 0; i < 5; ++i) {
            leds_off_all();
            gpio_put(LEDS[seq[i]], 1);
            sleep_ms(100);

        }
        leds_off_all();
        sleep_ms(100);

        bool canceled_round = false;

        // ===== 3) Espera aleatoria (cancelable con RST) =====
        uint32_t wait_ms = rng_uniform(WAIT_MIN_MS, WAIT_MAX_MS);
        absolute_time_t t_wait = make_timeout_time_ms(wait_ms);
        while (absolute_time_diff_us(get_absolute_time(), t_wait) > 0) {
            if (btn_pressed(RST)) { 
                leds_off_all(); 
                canceled_round = true; 
                break; 
            }
            sleep_us(SCAN_SLEEP_US);
        }
        if (canceled_round) {
            // Pausa entre rondas (cancelable)
            absolute_time_t t_pause = make_timeout_time_ms(ROUND_PAUSE_MS);
            while (absolute_time_diff_us(get_absolute_time(), t_pause) > 0) {
                if (btn_pressed(RST)) break;
                sleep_us(SCAN_SLEEP_US);
            }
            leds_off_all();
            sleep_ms(300);
            continue; // siguiente ronda
        }

        // ===== 4) Ronda: elegir objetivo, medir tiempo, penalizaciones =====
        uint target = rng_uniform(0, 2);     // 0..2
        uint led_pin = LEDS[target];
        gpio_put(led_pin, 1);
        absolute_time_t t0 = get_absolute_time();

        int64_t penalty_ms = 0;
        int64_t total_ms   = 0;
        bool finished      = false;

        for (;;) {
            // Cancelación global
            if (btn_pressed(RST)) { 
                gpio_put(led_pin, 0); 
                canceled_round = true; 
                break; 
            }

            // Timeout
            int64_t elapsed = ms_since(t0);
            if (elapsed >= MAX_ON_MS) {
                total_ms = MAX_ON_MS;
                finished = false;
                break;
            }

            // Lectura de botones (una por iteración)
            bool pressed = false;
            // Correctos/incorrectos sin apuntadores
            if (target == 0 && btn_pressed(BTN1)) {
                pressed = true; total_ms = elapsed + penalty_ms; finished = true;
            } else if (target == 1 && btn_pressed(BTN2)) {
                pressed = true; total_ms = elapsed + penalty_ms; finished = true;
            } else if (target == 2 && btn_pressed(BTN3)) {
                pressed = true; total_ms = elapsed + penalty_ms; finished = true;
            } else if (btn_pressed(BTN1) || btn_pressed(BTN2) || btn_pressed(BTN3)) {
                // Se presionó alguno pero no el correcto
                pressed = true;
                penalty_ms += PENALTY_INC_MS;
                if (elapsed + penalty_ms >= MAX_TOTAL_MS) {
                    total_ms = MAX_TOTAL_MS;
                    finished = false;
                }
            }

            if (pressed) {
                if (finished) {
                    if (total_ms > MAX_TOTAL_MS) total_ms = MAX_TOTAL_MS;
                    break;
                } else if (total_ms == MAX_TOTAL_MS) {
                    break;
                }
            }

            sleep_us(SCAN_SLEEP_US);
        }

        // Apagar LED del objetivo
        gpio_put(led_pin, 0);

        // Si se canceló con RST, no mostrar resultado; pasar a pausa entre rondas
        if (canceled_round) {
            absolute_time_t t_pause = make_timeout_time_ms(ROUND_PAUSE_MS);
            while (absolute_time_diff_us(get_absolute_time(), t_pause) > 0) {
                if (btn_pressed(RST)) break;
                sleep_us(SCAN_SLEEP_US);
            }
            leds_off_all();
            sleep_ms(300);
            continue; // siguiente ronda
        }

        // ===== 5) Mostrar resultado =====
        if (finished) {
            uint16_t show = (total_ms < 0) ? 0 :
                            (total_ms > MAX_TOTAL_MS ? MAX_TOTAL_MS : (uint16_t)total_ms);
            printf("LED%d -> Reaction: %u.%03u s", (int)target+1, show/1000, show%1000);
            if (penalty_ms > 0) printf(" (penalty +%lld ms)", penalty_ms);
            printf("\n");
            display7seg_show_ms_block(show, 1000);
        } else {
            uint16_t show = (total_ms <= 0) ? 0 :
                            (total_ms > MAX_TOTAL_MS ? MAX_TOTAL_MS : (uint16_t)total_ms);
            if (show == MAX_TOTAL_MS) {
                printf("LED%d -> Timeout: %u.%03u s\n", (int)target+1, show/1000, show%1000);
            } else {
                printf("LED%d -> Cancelado\n", (int)target+1);
            }
            display7seg_show_ms_block(show, 1000);
        }

        // ===== 6) Pausa entre rondas (cancelable) =====
        absolute_time_t t_pause = make_timeout_time_ms(ROUND_PAUSE_MS);
        while (absolute_time_diff_us(get_absolute_time(), t_pause) > 0) {
            if (btn_pressed(RST)) break;
            sleep_us(SCAN_SLEEP_US);
        }
        leds_off_all();
        sleep_ms(300);

    }
}
