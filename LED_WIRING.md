# 💡 Conexión de LEDs - NeoPixel y APA102

## ✅ Configuración Actual

### 🔴 **NeoPixels (WS2812B) - LEDs Individuales para Pads**

**Tipo:** WS2812B (LEDs RGB individuales, 1 por pad)
**Cantidad:** 4 LEDs
**Conexión:** Cadena serial (daisy-chain)

```
ESP32-S3 GPIO 48 (LED_PADS_PIN)
    │
    ├─> LED 0 (Pad 0 - Kick)
    │       │
    │       └─> LED 1 (Pad 1 - Snare)
    │               │
    │               └─> LED 2 (Pad 2 - HiHat)
    │                       │
    │                       └─> LED 3 (Pad 3 - Tom)
```

**Pinout por LED:**
```
LED WS2812B:
  ┌─────────┐
  │ VDD  DO │─── Data Out (al siguiente LED)
  │         │
  │ GND  DI │─── Data In (desde ESP32 o LED anterior)
  └─────────┘
```

**Conexión:**
- **VDD** → 5V (alimentación externa recomendada si >4 LEDs)
- **GND** → GND común con ESP32
- **DI** (primer LED) → GPIO 48
- **DO** → DI del siguiente LED (daisy-chain)

**Capacitor recomendado:** 100µF entre VDD y GND (cerca de los LEDs)

---

### 🔵 **APA102 (SK9822) - Anillos de Encoders**

**Tipo:** APA102/SK9822 (LEDs RGB con clock, más rápidos que WS2812B)
**Cantidad:** 24 LEDs (2 anillos x 12 LEDs)
**Conexión:** Cadena serial con clock separado

```
ESP32-S3 GPIO 47 (LED_ENC_DATA_PIN)  ─┐
ESP32-S3 GPIO 21 (LED_ENC_CLK_PIN)   ─┤
                                      │
    ┌─────────────────────────────────┘
    │
    ├─> Ring 1 (Encoder Left)
    │    LEDs 0-11
    │       │
    │       └─> Ring 2 (Encoder Right)
    │            LEDs 12-23
```

**Pinout por LED:**
```
LED APA102:
  ┌─────────────┐
  │ VCC   CO    │─── Clock Out (al siguiente LED)
  │             │
  │ GND   DO    │─── Data Out (al siguiente LED)
  │             │
  │ CI    DI    │─── Clock In / Data In
  └─────────────┘
```

**Conexión:**
- **VCC** → 5V (alimentación externa recomendada)
- **GND** → GND común con ESP32
- **CI** (primer LED) → GPIO 21 (Clock)
- **DI** (primer LED) → GPIO 47 (Data)
- **CO/DO** → CI/DI del siguiente LED

**Ventajas de APA102 vs WS2812B:**
- ⚡ Refresh rate más alto (20kHz vs 800Hz)
- 🎯 Clock separado = sin timing crítico
- 🔆 Control de brillo global independiente
- ✅ Más confiable con cables largos

---

## 📊 Comparación de Especificaciones

| Característica | WS2812B (NeoPixel) | APA102 (SK9822) |
|----------------|-------------------|-----------------|
| **Protocolo** | One-wire (timing crítico) | SPI (clock + data) |
| **Refresh Rate** | 800 Hz | 20 kHz |
| **Voltaje** | 5V (tolera 3.3V en pines) | 5V |
| **Corriente por LED** | ~60mA @ full white | ~60mA @ full white |
| **Control de Brillo** | PWM RGB | PWM RGB + 5-bit global |
| **Cables Largos** | ❌ Problemático (>1m) | ✅ Confiable (clock) |
| **Precio** | $ (más barato) | $$ (un poco más caro) |

---

## 🔌 Configuración en Código (Ya está listo)

### **NeoPixels (WS2812B):**

```cpp
// En neopixel_controller.cpp:
FastLED.addLeds<WS2812B, LED_PADS_PIN, GRB>(leds, NUM_PAD_LEDS);
//                ^         ^          ^
//                |         |          └─ Orden de color (GRB típico)
//                |         └─ GPIO 48
//                └─ Tipo de LED
```

### **APA102 (SK9822):**

```cpp
// En sk9822_controller.cpp:
FastLED.addLeds<SK9822, LED_ENC_DATA_PIN, LED_ENC_CLK_PIN, BGR>(leds, NUM_ENCODER_LEDS);
//                ^          ^                ^              ^
//                |          |                |              └─ Orden de color
//                |          |                └─ GPIO 21 (Clock)
//                |          └─ GPIO 47 (Data)
//                └─ Tipo de LED (SK9822 = APA102 compatible)
```

---

## ⚡ Alimentación

### **Cálculo de Corriente:**

**NeoPixels (4 LEDs):**
- Máximo por LED: 60mA @ full white (255, 255, 255)
- Total máximo: **4 × 60mA = 240mA**
- Típico (colores): ~100-150mA

**APA102 (24 LEDs):**
- Máximo por LED: 60mA @ full white
- Total máximo: **24 × 60mA = 1.44A**
- Típico (animaciones): ~500-800mA

