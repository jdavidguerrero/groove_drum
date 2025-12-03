# E-DRUM CONTROLLER - Especificación Técnica Completa
## Prompt para Desarrollo en Claude Code

---

## 🎯 OBJETIVO DEL PROYECTO

Desarrollar el firmware completo para un controlador de percusión electrónica (E-Drum) estilo Roland SPD-SX / HPD-20, utilizando dos microcontroladores ESP32-S3 comunicados por UART.

---

## 🏗️ ARQUITECTURA DE HARDWARE

### Sistema Dual-MCU

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         MCU #1 - MAIN BRAIN                             │
│                      (ESP32-S3 DevKitC-1 o similar)                     │
│                                                                         │
│  ENTRADAS:                          SALIDAS:                            │
│  ├─ 4× Piezos (ADC) ───────────────├─ PCM5102 DAC (I2S)                │
│  ├─ 2× Encoders ALPS (GPIO)        ├─ MIDI OUT (UART)                  │
│  ├─ 6× Botones (GPIO)              ├─ 4× WS2812B LEDs (Pads)           │
│  └─ SD Card (SPI)                  ├─ 12× SK9822 LEDs (Encoder Rings)  │
│                                    └─ UART TX/RX → MCU#2               │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                              UART 921600 baud
                                    │
┌─────────────────────────────────────────────────────────────────────────┐
│                         MCU #2 - DISPLAY                                │
│               (Waveshare ESP32-S3-Touch-LCD-1.28)                       │
│                                                                         │
│  HARDWARE INTEGRADO:                SALIDAS ADICIONALES:                │
│  ├─ Display GC9A01 240×240 (SPI)   └─ 12× WS2812B Ring (GPIO15)        │
│  ├─ Touch CST816S (I2C)                                                 │
│  └─ UART RX/TX ← MCU#1                                                  │
└─────────────────────────────────────────────────────────────────────────┘
```

### Pinout MCU #1 (Main Brain)

| Función | GPIO | Notas |
|---------|------|-------|
| Trigger Pad 0 | 4 | ADC1_CH3 |
| Trigger Pad 1 | 5 | ADC1_CH4 |
| Trigger Pad 2 | 6 | ADC1_CH5 |
| Trigger Pad 3 | 7 | ADC1_CH6 |
| I2S BCLK | 26 | PCM5102 |
| I2S LRCK | 25 | PCM5102 |
| I2S DOUT | 27 | PCM5102 |
| SD CS | 15 | SPI |
| SD MOSI | 13 | SPI |
| SD MISO | 12 | SPI |
| SD SCK | 14 | SPI |
| MIDI TX | 17 | 31250 baud |
| Encoder L A | 35 | Rotación |
| Encoder L B | 36 | Rotación |
| Encoder L SW | 37 | Push button |
| Encoder R A | 38 | Rotación |
| Encoder R B | 39 | Rotación |
| Encoder R SW | 40 | Push button |
| BTN KIT | 1 | Pull-up |
| BTN EDIT | 2 | Pull-up |
| BTN MENU | 42 | Pull-up |
| BTN CLICK | 41 | Pull-up |
| BTN FX | 40 | Pull-up |
| BTN SHIFT | 39 | Pull-up |
| LED Pads Data | 48 | WS2812B ×4 |
| LED Encoder Data | 47 | SK9822 Data |
| LED Encoder Clock | 21 | SK9822 Clock |
| UART TX → Display | 43 | 921600 baud |
| UART RX ← Display | 44 | 921600 baud |

### Pinout MCU #2 (Waveshare Display)

| Función | GPIO | Notas |
|---------|------|-------|
| TFT MOSI | 11 | GC9A01 |
| TFT SCLK | 12 | GC9A01 |
| TFT CS | 10 | GC9A01 |
| TFT DC | 8 | GC9A01 |
| TFT RST | 14 | GC9A01 |
| TFT Backlight | 2 | PWM capable |
| Touch SDA | 6 | CST816S |
| Touch SCL | 7 | CST816S |
| Touch INT | 5 | CST816S |
| Touch RST | 13 | CST816S |
| LED Ring Data | 15 | WS2812B ×12 |
| UART RX ← Main | 33 | 921600 baud |
| UART TX → Main | 21 | 921600 baud |

---

## 📡 PROTOCOLO DE COMUNICACIÓN UART

### Estructura de Frame

```
┌──────┬──────┬────────┬─────────────────┬───────┬──────┐
│ SYNC │ LEN  │  CMD   │     PAYLOAD     │  CRC  │ END  │
│ 0xAA │ 1B   │  1B    │    0-16 bytes   │  1B   │ 0x55 │
└──────┴──────┴────────┴─────────────────┴───────┴──────┘
```

- **SYNC**: 0xAA (inicio de frame)
- **LEN**: Longitud del payload (0-16)
- **CMD**: Comando (ver tabla)
- **PAYLOAD**: Datos específicos del comando
- **CRC**: CRC-8 (polynomial 0x07) sobre LEN+CMD+PAYLOAD
- **END**: 0x55 (fin de frame)

### Comandos MCU#1 → MCU#2

| CMD | Nombre | Payload | Estructura |
|-----|--------|---------|------------|
| 0x01 | PAD_HIT | 3B | `{pad_id, velocity, flags}` |
| 0x02 | PAD_RELEASE | 1B | `{pad_id}` |
| 0x03 | ENCODER_ROTATE | 3B | `{encoder_id, delta(signed), flags}` |
| 0x04 | ENCODER_PUSH | 2B | `{encoder_id, state}` |
| 0x05 | BUTTON_EVENT | 2B | `{button_id, state}` |
| 0x10 | KIT_INFO | 12B | `{kit_number, kit_name[10], flags}` |
| 0x11 | PAD_CONFIG | 8B | `{pad_id, note, channel, vol, pan, pitch, decay, color}` |
| 0x12 | GLOBAL_STATE | 6B | `{bpm(16), master_vol, click, usb_mode, sync}` |
| 0x13 | MIDI_ACTIVITY | 1B | `{flags}` |
| 0x20 | SYNC_REQUEST | 0B | - |
| 0x21 | ACK | 1B | `{acked_cmd}` |
| 0xFE | ERROR | 2B | `{error_code, context}` |
| 0xFF | HEARTBEAT | 4B | `{uptime_ms(32)}` |

### Comandos MCU#2 → MCU#1

| CMD | Nombre | Payload | Estructura |
|-----|--------|---------|------------|
| 0x81 | PARAM_CHANGE | 4B | `{target, target_id, param_id, value}` |
| 0x82 | KIT_SELECT | 1B | `{kit_number}` |
| 0x83 | VIEW_CHANGED | 1B | `{view_id}` |
| 0x90 | REQUEST_KIT_INFO | 1B | `{kit_number}` |
| 0x91 | REQUEST_PAD_CONFIG | 1B | `{pad_id}` |
| 0xA1 | ACK | 1B | `{acked_cmd}` |
| 0xFE | ERROR | 2B | `{error_code, context}` |
| 0xFF | HEARTBEAT | 4B | `{uptime_ms(32)}` |

### IDs de Referencia

```c
// Botones
enum { BTN_KIT=0, BTN_EDIT=1, BTN_MENU=2, BTN_CLICK=3, BTN_FX=4, BTN_SHIFT=5 };

