#include <stdio.h>    
#include "pico/stdlib.h"
#include "pico/time.h"

#define LED_PIN 16   // GP16 (pin físico 21)
#define BTN_PIN 14   // GP14 (pin físico 19)

static inline int btn_pressed(void) {
    // Botón con pull-up: presionado = 0
    return !gpio_get(BTN_PIN);
}

int main(void) {
    stdio_init_all();        // Para imprimir por USB
    gpio_init(LED_PIN);      // LED como salida
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    gpio_init(BTN_PIN);      // Botón como entrada con pull-up
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    // “Semilla” simple para aleatorio (no perfecta, pero suficiente)
    uint32_t seed = time_us_32();
    for (int i = 0; i < (seed & 0xFF); ++i) (void)time_us_32();

    while (true) {
        // 1) “Cuenta regresiva” muy simple en el LED
        gpio_put(LED_PIN, 1); sleep_ms(200);
        gpio_put(LED_PIN, 0); sleep_ms(200);
        gpio_put(LED_PIN, 1); sleep_ms(200);
        gpio_put(LED_PIN, 0); sleep_ms(400);

        // 2) Espera aleatoria (1–10 s) antes de encender el LED
        uint32_t wait_ms = 1000 + (time_us_32() % 9000);
        sleep_ms(wait_ms);

        // 3) Enciende LED y empieza cronómetro
        gpio_put(LED_PIN, 1);
        absolute_time_t t0 = get_absolute_time();

        // 4) Espera a que se presione el botón (sin antirrebote)
        while (!btn_pressed()) {
            tight_loop_contents();
        }

        absolute_time_t t1 = get_absolute_time();
        int64_t elapsed_ms = absolute_time_diff_us(t0, t1) / 1000;

        // 5) Apaga LED y muestra resultado
        gpio_put(LED_PIN, 0);
        printf("Reaction: %lld.%03lld s\n", elapsed_ms/1000, elapsed_ms%1000);

        // 6) Pausa breve y repetir
        sleep_ms(800);
    }
}
