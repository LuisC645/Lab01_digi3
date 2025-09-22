// include/debounce.h
#pragma once
#include <pico/stdlib.h>
#include <stdbool.h>

// Inicializa un botón (activo-bajo con pull-up)
static inline void btn_init(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

// Devuelve true si el botón fue presionado
static inline bool btn_pressed(uint pin) {
    if (!gpio_get(pin)) {            // activo-bajo -> presionado
        sleep_ms(20);                // retardo de antirrebote
        if (!gpio_get(pin)) {
            while (!gpio_get(pin)) { // espera a que se suelte
                tight_loop_contents();
            }
            return true;
        }
    }
    return false;
}