// Encoders
enum { ENC_LEFT=0, ENC_RIGHT=1 };

// Vistas UI
enum { VIEW_PERFORMANCE=0, VIEW_PAD_EDIT=1, VIEW_MIXER=2, VIEW_SETTINGS=3 };

// Parámetros de Pad
enum { PARAM_VOLUME=0, PARAM_PAN=1, PARAM_PITCH=2, PARAM_DECAY=3, PARAM_NOTE=4, PARAM_CHANNEL=5 };

// Parámetros Globales
enum { PARAM_MASTER_VOL=0x10, PARAM_BPM=0x11, PARAM_CLICK_EN=0x12, PARAM_USB_MODE=0x13 };

// Estados de botón/encoder
enum { STATE_RELEASED=0, STATE_PRESSED=1, STATE_LONG_PRESS=2, STATE_DOUBLE_CLICK=3 };

// Códigos de error
enum { ERR_CRC=1, ERR_UNKNOWN_CMD=2, ERR_INVALID_LEN=3, ERR_TIMEOUT=4, ERR_OVERFLOW=5 };
```

---

## 🎨 ESPECIFICACIÓN UI/UX - "ORBITAL DARK"

### Filosofía de Diseño

- **Pantalla circular 240×240**: UI céntrica y radial
- **Concepto "Reactor"**: Info vital en el centro, feedback en la órbita
- **Estética**: Retro-Futurista / Cyberpunk limpio

### Paleta de Colores

| Uso | Color | Hex |
|-----|-------|-----|
| Fondo | Negro Absoluto | #000000 |
| Primario (Info) | Blanco OLED | #FFFFFF |
| Acento 1 (Selección) | Cian Neón | #00FFFF |
| Acento 2 (Valores) | Ámbar | #FFA500 |
| Alerta/Record | Rojo Vivo | #FF0000 |

### Vistas (Screens)

#### VISTA 1: PERFORMANCE (Home)
```
        ╭─────────────────╮
       ╱    ┌─────────┐    ╲
      │     │   01    │     │    ← Número de Kit (grande)
      │     │808 CLASSIC│   │    ← Nombre del Kit
      │     │   120   │     │    ← BPM
       ╲    └─────────┘    ╱
        ╰─────────────────╯
              ↑
    Arcos orbitales invisibles que
    flashean al golpear cada pad
