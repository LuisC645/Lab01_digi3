#include <stdio.h>
#include "pico/stdlib.h"

// ====== CONFIGURA AQUÍ TUS PINES ======
static const uint LEDS[3]     = {2, 3, 4};     // 3 LEDs
static const uint BTN_LED[3]  = {5, 6, 7};     // 3 botones (uno por LED)
static const uint BTN_START   = 8;             // botón de inicio

// Si tus botones van a GND -> pull-up y lectura invertida (!gpio_get(pin))
static inline bool btn_pressed(uint pin) {
    // Antirrebote súper simple
    static absolute_time_t last[32];
    bool pressed = !gpio_get(pin);
    if (pressed) {
        absolute_time_t now = get_absolute_time();
        if (to_ms_since_boot(now) - to_ms_since_boot(last[pin]) > 50) {
            sleep_ms(5);
            bool still = !gpio_get(pin);
            if (still) {
                last[pin] = now;
                return true;
            }
        }
    }
    return false;
}

int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
