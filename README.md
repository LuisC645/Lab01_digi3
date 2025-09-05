# Proyecto: Juego de Reacción en Raspberry Pi Pico (SDK)

Este documento sirve como *README* y guía de desarrollo paso a paso. La idea es avanzar por *etapas* claras, con un "Definition of Done" (DoD) en cada una.

---

## Etapa 0 — Estructura y build “en blanco”

* *Objetivo:* proyecto SDK compila y genera UF2.
* *Entregable:* reaction.uf2 básico con loop vacío y printf por USB.
* *Archivos:* CMakeLists.txt, src/main.c.

---

## Etapa 1 — Bring-up de GPIO (LEDs y botones)

* *Objetivo:* verificar wiring y lógica de entradas.
* *Entregable:* LEDs ciclando; lectura de botones por USB.
* *Archivos:* main.c (init GPIO, pulls, helpers).

---

## Etapa 2 — Máquina de estados básica

* *Objetivo:* FSM con IDLE → COUNTDOWN → IDLE.
* *Entregable:* Secuencia LEDs 111→011→001→000 al presionar START.
* *Archivos:* main.c (enum estados, switch-case, temporización).

---

## Etapa 3 — Módulo 7 segmentos (multiplexado)

* *Objetivo:* motor de display funcionando a \~1 kHz.
* *Entregable:* visualización estable de “0123”.
* *Archivos:* sevenseg.h/.c (init, buffer, ISR), main.c (timer repetitivo).

---

## Etapa 4 — Formateo de tiempos en display

* *Objetivo:* mostrar S.mmm (segundo y milisegundos).
* *Entregable:* sevenseg_show_time_ms(ms) convierte 0–9999 ms a 4 dígitos.
* *Archivos:* sevenseg.c (render), main.c (prueba con valores fijos).

---

## Etapa 5 — Ventana aleatoria

* *Objetivo:* espera aleatoria 1–10 s tras el countdown.
* *Entregable:* transición COUNTDOWN → WAIT_RANDOM → LIT_AND_TIMING.
* *Archivos:* main.c (PRNG, tiempo objetivo, transición).

---

## Etapa 6 — Cronometría de reacción

* *Objetivo:* medir tiempo desde que se enciende LED hasta botón correcto.
* *Entregable:* display actualiza en vivo; botón correcto congela valor.
* *Archivos:* main.c (timestamper, elapsed, update a display).

---

## Etapa 7 — Penalización por errores

* *Objetivo:* sumar +1000 ms por botón incorrecto.
* *Entregable:* penalidad visible de inmediato, sin romper el conteo.
* *Archivos:* main.c (detección de botón no correspondiente, saturación).

---

## Etapa 8 — Timeout y reinicio limpio

* *Objetivo:* cancelar a los 10 s sin respuesta.
* *Entregable:* apaga LED, muestra 0000 breve y vuelve a IDLE.
* *Archivos:* main.c (comparación con umbral, reset de variables).

---

## Etapa 9 — Antirrebote y robustez

* *Objetivo:* evitar múltiples lecturas por una pulsación.
* *Entregable:* función btn_pressed() o IRQ con filtro.
* *Archivos:* main.c (debounce/IRQ), posible input.c si se separa.

---

## Etapa 10 — Telemetría y limpieza final

* *Objetivo:* pruebas de caja negra y README.
* *Entregable:* logs útiles (opcionales con #ifdef DEBUG), README con pines, FSM, cómo compilar/cargar y casos de prueba.
* *Archivos:* README.md, comentarios/Doxygen.

---

# Cómo iterar

1. Implementa *una sola etapa*.
2. Compila y carga el .uf2.
3. Valida el DoD.
4. Guarda cambios (commit).
5. Avanza a la siguiente etapa.

---

## Notas finales

* Usa gpio_pull_up() si los botones van a GND.
* Define en sevenseg.h si tu display es de ánodo o cátodo común.
* Haz pruebas con casos: respuesta rápida, lenta, con errores y timeout.
* Documenta wiring y estados al final.
