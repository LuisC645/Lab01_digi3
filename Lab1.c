#include "pico/stdlib.h"

#define LED_PIN 16
#define BTN_PIN 14

int main() {
    // Inicializa stdio si usas printf (opcional aquí)
    stdio_init_all();

    // Configuración LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Configuración botón con pull-up interno
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);  // pull-up => el pin está en 1 si no presionas

    while (true) {
        // Lee el botón (0 cuando se presiona, 1 cuando está suelto)
        bool pressed = !gpio_get(BTN_PIN);

        // Enciende LED si se presiona
        gpio_put(LED_PIN, pressed);

        sleep_ms(20); // pequeño delay para estabilidad
    }
}
