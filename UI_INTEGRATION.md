# 🎨 Integración de UI - NeoPixels, SK9822, Encoders y Botones

## ✅ Componentes Implementados

### 1. **NeoPixels para Pads** (WS2812B x4)
- **Archivo:** [neopixel_controller.h/cpp](src/main_brain/ui/)
- **Características:**
  - Flash instant áneo al golpear
  - Fade suave a color idle
  - Colores configurables por pad
  - Animación a 60 FPS

### 2. **SK9822 para Encoders** (APA102 x24)
- **Archivo:** [sk9822_controller.h/cpp](src/main_brain/ui/)
- **Características:**
  - 2 anillos de 12 LEDs cada uno
  - 6 modos de animación (breathing, meter, spinning, pulse, rainbow, solid)
  - Feedback visual cuando giras encoders
  - Clock separado = sin jitter

### 3. **Encoders Rotativos** (ALPS EC11 x2)
- **Archivo:** [encoder_handler.h/cpp](src/main_brain/ui/)
- **Características:**
  - Decodificación por cuadratura
  - Detección de dirección (CW/CCW)
  - Switch integrado (press, long press)
  - Aceleración opcional

### 4. **Botones Táctiles** (x6)
- **Archivo:** [button_handler.h/cpp](src/main_brain/ui/)
- **Características:**
  - Debounce por software (20ms)
  - Short press, long press (>500ms), double click (<300ms)
  - Hold and repeat (100ms rate)
  - 6 botones: KIT, EDIT, MENU, CLICK, FX, SHIFT

---

## 🚀 Integración en main.cpp

```cpp
#include <Arduino.h>
#include "edrum_config.h"
#include "trigger_scanner.h"
#include "event_dispatcher.h"
#include "pad_config.h"

// UI components
#include "neopixel_controller.h"
#include "sk9822_controller.h"
#include "encoder_handler.h"
#include "button_handler.h"

QueueHandle_t hitQueue;

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   GROOVE DRUM - Main Brain v2.0        ║");
    Serial.println("╚════════════════════════════════════════╝\n");

    // 1. CONFIGURATION
    Serial.println("[INIT] Loading pad configuration...");
    PadConfigManager::init();

    // 2. TRIGGER SYSTEM
    Serial.println("[INIT] Initializing trigger system...");
    hitQueue = xQueueCreate(32, sizeof(HitEvent));
    triggerScanner.begin(hitQueue);
    startTriggerScanner();

    // 3. EVENT DISPATCHER (includes LEDs)
    Serial.println("[INIT] Starting event dispatcher...");
    EventDispatcher::begin();

    // 4. UI COMPONENTS
    Serial.println("[INIT] Initializing UI components...");
    EncoderHandler::begin();
    ButtonHandler::begin();

    // Set encoder rings to breathing animation
    SK9822Controller::setMode(ENC_LEFT, SK9822Controller::ANIM_IDLE_BREATHING);
    SK9822Controller::setMode(ENC_RIGHT, SK9822Controller::ANIM_IDLE_BREATHING);
    SK9822Controller::setColor(ENC_LEFT, CRGB::Cyan);
    SK9822Controller::setColor(ENC_RIGHT, CRGB::Magenta);

    // Set pad idle colors from config
    for (uint8_t i = 0; i < 4; i++) {
        PadConfig& cfg = PadConfigManager::getConfig(i);
        NeoPixelController::setIdleColor(i, cfg.ledColorIdle, cfg.ledBrightness);
    }

    Serial.println("\n✓ System ready\n");
}

void loop() {
    // 1. Process trigger events
    EventDispatcher::processEvents();

    // 2. Update UI input
    EncoderHandler::update();
    ButtonHandler::update();

    // 3. Handle encoder events
    handleEncoders();

    // 4. Handle button events
    handleButtons();

    delay(1);  // Yield to watchdog
}

// ============================================================================
// ENCODER HANDLING
// ============================================================================

void handleEncoders() {
    for (uint8_t enc = 0; enc < NUM_ENCODERS; enc++) {
        EncoderHandler::EncoderEvent event = EncoderHandler::pollEvent(enc);

        switch (event) {
            case EncoderHandler::EVENT_ROTATED_CW:
                Serial.printf("Encoder %d: CW (pos=%ld)\n", enc, EncoderHandler::getPosition(enc));
                SK9822Controller::pulse(enc);  // Visual feedback
                // TODO: Adjust parameter (volume, threshold, etc.)
                break;

            case EncoderHandler::EVENT_ROTATED_CCW:
                Serial.printf("Encoder %d: CCW (pos=%ld)\n", enc, EncoderHandler::getPosition(enc));
                SK9822Controller::pulse(enc);
                break;

            case EncoderHandler::EVENT_SWITCH_PRESSED:
                Serial.printf("Encoder %d: Switch pressed\n", enc);
                // TODO: Enter edit mode
                break;

            case EncoderHandler::EVENT_SWITCH_LONG_PRESS:
                Serial.printf("Encoder %d: Long press\n", enc);
                // TODO: Reset to default
                break;

            default:
                break;
        }
    }
}

// ============================================================================
// BUTTON HANDLING
// ============================================================================

void handleButtons() {
    for (uint8_t btn = 0; btn < NUM_BUTTONS; btn++) {
        ButtonHandler::ButtonEvent event = ButtonHandler::pollEvent((ButtonID)btn);

        switch (event) {
            case ButtonHandler::EVENT_CLICK:
                handleButtonClick((ButtonID)btn);
                break;

            case ButtonHandler::EVENT_LONG_PRESS:
                handleButtonLongPress((ButtonID)btn);
                break;

            case ButtonHandler::EVENT_DOUBLE_CLICK:
                handleButtonDoubleClick((ButtonID)btn);
                break;

            default:
                break;
        }
    }
}

void handleButtonClick(ButtonID btn) {
    switch (btn) {
        case BTN_KIT:
            Serial.println("Button: KIT clicked");
            // TODO: Change kit/preset
            break;

        case BTN_EDIT:
            Serial.println("Button: EDIT clicked");
            // TODO: Enter edit mode
            break;

        case BTN_MENU:
            Serial.println("Button: MENU clicked");
            // TODO: Show menu
            break;

        case BTN_CLICK:
            Serial.println("Button: CLICK clicked");
            // TODO: Toggle metronome
            break;

        case BTN_FX:
            Serial.println("Button: FX clicked");
            // TODO: Apply FX
            break;

        case BTN_SHIFT:
            Serial.println("Button: SHIFT clicked");
            // TODO: Shift mode
            break;
    }
}

void handleButtonLongPress(ButtonID btn) {
    Serial.printf("Button %d: Long press\n", btn);
    // TODO: Alternative functions
}

void handleButtonDoubleClick(ButtonID btn) {
    Serial.printf("Button %d: Double click\n", btn);
    // TODO: Quick actions
}
```

