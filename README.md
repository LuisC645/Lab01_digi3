# Juego de tiempo de reacción — RP2040 + 7 segmentos

Proyecto para Raspberry Pi Pico (RP2040) que mide el tiempo de reacción con 3 LEDs y botones, mostrando el resultado en un display 7 segmentos de 4 dígitos (ánodo común) multiplexado con transistores PNP.

## Estructura del repositorio
```
├─ lib/
│  ├─ debounce/
│  │  ├─ debounce.h
│  │  └─ debounce.c
│  └─ display7seg/
│     ├─ display7seg.h
│     └─ display7seg.c
├─ src/
│  └─ main.c
├─ CMakeLists.txt
└─ README.md
```
---

## 🔌 Mapeo de pines

**Display (segmentos a..g + dp):**

| Segmento | GPIO |
|---------:|:----:|
| a b c d e f g | 0,1,2,3,4,5,6 |
| dp | 7 |

**Dígitos (PNP alto-lado, D0 izquierda → D3 derecha):**

| Dígito | GPIO |
|------:|:----:|
| D0 D1 D2 D3 | 28, 27, 26, 21 |

**Juego (LEDs/Botones):**

| Elemento | GPIO |
|--------:|:----:|
| LED1 / BTN1 | 11 / 20 |
| LED2 / BTN2 | 12 / 18 |
| LED3 / BTN3 | 13 / 19 |
| RST (activo-bajo) | 22 |

> Los botones son **activo-bajo** con **pull-up** (habilitados en la lib `debounce`).

---

