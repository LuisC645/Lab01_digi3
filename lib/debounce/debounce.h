/**
 * @file debounce.h
 * @brief Utilidades mínimas de botón con antirrebote para RP2040 (activo-bajo).
 *
 * Implementación *header-only*:
 * - `btn_init(pin)`: configura GPIO como entrada con pull-up interno.
 * - `btn_pressed(pin)`: detecta pulsación con retardo de antirrebote y
 *   **bloquea** hasta que el usuario suelte el botón (edge + hold-off).
 *
 * @note Convención **activo-bajo**: presionado = nivel 0.
 * @note `btn_pressed()` es **bloqueante** mientras el botón permanezca presionado.
 * @author Luis Castillo Chicaiza
 */

#pragma once
#include <pico/stdlib.h>
#include <stdbool.h>

/**
 * @brief Inicializa un botón activo-bajo con pull-up interno.
 *
 * Configura el pin como entrada y habilita la resistencia de pull-up,
 * de modo que el reposo sea nivel alto y la pulsación lleve el pin a 0.
 *
 * @param pin GPIO del botón.
 */
static inline void btn_init(uint pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

/**
 * @brief Detecta pulsación con antirrebote y espera hasta soltar.
 *
 * Secuencia:
 * 1. Lee el pin (activo-bajo). Si está a 0, asume posible pulsación.
 * 2. Aplica un retardo de antirrebote (20 ms) y vuelve a leer.
 * 3. Si sigue a 0, considera pulsación válida y **bloquea** hasta que el
 *    usuario suelte (vuelva a 1). Devuelve `true` una sola vez por pulsación.
 *
 * @param pin GPIO del botón (activo-bajo con pull-up).
 * @return `true` si se detectó y confirmó una pulsación (y ya se soltó);
 *         `false` si no hubo pulsación.
 *
 * @warning Esta función es **bloqueante** durante la pulsación (espera de suelta).
 *          Úsala donde este comportamiento sea aceptable.
 */
static inline bool btn_pressed(uint pin) {
    if (!gpio_get(pin)) {            // activo-bajo -> presionado (nivel 0)
        sleep_ms(20);                // retardo de antirrebote (debounce)
        if (!gpio_get(pin)) {        // confirma que sigue presionado
            while (!gpio_get(pin)) { // espera activa hasta que se suelte
                tight_loop_contents();
            }
            return true;             // pulsación única confirmada
        }
    }
    return false;                    // no hay pulsación válida
}