**TOTAL PEOR CASO:** ~1.7A @ 5V

### **Recomendaciones:**

1. **Alimentación Separada (Recomendado):**
   ```
   Fuente 5V/2A ─┬─> VDD/VCC de todos los LEDs
                 └─> GND común con ESP32

   ESP32 USB ───> Solo alimenta el MCU
   ```

2. **Desde ESP32 (Solo para testing):**
   - ⚠️ Pin 5V del ESP32: Máximo 500mA
   - ✅ OK para testing con pocos LEDs
   - ❌ NO para uso final con todos los LEDs

3. **Capacitores:**
   - 100µF entre VDD/GND cerca de los LEDs
   - Previene picos de corriente al encender

---

## 🎨 Uso en Tu Código

### **Flash LED al Golpear:**
```cpp
// Automático desde event_dispatcher:
PadConfig& cfg = PadConfigManager::getConfig(padId);
NeoPixelController::flashPad(padId, cfg.ledColorHit, cfg.ledBrightness, 200);
// → Flash instantáneo con color configurado
// → Fade suave a idle en 200ms
```

### **Cambiar Color Idle:**
```cpp
NeoPixelController::setIdleColor(0, 0xFF0000, 80);  // Kick = rojo, 80% brillo
```

### **Test de LEDs:**
```cpp
// En serial, presiona 'l':
NeoPixelController::setAll(CRGB::Red);
delay(500);
NeoPixelController::setAll(CRGB::Green);
delay(500);
NeoPixelController::setAll(CRGB::Blue);
delay(500);
NeoPixelController::clear();
```

### **Encoders - Modo Meter:**
```cpp
SK9822Controller::setMode(ENC_LEFT, ANIM_VALUE_METER);
SK9822Controller::setValue(ENC_LEFT, 0.75f);  // 75% = 9 de 12 LEDs
```

### **Encoders - Pulse al Girar:**
```cpp
// En handleEncoders():
if (event == EncoderHandler::EVENT_ROTATED_CW) {
    SK9822Controller::pulse(encoderId);  // Flash blanco 200ms
}
```

---

## 🧪 Testing Paso a Paso

### **1. Test Individual de NeoPixels:**
```cpp
void testNeoPixels() {
    Serial.println("Testing NeoPixels...");

    for (uint8_t i = 0; i < 4; i++) {
        NeoPixelController::setIdleColor(i, 0xFF0000, 255);  // Rojo
        delay(500);
        NeoPixelController::setIdleColor(i, 0x000000, 0);    // Off
    }
}
```

### **2. Test Individual de APA102:**
```cpp
void testAPA102() {
    Serial.println("Testing APA102...");
    SK9822Controller::testPattern();  // Built-in test
}
```

### **3. Test de Colores:**
```
Rojo:     0xFF0000
Verde:    0x00FF00
Azul:     0x0000FF
Amarillo: 0xFFFF00
Magenta:  0xFF00FF
Cyan:     0x00FFFF
Blanco:   0xFFFFFF
```

---

## 🐛 Troubleshooting

### **NeoPixels no encienden:**
1. ✅ Verificar alimentación 5V
2. ✅ Verificar GND común ESP32 ↔ LEDs
3. ✅ Verificar GPIO 48 conectado a DI del primer LED
4. ✅ Probar cambiar orden de color: `GRB` → `RGB` o `BRG`
5. ✅ Agregar resistencia 470Ω entre GPIO48 y DI (opcional)

### **APA102 no encienden:**
1. ✅ Verificar alimentación 5V
2. ✅ Verificar GND común
3. ✅ Verificar GPIO 47 (Data) y GPIO 21 (Clock) conectados
4. ✅ Probar cambiar orden de color: `BGR` → `RGB` o `GRB`

### **Colores incorrectos:**
- Cambiar orden de color en `addLeds<>()`:
  - `GRB` = Verde-Rojo-Azul
  - `RGB` = Rojo-Verde-Azul
  - `BGR` = Azul-Verde-Rojo

### **LEDs parpadean o comportamiento errático:**
- Agregar capacitor 100µF entre VDD/GND
- Reducir brillo: `FastLED.setBrightness(128)`
- Alimentación externa separada

---

## 📐 Distribución Física Sugerida

```
        ┌─────────────────┐
        │   Drum Module   │
        └─────────────────┘
              │
    ┌─────────┴─────────┐
    │                   │
┌───▼────┐         ┌───▼────┐
│ Pad 0  │         │ Pad 1  │
│ (Kick) │         │ (Snare)│
│  LED   │         │  LED   │
└────────┘         └────────┘

┌────────┐         ┌────────┐
│ Pad 2  │         │ Pad 3  │
│(HiHat) │         │ (Tom)  │
│  LED   │         │  LED   │
└────────┘         └────────┘

        Front Panel
┌───────────────────────────┐
│  ⭕ Enc L    ⭕ Enc R     │
│   (12 LEDs)  (12 LEDs)    │
│                           │
│  [Buttons...]             │
└───────────────────────────┘
```

---

¡Todo listo para conectar! Los controladores ya están implementados y compilados. 🎨✨
