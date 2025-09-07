#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"

// LED1 y su botón
#define LED1 16   // GP16 (pin 21)
#define BTN1 14   // GP14 (pin 19)

// LED2 y su botón
#define LED2 17   // GP17 (pin 22)
#define BTN2 13   // GP12 (pin 17)

// Botones con pull-up: presionado = 0
static inline bool btn1_pressed(void){ return !gpio_get(BTN1); }
static inline bool btn2_pressed(void){ return !gpio_get(BTN2); }

int main(void) {
    stdio_init_all();

    // LEDs
    gpio_init(LED1); gpio_set_dir(LED1, GPIO_OUT); gpio_put(LED1, 0);
    gpio_init(LED2); gpio_set_dir(LED2, GPIO_OUT); gpio_put(LED2, 0);

    // Botones (pull-up interno)
    gpio_init(BTN1); gpio_set_dir(BTN1, GPIO_IN); gpio_pull_up(BTN1);
    gpio_init(BTN2); gpio_set_dir(BTN2, GPIO_IN); gpio_pull_up(BTN2);

    while (true) {
        // Pequeña “señal” de inicio
        gpio_put(LED1, 1); gpio_put(LED2, 1); sleep_ms(150);
        gpio_put(LED1, 0); gpio_put(LED2, 0); sleep_ms(350);

        // Espera aleatoria 1–5 s
        uint32_t wait_ms = 1000 + (time_us_32() % 4000);
        sleep_ms(wait_ms);

        // Elegir LED aleatorio (0 -> LED1, 1 -> LED2)
        int choice = time_us_32() & 1;
        int led_pin   = (choice == 0) ? LED1 : LED2;
        int expectBtn = (choice == 0) ? BTN1 : BTN2;

        // Enciende el LED seleccionado
        gpio_put(led_pin, 1);
        absolute_time_t t0 = get_absolute_time();

        // Esperar hasta que se presione SOLO el botón asociado
        while (true) {
            // Si se presiona el botón correcto: medir y salir
            if (expectBtn == BTN1 && btn1_pressed()) break;
            if (expectBtn == BTN2 && btn2_pressed()) break;

            // Si se presiona el botón incorrecto: se ignora (no cuenta)
            // (no hacemos nada, seguimos esperando)
            tight_loop_contents();
        }

        absolute_time_t t1 = get_absolute_time();
        int64_t elapsed_ms = absolute_time_diff_us(t0, t1) / 1000;

        // Apaga LED y muestra resultado
        gpio_put(led_pin, 0);
        printf("LED%d -> Reaction: %lld.%03lld s\n",
               (choice == 0) ? 1 : 2,
               elapsed_ms/1000, elapsed_ms%1000);

        // Esperar a que se suelte el botón antes de la siguiente ronda (simple)
        // (para no capturar una pulsación “arrastrada”)
        if (expectBtn == BTN1) { while (btn1_pressed()) tight_loop_contents(); }
        else                   { while (btn2_pressed()) tight_loop_contents(); }

        sleep_ms(600);
    }
}
