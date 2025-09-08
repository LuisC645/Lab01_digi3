# Juego de Tiempos de Reacción – Bitácora de Desarrollo

## Avances
- Configuración del proyecto con **SDK de Raspberry Pi Pico** en C.
- Pruebas iniciales con **un LED y un botón** (GPIO16 y GPIO14).
- Extensión a **2 LEDs + 2 botones** y luego a **3 LEDs + 3 botones**.
- Implementación del **flujo básico del juego**:
  1. Animación de inicio con LEDs.
  2. Espera aleatoria (1–5 s).
  3. LED aleatorio se enciende.
  4. Usuario presiona el botón asociado:
     - Correcto → mide tiempo de reacción.
     - Incorrecto → penalización de **+1000 ms**.
  5. Si pasan **10 s totales** (incluyendo penalizaciones) → se corta la ronda automáticamente.
- Ajuste de salida por USB serial:
  - `stdout` configurado **sin buffer** para evitar lag al imprimir.
  - Impresión inmediata de resultados.

## Observaciones
- Penalizaciones múltiples se acumulan hasta que el tiempo total llega a **9999 ms** (tope).
- Se detectó “lag” en los `printf` -> **no corregido**.
- Actualmente los botones **no tienen antirrebote**, lo que puede generar penalizaciones extra por ruido mecánico.

## Pendientes
- [ ] **Antirrebote (debounce)** en botones (20–30 ms) para evitar lecturas dobles.
- [ ] **Display de 7 segmentos (3 dígitos + signo):**
  - Mostrar tiempos en ms (0–9999).
  - Multiplexado de los dígitos.
  - Indicación de penalización.
- [ ] **Botón de reinicio de juego** dedicado.
  - Escenarios de prueba.
  - Requisitos de entrega (diagrama de conexiones, explicación de código, etc.).

## Notas rápidas
- LEDs conectados con resistencias de 1k
- Botones con **pull-up interno** (presionado = nivel bajo).