```

#### VISTA 2: PAD EDIT
```
        ╭─────────────────╮
       ╱   SNARE - P2      ╲     ← Header pequeño
      │  ╭───────────────╮  │
      │  │    PITCH      │  │    ← Parámetro actual
      │  │     +12       │  │    ← Valor
      │  ╰───────────────╯  │
       ╲  ▓▓▓▓▓▓▓▓░░░░░░   ╱     ← Arco de progreso 270°
        ╰─────────────────╯
```
- Encoder IZQ: Cambia parámetro (Vol→Pan→Pitch→Decay→Filter)
- Encoder DER: Modifica valor

#### VISTA 3: MIXER
```
        ╭─────────────────╮
       ╱  ╭─╮ ╭─╮ ╭─╮ ╭─╮  ╲
      │   │█│ │▓│ │▒│ │░│   │    ← 4 arcos concéntricos
      │   │█│ │▓│ │▒│ │░│   │       representando volumen
      │   │█│ │▓│ │▒│ │░│   │       de cada pad
       ╲  ╰─╯ ╰─╯ ╰─╯ ╰─╯  ╱
        ╰─────────────────╯
         P1  P2  P3  P4
```

#### VISTA 4: SETTINGS
```
        ╭─────────────────╮
       ╱                   ╲
      │    ○ MIDI CH: 10   │     ← Roller/lista curva
      │    ● VEL CURVE:LOG │     ← Item seleccionado
      │    ○ SENSITIVITY:80│
       ╲                   ╱
        ╰─────────────────╯
```

### Interacción con Controles

| Control | Giro | Push |
|---------|------|------|
| Encoder IZQ | Navegar/Scroll | Confirmar/Enter |
| Encoder DER | Modificar valor | Fine/Coarse o Back |

| Botón | Función | + SHIFT |
|-------|---------|---------|
| KIT | Ir a Performance | - |
| EDIT | Modo edición (golpear pad) | - |
| MENU | Settings globales | - |
| CLICK | Toggle metrónomo | - |
| FX | Envío FX / Panic | Master Mute |
| SHIFT | Modificador | - |

---

## 💡 SISTEMA DE LEDs

### Ring de Pantalla (12× WS2812B) - MCU#2
- **Idle**: Respiración cian suave (2s ciclo)
- **Pad Hit**: Flash del cuadrante correspondiente (3 LEDs por pad)
- **Navegación**: Indicador de posición en menús

### LEDs de Pads (4× WS2812B) - MCU#1
- **Idle**: Brillo tenue del color del instrumento
- **Hit**: Flash blanco 10ms + decay 150ms
- **Colores**: Kick=Rojo, Snare=Azul, HiHat=Amarillo, Tom=Verde

### Encoder Rings (2× 16 SK9822) - MCU#1
- **Performance**: Animación respiración o Master Volume
- **Edit Mode**: Indicador físico del valor (arco LED)

---

## 📁 ESTRUCTURA DEL PROYECTO

```
edrum-controller/
├── platformio.ini
├── shared/
│   ├── protocol/
│   │   ├── protocol.h
│   │   └── protocol.cpp
│   └── config/
│       └── edrum_config.h
├── src/
│   ├── main_brain/
│   │   ├── main.cpp
│   │   ├── triggers.h / .cpp
│   │   ├── audio_engine.h / .cpp
│   │   ├── leds_main.h / .cpp
│   │   ├── encoders.h / .cpp
│   │   ├── buttons.h / .cpp
│   │   ├── midi_out.h / .cpp
│   │   ├── sd_manager.h / .cpp
│   │   └── comm_main.h / .cpp
│   └── display/
│       ├── main.cpp
│       ├── lv_conf.h
│       ├── ui/
│       │   ├── ui_manager.h / .cpp
│       │   ├── ui_theme.h / .cpp
│       │   └── screens/
│       │       ├── scr_performance.h / .cpp
│       │       ├── scr_pad_edit.h / .cpp
│       │       ├── scr_mixer.h / .cpp
│       │       └── scr_settings.h / .cpp
│       ├── leds_ring.h / .cpp
│       ├── touch_driver.h / .cpp
│       └── comm_display.h / .cpp
└── test/
```

---

## ⚙️ CONFIGURACIÓN PLATFORMIO

```ini
[platformio]
default_envs = main_brain, display

[env]
platform = espressif32@6.9.0
framework = arduino
monitor_speed = 115200
build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1

