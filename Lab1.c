#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"

// LED – Botón pares
#define LED1 16   // GP16
#define BTN1 14   // GP14

#define LED2 17   // GP17
#define BTN2 13   // GP13

#define LED3 18   // GP18
#define BTN3 15   // GP15

#define MAX_ON_MS 9999         // 10 s
#define MAX_TOTAL_MS  9999     // tope absoluto: elapsed + penalty

static inline bool btn_pressed(int gpio) { return !gpio_get(gpio); }

int main(void) {
    stdio_init_all();
    setvbuf(stdout, NULL, _IONBF, 0); // sin buffering para evitar lag


    const int LEDS[3] = { LED1, LED2, LED3 };
    const int BTNS[3] = { BTN1, BTN2, BTN3 };

    // Init
    for (int i = 0; i < 3; ++i) {
        gpio_init(LEDS[i]); gpio_set_dir(LEDS[i], GPIO_OUT); gpio_put(LEDS[i], 0);
        gpio_init(BTNS[i]); gpio_set_dir(BTNS[i], GPIO_IN);  gpio_pull_up(BTNS[i]);
    }

    while (true) {
        // Secuencia de inicio
        gpio_put(LED1,1); gpio_put(LED2,0); gpio_put(LED3,0); sleep_ms(100);
        gpio_put(LED1,0); gpio_put(LED2,1); gpio_put(LED3,0); sleep_ms(100);
        gpio_put(LED1,0); gpio_put(LED2,0); gpio_put(LED3,1); sleep_ms(100);
        gpio_put(LED1,0); gpio_put(LED2,1); gpio_put(LED3,0); sleep_ms(100);
        gpio_put(LED1,1); gpio_put(LED2,0); gpio_put(LED3,0); sleep_ms(100);
        gpio_put(LED1,0); gpio_put(LED2,0); gpio_put(LED3,0); sleep_ms(100);

        // Espera aleatoria 1–5 s
        uint32_t wait_ms = 1000 + (time_us_32() % 4000);
        sleep_ms(wait_ms);

        // Selecciona LED objetivo
        int target = time_us_32() % 3;
        int led_pin = LEDS[target];
        int btn_pin = BTNS[target];

        // Enciende objetivo y arranca cronómetro
        gpio_put(led_pin, 1);
        absolute_time_t t0 = get_absolute_time();

        bool finished = false;
        int64_t penalty_ms = 0;
        int64_t total_ms = 0;

        // Esperar hasta que se presione el botón CORRECTO
        while (true) {

            // ¿Se agotaron los 10 s?
            int64_t elapsed_ms = absolute_time_diff_us(t0, get_absolute_time()) / 1000;

            if (elapsed_ms >= MAX_ON_MS) {
                // Tiempo se fija en 10 s
                total_ms = MAX_ON_MS;
                break; // timeout
            }

            // Escanea botones
            for (int i = 0; i < 3; ++i) {
                if (btn_pressed(BTNS[i])) {
                    // Consumir una pulsación (espera a soltar)
                    while (btn_pressed(BTNS[i])) tight_loop_contents();

                    if (i == target) {
                        // Correcto antes del timeout
                        int64_t hit_ms = absolute_time_diff_us(t0, get_absolute_time()) / 1000;
                        total_ms = hit_ms + penalty_ms;

                        if (total_ms > MAX_TOTAL_MS) total_ms = MAX_TOTAL_MS;
                        finished = true;
                        break;
                    } else {
                        // Incorrecto: +1 s de penalización
                        penalty_ms += 1000;
                        if ( (absolute_time_diff_us(t0, get_absolute_time())/1000) + penalty_ms >= MAX_TOTAL_MS ) {
                            total_ms = MAX_TOTAL_MS;
                            finished = false;
                            goto end_round;   // fin inmediato por tope
                        }
                    }
                }
            }
            if (finished) break;
            tight_loop_contents();
        }

        end_round:
            // Apaga LED y muestra resultado
            gpio_put(led_pin, 0);
            if (finished) {
                printf("LED%d -> Reaction: %lld.%03lld s",
                    target+1, total_ms/1000, total_ms%1000);
            } else {
                // Timeout (no hubo acierto en 10 s)
                printf("LED%d -> Timeout: %lld.%03lld s",
                    target+1, total_ms/1000, total_ms%1000);
            }
            if (penalty_ms > 0) {
                printf(" (penalty +%lld ms)\n", penalty_ms);
            } else {
                printf("\n");
            }
                    
            // Pausa antes de la siguiente ronda
            sleep_ms(600);
    }
}
