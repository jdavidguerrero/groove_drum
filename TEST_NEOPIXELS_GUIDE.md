# 🧪 Guía de Test - NeoPixels WS2812B

## 📋 Qué Hace Este Test

El test verifica todos los aspectos de tus 4 NeoPixels:

1. ✅ **Conexión correcta** - Verifica que todos los LEDs respondan
2. ✅ **Orden de colores** - Detecta si es GRB, RGB o BGR
3. ✅ **Brillo** - Prueba desde 100% hasta 10%
4. ✅ **LEDs individuales** - Enciende cada LED por separado
5. ✅ **Rainbow** - Ciclo de colores fluido
6. ✅ **Flash simulation** - Simula golpes de drum con fade
7. ✅ **Colores diferentes** - Muestra cada pad con su color

---

## 🔌 Conexión Física

Antes de subir el código, conecta los LEDs:

```
ESP32-S3 GPIO 48 ──→ DI (Data In) del primer LED

LED 0 (Kick)  ─DO→ DI─ LED 1 (Snare) ─DO→ DI─ LED 2 (HiHat) ─DO→ DI─ LED 3 (Tom)

Todos los LEDs:
  VDD → 5V (puede ser del ESP32 para test, usar fuente externa después)
  GND → GND común con ESP32
```

**⚠️ IMPORTANTE:**
- Conectar **GND común** entre ESP32 y LEDs
- Para test inicial, puedes usar el 5V del ESP32
- Para uso final, usar fuente externa 5V/2A

---

## 🚀 Compilar y Subir

### Opción 1: Desde Terminal

```bash
# Compilar test de NeoPixels
~/.platformio/penv/bin/pio run --environment test_neopixels

# Subir y abrir monitor serial
~/.platformio/penv/bin/pio run --environment test_neopixels -t upload -t monitor
```

### Opción 2: Desde VSCode

1. Abre `platformio.ini`
2. Cambia la línea 5:
   ```ini
   default_envs = test_neopixels
   ```
3. Presiona botón **Upload** (→) en la barra inferior
4. Presiona botón **Monitor** (🔌) para ver salida serial

---

## 📺 Qué Verás en el Monitor Serial

```
╔════════════════════════════════════════╗
║   TEST NEOPIXELS - WS2812B            ║
╚════════════════════════════════════════╝

✅ FastLED inicializado
   Pin: GPIO 48
   LEDs: 4
   Tipo: WS2812B (GRB)

Iniciando secuencia de test...

═══════════════════════════════════════
 TEST 1: Todos los LEDs - Rojo
═══════════════════════════════════════
[Los 4 LEDs se ponen rojos por 2 segundos]

═══════════════════════════════════════
 TEST 2: Todos los LEDs - Verde
═══════════════════════════════════════
[...]
```

---

## 🎨 Secuencia Completa de Tests

| Test | Duración | Qué Hace | Qué Verificar |
|------|----------|----------|---------------|
| **1. Rojo** | 2s | Todos rojos | ¿Todos encienden? ¿Color correcto? |
| **2. Verde** | 2s | Todos verdes | ¿Verde o azul? (si azul → orden incorrecto) |
| **3. Azul** | 2s | Todos azules | ¿Azul o verde? |
| **4. Blanco** | 2s | Todos blancos | ¿Brillo uniforme? |
| **5. Individual** | 4s | Cada LED por separado | ¿Todos responden? ¿Orden correcto 0→3? |
| **6. Rainbow** | ~5s | Ciclo de colores | ¿Transición suave? |
| **7. Brillo** | 5s | 100% → 10% | ¿Fade suave? |
| **8. Flash** | 5s | Simula golpes | ¿Flash y fade funcionan? |
| **9. Colores** | 3s | Rojo/Verde/Cyan/Azul | ¿Cada LED diferente? |

**Total:** ~30 segundos, luego se repite automáticamente.

---

## 🐛 Troubleshooting

### ❌ **Ningún LED enciende**

**Posibles causas:**
1. ✅ Verificar conexión GPIO 48 → DI primer LED
2. ✅ Verificar GND común ESP32 ↔ LEDs
3. ✅ Verificar alimentación 5V
4. ✅ Probar con resistencia 470Ω entre GPIO48 y DI

**Código de diagnóstico:**
```cpp
// En test_neopixels.cpp línea 28, cambiar:
#define COLOR_ORDER GRB
// Por:
#define COLOR_ORDER RGB  // Probar diferentes órdenes
```

### ❌ **Solo el primer LED funciona**

**Causa:** Conexión rota entre LEDs o LED defectuoso.

**Solución:**
1. Verificar conexión DO → DI entre LEDs
2. Reemplazar LED problemático
3. Verificar soldaduras

### ❌ **Colores incorrectos (Rojo es verde, etc.)**

**Causa:** Orden de colores incorrecto.