[env:main_brain]
board = esp32-s3-devkitc-1
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.psram = enabled
build_src_filter = +<main_brain/> +<../shared/>
build_flags = 
    ${env.build_flags}
    -DMCU_MAIN_BRAIN=1
lib_deps = 
    fastled/FastLED@^3.6.0
    madhephaestus/ESP32Encoder@^0.10.1
    fortyseveneffects/MIDI Library@^5.0.2
board_build.partitions = huge_app.csv

[env:display]
board = esp32-s3-devkitc-1
board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.psram = enabled
build_src_filter = +<display/> +<../shared/>
build_flags = 
    ${env.build_flags}
    -DMCU_DISPLAY=1
    -DLV_CONF_INCLUDE_SIMPLE=1
    -DUSER_SETUP_LOADED=1
    -DGC9A01_DRIVER=1
    -DTFT_WIDTH=240
    -DTFT_HEIGHT=240
lib_deps = 
    lvgl/lvgl@^8.3.11
    bodmer/TFT_eSPI@^2.5.43
    fastled/FastLED@^3.6.0
board_build.partitions = huge_app.csv
```

---

## 🚀 INSTRUCCIONES PARA CLAUDE CODE

### Fase 1: Estructura Base
1. Crear la estructura de carpetas del proyecto
2. Implementar `platformio.ini` completo
3. Crear `shared/config/edrum_config.h` con todos los defines
4. Implementar `shared/protocol/protocol.h` y `protocol.cpp`

### Fase 2: MCU#1 - Main Brain
1. Implementar `comm_main.h/.cpp` - Comunicación UART
2. Implementar `triggers.h/.cpp` - Lectura de piezos con detección de golpe
3. Implementar `encoders.h/.cpp` - Lectura de encoders con debounce
4. Implementar `buttons.h/.cpp` - Lectura de botones con estados
5. Implementar `leds_main.h/.cpp` - Control de WS2812B y SK9822
6. Implementar `midi_out.h/.cpp` - Envío MIDI
7. Implementar `audio_engine.h/.cpp` - Placeholder para audio I2S
8. Implementar `main.cpp` con FreeRTOS dual-core

### Fase 3: MCU#2 - Display
1. Configurar `lv_conf.h` para pantalla circular
2. Implementar driver de display GC9A01 con TFT_eSPI
3. Implementar `touch_driver.h/.cpp` para CST816S
4. Implementar `comm_display.h/.cpp` - Comunicación UART
5. Implementar `leds_ring.h/.cpp` - Ring de 12 NeoPixels
6. Implementar `ui_theme.h/.cpp` - Tema "Orbital Dark"
7. Implementar cada pantalla:
   - `scr_performance.cpp`
   - `scr_pad_edit.cpp`
   - `scr_mixer.cpp`
   - `scr_settings.cpp`
8. Implementar `ui_manager.h/.cpp` - Gestión de vistas y transiciones
9. Implementar `main.cpp`

### Fase 4: Integración
1. Probar comunicación UART entre ambos MCUs
2. Verificar flujo completo: golpe → display → feedback LED
3. Implementar guardado/carga de kits en SD

---

## 📋 CRITERIOS DE CALIDAD

### Código
- [ ] Usar `#pragma once` en headers
- [ ] Comentarios en inglés
- [ ] Nombres descriptivos (camelCase para funciones, UPPER_CASE para defines)
- [ ] Evitar delays bloqueantes - usar FreeRTOS
- [ ] Manejar errores con códigos de retorno

### Performance
- [ ] Latencia trigger-to-LED < 2ms
- [ ] UI a 60 FPS mínimo
- [ ] UART sin pérdida de frames
- [ ] LEDs actualizados sin glitches

### UX
- [ ] Transiciones suaves entre vistas (200ms ease-out)
- [ ] Feedback visual inmediato en cada acción
- [ ] Coherencia visual entre pantalla y LEDs físicos

---

## 🎯 COMANDO INICIAL SUGERIDO

```
Crea la estructura completa del proyecto PlatformIO para el E-Drum Controller 
siguiendo la especificación. Comienza con:
1. platformio.ini
2. shared/config/edrum_config.h
3. shared/protocol/protocol.h y protocol.cpp

Asegúrate de que compile para ambos entornos (main_brain y display).
```

---

## 📚 REFERENCIAS

- [Waveshare ESP32-S3-Touch-LCD-1.28](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28)
- [LVGL Documentation](https://docs.lvgl.io/8.3/)
- [FastLED Library](https://fastled.io/)
- [ESP32-S3 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- [TFT_eSPI Setup](https://github.com/Bodmer/TFT_eSPI)

---

*Documento generado para desarrollo con Claude Code - E-Drum Controller v1.0*