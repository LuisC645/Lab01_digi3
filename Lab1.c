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
#define MAX_ON_MS        5000   // 5s max
#define MAX_TOTAL_MS     9999    // max en disp
#define PENALTY_INC_MS   1000    // penalización
#define WAIT_MIN_MS      1000
#define WAIT_MAX_MS      5000
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

// ===== Random number =====
static uint32_t rng(uint32_t min_incl, uint32_t max_incl) {
    uint32_t now = time_us_32();                   // cambia todo el tiempo
    uint32_t span = (max_incl - min_incl + 1u);
    return min_incl + (now % span);
}

// Wait
static void wait_for_start(void) {
    absolute_time_t t0 = get_absolute_time();
    bool phase = false;

    for (;;) {
        if (btn_pressed(RST)) {
            leds_off_all();
            sleep_ms(10);
            return;
        }
        if (ms_since(t0) >= 300) {
            t0 = get_absolute_time();
            phase = !phase;
            gpio_put(LED1, phase);
            gpio_put(LED2, phase);
            gpio_put(LED3, phase);
        }
        sleep_us(SCAN_SLEEP_US);
    }
}

// Start sequence
static void start_sequence(void) {
    const uint32_t STEP_MS = 400;

    // 000: todos apagados
    gpio_put(LED1, 0); gpio_put(LED2, 0); gpio_put(LED3, 0);
    sleep_ms(STEP_MS);

    // 111: LED1, LED2, LED3 encendidos
    gpio_put(LED1, 1); gpio_put(LED2, 1); gpio_put(LED3, 1);
    sleep_ms(STEP_MS);

    // 011: LED2, LED3 encendidos
    gpio_put(LED1, 0); gpio_put(LED2, 1); gpio_put(LED3, 1);
    sleep_ms(STEP_MS);

    // 001: solo LED3 encendido
    gpio_put(LED1, 0); gpio_put(LED2, 0); gpio_put(LED3, 1);
    sleep_ms(STEP_MS);

    // 000: todos apagados
    gpio_put(LED1, 0); gpio_put(LED2, 0); gpio_put(LED3, 0);
    sleep_ms(STEP_MS);
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
        start_sequence();

        bool canceled_round = false;

        // ===== 3) Espera aleatoria (cancelable con RST) =====
        uint32_t wait_ms = rng(WAIT_MIN_MS, WAIT_MAX_MS);
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

        // ===== 4) Ronda: elegir objetivo, mostrar tiempo en vivo por serial, medir penalizaciones =====
        uint target = rng(0, 2);
        uint led_pin = LEDS[target];
        gpio_put(led_pin, 1);
        absolute_time_t t0 = get_absolute_time();

        int64_t penalty_ms = 0;
        int64_t total_ms   = 0;
        bool finished      = false;

        // --- tiempo en vivo por consola ---
        int64_t last_report_ms = -1000;
        const  int64_t report_step_ms = 100; // refresco cada 100 ms

        printf("LED%d encendido. Tiempo: 0 ms", (int)target+1);

        for (;;) {
            // Cancelación global
            if (btn_pressed(RST)) {
                gpio_put(led_pin, 0);
                canceled_round = true;
                break;
            }

            // Tiempo transcurrido
            int64_t elapsed = ms_since(t0);

            // Timeout
            if (elapsed >= MAX_ON_MS) {
                total_ms = MAX_ON_MS;
                finished = false;
                printf("\rLED%d encendido. Tiempo: %lld ms\n", (int)target+1, (long long)MAX_ON_MS);
                break;
            }

            // Mostrar tiempo en vivo (sin penalización) en la misma línea
            if (elapsed - last_report_ms >= report_step_ms) {
                last_report_ms = elapsed;
                printf("\rLED%d encendido. Tiempo: %lld ms", (int)target+1, (long long)elapsed);
                fflush(stdout);
            }

            // Lectura de botones (una por iteración)
            bool pressed = false;
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

                // (Opcional) notificar penalización sin romper la línea
                printf("\rLED%d encendido. Tiempo: %lld ms (+%lld ms penal.)",
                       (int)target+1, (long long)elapsed, (long long)PENALTY_INC_MS);

                if (elapsed + penalty_ms >= MAX_TOTAL_MS) {
                    total_ms = MAX_TOTAL_MS;
                    finished = false;
                }
            }

            if (pressed) {
                if (finished) {
                    if (total_ms > MAX_TOTAL_MS) total_ms = MAX_TOTAL_MS;
                    // cerrar la línea en vivo con salto de línea
                    printf("\rLED%d encendido. Tiempo: %lld ms\n", (int)target+1, (long long)(total_ms - penalty_ms));
                    break;
                } else if (total_ms == MAX_TOTAL_MS) {
                    printf("\rLED%d encendido. Tiempo: %lld ms\n", (int)target+1, (long long)(MAX_TOTAL_MS));
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

        // ===== 5) Mostrar resultado final y en display =====
        bool timed_out = (!finished && (total_ms >= MAX_ON_MS));

        if (finished) {
            uint16_t show = (total_ms < 0) ? 0 :
                            (total_ms > MAX_TOTAL_MS ? MAX_TOTAL_MS : (uint16_t)total_ms);
            printf("LED%d -> Reaction: %u.%03u s", (int)target+1, show/1000, show%1000);
            if (penalty_ms > 0) printf(" (penalty +%lld ms)", penalty_ms);
            printf("\n");
            display7seg_show_ms_block(show, 1000);
        } else if (timed_out) {
            // TIMEOUT: mostrar 0.000 en el display
            printf(
                "LED%d -> Timeout: %d.%03d s\n",
                (int)target+1, (int)(MAX_ON_MS/1000), (int)(MAX_ON_MS%1000)
            );
            display7seg_show_ms_block(0, 1000);
        } else {
            // Cancelado por RST (mantiene comportamiento previo)
            uint16_t show = (total_ms <= 0) ? 0 :
                            (total_ms > MAX_TOTAL_MS ? MAX_TOTAL_MS : (uint16_t)total_ms);
            printf("LED%d -> Cancelado\n", (int)target+1);
            display7seg_show_ms_block(show, 1000);
        }

        // ===== 6) Pausa entre rondas (cancelable) =====
        absolute_time_t t_pause = make_timeout_time_ms(ROUND_PAUSE_MS);
        while (absolute_time_diff_us(get_absolute_time(), t_pause) > 0) {
            if (btn_pressed(RST)) break;
            sleep_us(SCAN_SLEEP_US);
        }
        leds_off_all();
        sleep_ms(1000);
    }
}