**Solución en código:**
```cpp
// En test_neopixels.cpp línea 28:

// Si Rojo muestra Verde:
#define COLOR_ORDER GRB  // En vez de RGB

// Si Rojo muestra Azul:
#define COLOR_ORDER BGR  // En vez de RGB

// Probar todas:
// GRB, RGB, BGR, RBG, GBR, BRG
```

### ❌ **LEDs parpadean o se apagan aleatoriamente**

**Causa:** Alimentación insuficiente o ruido.

**Solución:**
1. Agregar capacitor 100µF entre VDD y GND (cerca de los LEDs)
2. Usar fuente externa 5V/2A (no USB del ESP32)
3. Reducir brillo en código:
   ```cpp
   FastLED.setBrightness(128);  // 50% en vez de 255
   ```

### ❌ **LEDs se encienden con colores random al inicio**

**Normal:** Comportamiento típico al encender sin comando inicial.

**Solución (opcional):**
```cpp
// Agregar en setup():
FastLED.clear();
FastLED.show();
delay(100);
```

---

## 🔧 Modificar el Test

### Cambiar velocidad de animaciones:

```cpp
// En loop(), buscar delay() y cambiar valores:
delay(2000);  // 2 segundos
delay(1000);  // 1 segundo
delay(500);   // 0.5 segundos
```

### Probar solo un test específico:

```cpp
void loop() {
    // Comentar todos los tests excepto el que quieras:

    // TEST 1: Solo rojo
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(1000);
}
```

### Agregar tu propio test:

```cpp
void loop() {
    // ... tests existentes ...

    // Mi test personalizado
    Serial.println("MI TEST");
    leds[0] = CRGB::Purple;
    leds[1] = CRGB::Orange;
    leds[2] = CRGB::Yellow;
    leds[3] = CRGB::Pink;
    FastLED.show();
    delay(3000);
}
```

---

## 📊 Colores Disponibles en FastLED

```cpp
// Colores básicos:
CRGB::Red
CRGB::Green
CRGB::Blue
CRGB::White
CRGB::Black  // Apagado

// Colores extendidos:
CRGB::Yellow
CRGB::Cyan
CRGB::Magenta
CRGB::Orange
CRGB::Purple
CRGB::Pink
CRGB::Lime
CRGB::Navy
CRGB::Teal
CRGB::Violet

// Color RGB personalizado:
CRGB(255, 0, 0)     // Rojo
CRGB(0, 255, 0)     // Verde
CRGB(0, 0, 255)     // Azul
CRGB(255, 128, 0)   // Naranja

// Color HSV (Hue, Saturation, Value):
CHSV(0, 255, 255)     // Rojo
CHSV(85, 255, 255)    // Verde
CHSV(170, 255, 255)   // Azul
```

---

## ✅ Checklist de Verificación

Después de correr el test, verifica:

- [ ] ✅ Los 4 LEDs encienden
- [ ] ✅ Colores son correctos (rojo es rojo, verde es verde)
- [ ] ✅ Cada LED responde individualmente
- [ ] ✅ Brillo es uniforme entre LEDs
- [ ] ✅ No hay parpadeos o glitches
- [ ] ✅ Rainbow es fluido
- [ ] ✅ Flash y fade funcionan bien

Si todo está ✅, **¡tus NeoPixels funcionan perfectamente!**

---

## 🔄 Volver al Código Principal

Cuando termines el test:

1. **Opción 1 - Editar platformio.ini:**
   ```ini
   default_envs = main_brain
   ```

2. **Opción 2 - Desde terminal:**
   ```bash
   ~/.platformio/penv/bin/pio run --environment main_brain -t upload
   ```

---

## 💡 Tips

1. **Tomar foto/video del test** - Te ayudará si hay problemas después
2. **Probar con diferentes brillos** - Verifica consumo de corriente
3. **Medir voltaje** - Con multímetro en VDD mientras funcionan
4. **Test de duración** - Dejar corriendo 30 minutos para verificar estabilidad

---

## 📸 Resultado Esperado

**Test Individual (Test 5):**
```
LED 0 ●⚫⚫⚫  → Rojo encendido 1s, luego apaga
LED 1 ⚫●⚫⚫  → Rojo encendido 1s, luego apaga
LED 2 ⚫⚫●⚫  → Rojo encendido 1s, luego apaga
LED 3 ⚫⚫⚫●  → Rojo encendido 1s, luego apaga
```

**Test Colores (Test 9):**
```
LED 0 🔴⚫⚫⚫  → Rojo (Kick)
LED 1 ⚫🟢⚫⚫  → Verde (Snare)
LED 2 ⚫⚫🔵⚫  → Cyan (HiHat)
LED 3 ⚫⚫⚫🔵  → Azul (Tom)
```

---

¡Listo para probar! 🚀

Si algo no funciona, comparte la salida del monitor serial y te ayudo a diagnosticar.