---

## 🎨 Ejemplo de Uso: LED Feedback al Golpear

El sistema ya está integrado con `EventDispatcher`. Cuando se detecta un hit:

```cpp
// En event_dispatcher.cpp:
void EventDispatcher::processEvents() {
    HitEvent event;
    while (xQueueReceive(hitQueue, &event, 0) == pdTRUE) {
        PadConfig& cfg = PadConfigManager::getConfig(event.padId);

        // LED flash automático ✓
        LEDRequest ledReq = {
            .padId = event.padId,
            .color = cfg.ledColorHit,        // Color del hit (desde config)
            .brightness = cfg.ledBrightness,
            .fadeDuration = cfg.ledFadeDuration
        };
        xQueueSend(ledQueue, &ledReq, 0);

        // La tarea LED worker llama a:
        // NeoPixelController::flashPad(padId, color, brightness, fadeDuration);
        // → LED flash instantáneo
        // → Fade suave a idle color en 200ms
    }
}
```

---

## 🎯 Modos de Animación SK9822

### Modo 1: Idle Breathing (Default)
```cpp
SK9822Controller::setMode(ENC_LEFT, ANIM_IDLE_BREATHING);
SK9822Controller::setColor(ENC_LEFT, CRGB::Cyan);
```
Respiración suave, 2 segundos de período.

### Modo 2: Value Meter
```cpp
SK9822Controller::setMode(ENC_LEFT, ANIM_VALUE_METER);
SK9822Controller::setValue(ENC_LEFT, 0.75f);  // 75% = 9 de 12 LEDs encendidos
```
Perfecto para mostrar volumen, threshold, etc.

### Modo 3: Spinning
```cpp
SK9822Controller::setMode(ENC_LEFT, ANIM_SPINNING);
```
Efecto cometa rotando (velocidad ajustable).

### Modo 4: Pulse (Feedback al Girar)
```cpp
// Llamar cuando el encoder gira:
SK9822Controller::pulse(ENC_LEFT);
```
Flash blanco de 200ms, luego vuelve a modo anterior.

### Modo 5: Rainbow
```cpp
SK9822Controller::setMode(ENC_LEFT, ANIM_RAINBOW);
```
Ciclo de arcoíris continuo.

### Modo 6: Solid Color
```cpp
SK9822Controller::setMode(ENC_LEFT, ANIM_SOLID);
SK9822Controller::setColor(ENC_LEFT, CRGB::Red);
```
Color sólido estático.

