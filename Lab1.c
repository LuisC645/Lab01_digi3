#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"

// LED – Botón pares
#define LED1 16   // GP16
#define BTN1 14   // GP14

#define LED2 17   // GP17
#define BTN2 13   // GP12

#define LED3 18   // GP18
#define BTN3 15   // GP15

// Helpers: botones con pull-up, presionado = 0
static inline bool btn_pressed(int gpio) {
    return !gpio_get(gpio);
}

int main(void) {
    stdio_init_all();

    // Inicializa LEDs
    gpio_init(LED1); gpio_set_dir(LED1, GPIO_OUT); gpio_put(LED1, 0);
    gpio_init(LED2); gpio_set_dir(LED2, GPIO_OUT); gpio_put(LED2, 0);
    gpio_init(LED3); gpio_set_dir(LED3, GPIO_OUT); gpio_put(LED3, 0);

    // Inicializa botones
    gpio_init(BTN1); gpio_set_dir(BTN1, GPIO_IN); gpio_pull_up(BTN1);
    gpio_init(BTN2); gpio_set_dir(BTN2, GPIO_IN); gpio_pull_up(BTN2);
    gpio_init(BTN3); gpio_set_dir(BTN3, GPIO_IN); gpio_pull_up(BTN3);

    while (true) {
        // Breve señal de inicio
        gpio_put(LED1, 1); gpio_put(LED2, 1); gpio_put(LED3, 1); sleep_ms(100);
        gpio_put(LED1, 0); gpio_put(LED2, 0); gpio_put(LED3, 0); sleep_ms(200);
        gpio_put(LED1, 1); gpio_put(LED2, 1); gpio_put(LED3, 1); sleep_ms(100);
        gpio_put(LED1, 0); gpio_put(LED2, 0); gpio_put(LED3, 0); sleep_ms(200);
        gpio_put(LED1, 1); gpio_put(LED2, 1); gpio_put(LED3, 1); sleep_ms(100);
        gpio_put(LED1, 0); gpio_put(LED2, 0); gpio_put(LED3, 0); sleep_ms(200);

        // Espera aleatoria 1–5 s
        uint32_t wait_ms = 1000 + (time_us_32() % 4000);
        sleep_ms(wait_ms);

        // Selecciona LED aleatorio
        int choice = time_us_32() % 3;  
        int led_pin, btn_pin;
        if (choice == 0) { led_pin = LED1; btn_pin = BTN1; }
        else if (choice == 1) { led_pin = LED2; btn_pin = BTN2; }
        else { led_pin = LED3; btn_pin = BTN3; }

        // Enciende LED elegido y arranca cronómetro
        gpio_put(led_pin, 1);
        absolute_time_t t0 = get_absolute_time();

        // Espera hasta que el botón asociado se presione
        while (!btn_pressed(btn_pin)) {
            tight_loop_contents();
        }

        absolute_time_t t1 = get_absolute_time();
        int64_t elapsed_ms = absolute_time_diff_us(t0, t1) / 1000;

        // Apaga LED y muestra resultado
        gpio_put(led_pin, 0);
        printf("LED%d -> Reaction: %lld.%03lld s\n",
               choice+1, elapsed_ms/1000, elapsed_ms%1000);

        // Esperar a que se suelte antes de otra ronda
        while (btn_pressed(btn_pin)) {
            tight_loop_contents();
        }

        sleep_ms(600);
    }
}