---

## 🔧 Comandos de Test Serial

Agregar al `handleSerialCommand()`:

```cpp
case 'l':  // LED test
    Serial.println("Testing LEDs...");
    NeoPixelController::setAll(CRGB::Red);
    delay(500);
    NeoPixelController::setAll(CRGB::Green);
    delay(500);
    NeoPixelController::setAll(CRGB::Blue);
    delay(500);
    NeoPixelController::clear();
    break;

case 'k':  // SK9822 test
    SK9822Controller::testPattern();
    break;

case 'e':  // Encoder info
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        Serial.printf("Encoder %d: pos=%ld, pressed=%d\n",
                      i,
                      EncoderHandler::getPosition(i),
                      EncoderHandler::isSwitchPressed(i));
    }
    break;

case 'b':  // Button info
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        Serial.printf("Button %d: pressed=%d\n",
                      i,
                      ButtonHandler::isPressed((ButtonID)i));
    }
    break;
```

---

## 📊 Uso de Memoria

| Componente | RAM | Flash |
|------------|-----|-------|
| NeoPixels (4 LEDs) | ~200 bytes | ~2 KB |
| SK9822 (24 LEDs) | ~400 bytes | ~3 KB |
| Encoders | ~100 bytes | ~1 KB |
| Botones | ~200 bytes | ~1 KB |
| **TOTAL UI** | **~900 bytes** | **~7 KB** |

---

## 🎨 Configuración de Colores por Pad

En `pad_config.h`, cada pad ya tiene:

```cpp
struct PadConfig {
    uint32_t ledColorHit;   // Color al golpear (0xRRGGBB)
    uint32_t ledColorIdle;  // Color en reposo
    uint8_t ledBrightness;  // 0-255
    uint16_t ledFadeDuration; // Tiempo de fade en ms
};

// Ejemplo - Kick rojo:
const PadConfig DEFAULT_KICK_CONFIG = {
    .ledColorHit = 0xFF0000,    // Rojo brillante
    .ledColorIdle = 0x330000,   // Rojo oscuro
    .ledBrightness = 80,
    .ledFadeDuration = 200
};
```

---

## 🔌 Conexiones de Hardware

### NeoPixels (WS2812B)
- **Data:** GPIO 48
- **Power:** 5V, GND
- **Orden:** Pad 0 (Kick) → Pad 1 (Snare) → Pad 2 (HiHat) → Pad 3 (Tom)

### SK9822 (Encoder Rings)
- **Data:** GPIO 47
- **Clock:** GPIO 21
- **Power:** 5V, GND
- **Orden:** Ring 1 (LED 0-11) → Ring 2 (LED 12-23)

### Encoders
| Encoder | Pin A | Pin B | Switch |
|---------|-------|-------|--------|
| Left | GPIO 35 | GPIO 36 | GPIO 37 |
| Right | GPIO 38 | GPIO 39 | GPIO 40 |

### Botones (Active LOW con pullup)
| Button | GPIO |
|--------|------|
| KIT | 1 |
| EDIT | 2 |
| MENU | 42 |
| CLICK | 41 |
| FX | 40 |
| SHIFT | 39 |

---

## 🎯 Próximos Pasos

1. **Compilar y probar LEDs:**
   ```bash
   pio run -t upload -t monitor
   # Presiona 'l' para test de NeoPixels
   # Presiona 'k' para test de SK9822
   ```

2. **Probar encoders:**
   - Gira encoders → Deberías ver pulsos en los rings
   - Presiona switches → Log en serial

3. **Probar botones:**
   - Presiona cualquier botón → Log con tipo de evento

4. **Integrar con GUI:**
   - Encoders cambien parámetros
   - Botones naveguen menús
   - LEDs indiquen estado

---

## 📚 Archivos Creados

1. [neopixel_controller.h](src/main_brain/ui/neopixel_controller.h)
2. [neopixel_controller.cpp](src/main_brain/ui/neopixel_controller.cpp)
3. [sk9822_controller.h](src/main_brain/ui/sk9822_controller.h)
4. [sk9822_controller.cpp](src/main_brain/ui/sk9822_controller.cpp)
5. [encoder_handler.h](src/main_brain/ui/encoder_handler.h)
6. [encoder_handler.cpp](src/main_brain/ui/encoder_handler.cpp)
7. [button_handler.h](src/main_brain/ui/button_handler.h)
8. [button_handler.cpp](src/main_brain/ui/button_handler.cpp)

**event_dispatcher.cpp actualizado** para usar NeoPixels y SK9822 reales.

---

¡Todo listo para compilar! 🚀
