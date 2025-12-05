# 🥁 Sistema de Detección de Triggers - Explicación Detallada

## 📋 Tabla de Contenidos
1. [Flujo Completo de Señal](#flujo-completo-de-señal)
2. [Proceso Detallado por Etapa](#proceso-detallado-por-etapa)
3. [Estrategia de Configuración GUI](#estrategia-de-configuración-gui)
4. [Parámetros Configurables por Pad](#parámetros-configurables-por-pad)
5. [Protocolo de Comunicación UART](#protocolo-de-comunicación-uart)
6. [Arquitectura del Sistema](#arquitectura-del-sistema)

---

## 🔄 Flujo Completo de Señal

```
┌─────────────┐
│  Golpe      │
│  Físico     │
└──────┬──────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│  ETAPA 1: SENSOR PIEZO                                      │
│  • Conversión mecánica → eléctrica                          │
│  • Genera pico de voltaje (0-2.45V)                         │
│  • Duración: ~0.5-3ms                                       │
└──────┬──────────────────────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│  ETAPA 2: ADC ESP32-S3                                      │
│  • Conversión analógica → digital                           │
│  • Resolución: 12 bits (0-4095)                             │
│  • Atenuación: 11dB (rango 0-2.45V)                         │
│  • Tiempo conversión: ~10-20µs                              │
└──────┬──────────────────────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│  ETAPA 3: LECTURA PERIÓDICA (trigger_scanner.cpp)          │
│  • FreeRTOS Task en Core 0, prioridad 24                   │
│  • Frecuencia: 2kHz (cada 500µs)                           │
│  • Lee 4 pads secuencialmente (~80µs total)                │
│  • Envía valor raw al detector                             │
└──────┬──────────────────────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│  ETAPA 4: DETECCIÓN DE PICO (trigger_detector.cpp)         │
│                                                             │
│  ┌───────────┐                                             │
│  │ STATE_IDLE│◄────────────┐                               │
│  └─────┬─────┘             │                               │
│        │ signal > threshold │                               │
│        ▼                    │                               │
│  ┌────────────┐            │                               │
│  │STATE_RISING│            │                               │
│  └─────┬──────┘            │                               │
│        │ peak detectado    │                               │
│        ▼                   │                               │
│  ┌──────────────────┐     │                               │
│  │STATE_PEAK_DETECTED│     │                               │
│  └─────┬─────────────┘     │                               │
│        │ velocity calculada│                               │
│        ▼                   │                               │
│  ┌───────────┐             │                               │
│  │STATE_DECAY│─────────────┘                               │
│  └───────────┘   signal < threshold                        │
│                                                             │
│  • Baseline tracking (ruido adaptativo)                    │
│  • Peak window: 2ms                                        │
│  • Crosstalk rejection: 50ms window                        │
│  • Velocity curve: sqrt(x) para feel natural               │
└──────┬──────────────────────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│  ETAPA 5: EVENTO DE HIT                                     │
│  • Generado cuando peak detectado                           │
│  • Contiene: padId, velocity (0-127), timestamp             │
│  • Enviado por FreeRTOS Queue                              │
└──────┬──────────────────────────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────────────────────────┐
│  ETAPA 6: PROCESAMIENTO (main.cpp loop)                    │
│  • Recibe evento de queue                                   │
│  • Acciones paralelas:                                      │
│    ├─ Enviar MIDI (USB/UART)                               │
│    ├─ Reproducir sample (I2S audio)                        │
│    ├─ Activar LED con color del pad                        │
│    └─ Enviar telemetría a GUI ESP                          │
└─────────────────────────────────────────────────────────────┘
```

**⏱️ LATENCIA TOTAL: ~1.5-2.5ms** (excelente para e-drums profesionales)

---

## 🔍 Proceso Detallado por Etapa

### **ETAPA 1: Sensor Piezo → Voltaje**

El piezo es un cristal piezoeléctrico que genera carga cuando se deforma:

```
Presión mecánica → Deformación cristal → Carga eléctrica → Voltaje
```

**Características de la señal:**
- **Ataque**: 0.2-0.5ms (subida rápida al pico)
- **Pico**: Ocurre en ~0.5-1.5ms después del golpe
- **Decay**: 2-10ms (caída exponencial)
- **Amplitud**: 0-2.45V (limitado por atenuación ADC)

**Ejemplo de forma de onda:**
```
Voltaje (V)
   2.5│        ╱╲
      │       ╱  ╲
   2.0│      ╱    ╲___
      │     ╱         ╲___
   1.5│    ╱              ╲___
      │   ╱                   ╲___
   1.0│  ╱                        ╲___
      │ ╱                             ╲___
   0.5│╱                                  ╲___
      └┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴─► Tiempo (ms)
      0  1  2  3  4  5  6  7  8  9  10 11 12
      │←ataque│←pico│←─────── decay ─────────→│
```

---

### **ETAPA 2: ADC Conversión**

El ESP32-S3 tiene un ADC de 12 bits con atenuación configurable:

| Atenuación | Rango Voltaje | Uso |
|------------|---------------|-----|
| 0dB        | 0-1.1V        | Señales débiles |
| 2.5dB      | 0-1.5V        | - |
| 6dB        | 0-2.2V        | - |
| **11dB**   | **0-2.45V**   | **✓ Nuestro caso** |

**Conversión ADC → Voltaje:**
```cpp
voltaje = (raw * 2.45) / 4095.0;
// Ejemplo: raw=2000 → 2000*2.45/4095 = 1.196V
```

**Tiempo de conversión:** ~10-20µs por lectura (incluye estabilización)

---

### **ETAPA 3: Lectura Periódica (Scanner)**

Tarea FreeRTOS que lee todos los pads en bucle:

```cpp
void scanPiezosTask(void* parameter) {
    const TickType_t scanPeriodTicks = pdMS_TO_TICKS(1);  // ~500µs
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true) {
        // Lee los 4 pads secuencialmente
        for (uint8_t i = 0; i < NUM_PADS; i++) {
            uint16_t raw = analogRead(PAD_ADC_PINS[i]);  // ~20µs
            TriggerDetector::process(i, raw);            // ~10µs
        }

        // Total: ~120µs → Quedan 380µs libres en el ciclo
        vTaskDelayUntil(&lastWakeTime, scanPeriodTicks);
    }
}
```

**Prioridad 24**: Garantiza ejecución puntual incluso con otras tareas activas

**Frecuencia 2kHz**: Suficiente para capturar picos de 0.5ms (Nyquist: 4 samples/pico)

---

### **ETAPA 4: Detección de Pico (State Machine)**

#### **Estado 1: IDLE (Esperando golpe)**

```cpp
// Tracking de baseline (ruido adaptativo)
baseline = (baseline * 1023 + raw) >> 10;  // EMA con alpha=1/1024

// Señal = raw - ruido
signal = (raw > baseline) ? (raw - baseline) : 0;

// Detectar inicio de golpe
if (signal > THRESHOLD_PER_PAD[padId]) {
    state = STATE_RISING;
    peakValue = signal;
    windowStartTime = micros();
}
```

**¿Por qué baseline tracking?**
- El ruido varía con temperatura, vibración, humedad
- Baseline se adapta lentamente (~segundos) al ruido nuevo
- Golpe real destaca sobre baseline

**Ejemplo:**
```
Si baseline=150 y threshold=250:
  raw=180 → signal=30  → NO TRIGGER (noise)
  raw=450 → signal=300 → TRIGGER! (hit)
```

#### **Estado 2: RISING (Buscando pico máximo)**

```cpp
// Actualizar máximo mientras sube
if (signal > peakValue) {
    peakValue = signal;
}

// OPTIMIZACIÓN: Terminación temprana
// Si cae 30%, el pico ya pasó
if (signal < peakValue * 0.7f) {
    state = STATE_PEAK_DETECTED;
    // ¡Ahorra ~0.5-1ms vs esperar ventana completa!
}

// Timeout: Ventana de 2ms
if (micros() - windowStartTime > 2000) {
    state = STATE_PEAK_DETECTED;
}
```

**¿Por qué 2ms?**
- Pico típico ocurre en 0.5-1.5ms
- 2ms captura el 99% de casos
- Más tiempo = más latencia innecesaria

#### **Estado 3: PEAK_DETECTED (Calculando velocity)**

```cpp
// 1. CROSSTALK REJECTION
for (otros pads) {
    uint32_t timeSince = currentTime - pad.lastHitTime;

    if (timeSince < 50000 &&  // Dentro de ventana 50ms
        pad.lastVelocity > currentVelocity * 0.7f) {  // 70% ratio

        // RECHAZAR! Es vibración simpática
        state = STATE_DECAY;
        return;
    }
}

// 2. MAPEO ADC → VELOCITY MIDI
float normalized = (peakValue - velocityMin) / (velocityMax - velocityMin);
normalized = constrain(normalized, 0.0f, 1.0f);

// 3. CURVA DE VELOCIDAD (sqrt para feel natural)
float curved = pow(normalized, velocityCurve);  // típicamente 0.5

// 4. ESCALAR A MIDI (0-127)
uint8_t velocity = (uint8_t)(curved * 127.0f);
velocity = constrain(velocity, 1, 127);

// 5. GENERAR EVENTO
HitEvent event = {padId, velocity, timestamp, peakValue};
xQueueSend(hitQueue, &event, 0);

state = STATE_DECAY;
```

**¿Por qué curva sqrt(x)?**

Sin curva (lineal):
```
Golpe suave (20% fuerza) → 25 velocity  (difícil controlar)
Golpe medio (50% fuerza) → 64 velocity
Golpe fuerte (80% fuerza) → 102 velocity (poca diferencia)
```

Con curva sqrt(x):
```
Golpe suave (20% fuerza) → 57 velocity  (✓ Más expresivo)
Golpe medio (50% fuerza) → 90 velocity
Golpe fuerte (80% fuerza) → 114 velocity (✓ Mejor rango)
```

#### **Estado 4: DECAY (Anti-rebote)**

```cpp
// Esperar a que señal baje suficiente
if (signal < threshold * 0.8f) {  // 20% debajo
    state = STATE_IDLE;  // Listo para próximo hit
    lastHitTime = micros();
}

// Timeout de seguridad
if (micros() - peakTime > MAX_DECAY_MS * 1000) {
    state = STATE_IDLE;
}
```

**Previene doble trigger** por rebote mecánico del pad

---

### **ETAPA 5: Evento de Hit**

Estructura enviada por FreeRTOS Queue:

```cpp
struct HitEvent {
    uint8_t padId;        // 0-3 (Kick, Snare, HiHat, Tom)
    uint8_t velocity;     // 0-127 (MIDI velocity)
    uint32_t timestamp;   // Microsegundos desde boot
    uint16_t peakValue;   // ADC raw (para debugging)
};
```

**FreeRTOS Queue**:
- Thread-safe entre Core 0 (scanner) y Core 1 (main loop)
- Capacidad: 32 eventos
- Sin bloqueo (si lleno, descarta evento más viejo)

---

### **ETAPA 6: Procesamiento del Evento**

En el `loop()` principal (Core 1):

```cpp
void loop() {
    HitEvent event;

    if (xQueueReceive(hitQueue, &event, 0) == pdTRUE) {
        // 1. Enviar MIDI (si conectado por USB/UART)
        sendMIDI(event.padId, event.velocity);

        // 2. Reproducir sample por I2S
        playSample(config.sampleName, event.velocity);

        // 3. Activar LED
        setLEDColor(event.padId, config.ledColorHit);
        fadeLED(config.ledFadeDuration);

        // 4. Telemetría a GUI
        UARTProtocol::sendHitEvent(event.padId, event.velocity,
                                    event.timestamp, event.peakValue);

        // 5. Log serial (debug)
        Serial.printf("PAD: %s | VEL: %d | PEAK: %d\n",
                      PAD_NAMES[event.padId],
                      event.velocity,
                      event.peakValue);
    }

    // Procesar comandos de GUI
    UARTProtocol::processIncoming();

    // Actualizar LEDs, encoders, etc.
    updateUI();
}
```

---

## 🎛️ Estrategia de Configuración GUI

### **Arquitectura de Comunicación**

```
┌──────────────────┐                    ┌──────────────────┐
│   MAIN BRAIN     │                    │    GUI ESP32     │
│   (ESP32-S3)     │◄──────UART────────►│   (ESP32 / S3)   │
│                  │     921600 baud    │                  │
│  • Trigger scan  │                    │  • Display TFT   │
│  • Audio I2S     │                    │  • Touch input   │
│  • Config NVS    │                    │  • Encoders      │
│  • LED control   │                    │  • Buttons       │
└──────────────────┘                    └──────────────────┘
```

### **Protocolo Binario con JSON**

**¿Por qué binario + JSON híbrido?**
- Binario para eventos de alta frecuencia (hits, pad states) → Eficiente
- JSON para configuración y dumps completos → Flexible

**Estructura de mensaje:**
```
[START_BYTE][MSG_TYPE][LENGTH_MSB][LENGTH_LSB][PAYLOAD...][CRC16_MSB][CRC16_LSB]
     1B         1B         1B          1B        0-512B         1B         1B
```

**Ejemplo - Enviar hit event:**
```
AA 01 00 09  [padId=02 vel=7F timestamp=12345678 peak=08A0]  3F 2E
│  │  │  │    └──────────── 9 bytes payload ────────────┘   └CRC┘
│  │  └──┴─ Length = 9
│  └─ MSG_HIT_EVENT
└─ Start byte (0xAA)
```

**Ejemplo - Actualizar threshold (GUI → Main):**
```
AA 20 00 03  [padId=01 threshold=0190]  8B 4A
│  │  │  │    └──── 3 bytes ─────┘     └CRC┘
│  │  └──┴─ Length = 3
│  └─ CMD_SET_THRESHOLD
└─ Start byte
```

---

## 📝 Parámetros Configurables por Pad

### **Categoría 1: Detección de Trigger**

| Parámetro | Tipo | Rango | Descripción | Uso GUI |
|-----------|------|-------|-------------|---------|
| `threshold` | uint16_t | 50-2000 | Nivel ADC para detectar golpe | Slider "Sensibilidad" (invertido: 100%=50, 0%=2000) |
| `velocityMin` | uint16_t | 50-1000 | ADC mínimo para velocity=1 | Calibración automática |
| `velocityMax` | uint16_t | 500-4000 | ADC máximo para velocity=127 | Calibración automática |
| `velocityCurve` | float | 0.3-2.0 | Exponente de curva (<1=soft, >1=hard) | Preset: "Linear/Soft/Medium/Hard" |

**Ejemplo GUI - Pantalla "Sensibilidad":**
```
┌────────────────────────────────────┐
│  PAD: SNARE                        │
├────────────────────────────────────┤
│                                    │
│  Sensibilidad:  ████████░░ 80%    │
│  (threshold = 150)                 │
│                                    │
│  Curva:  ○ Linear  ● Soft         │
│          ○ Medium  ○ Hard         │
│                                    │
│  [Calibrar Automático]             │
│                                    │
└────────────────────────────────────┘
```

---

### **Categoría 2: Crosstalk Rejection**

| Parámetro | Tipo | Rango | Descripción | Uso GUI |
|-----------|------|-------|-------------|---------|
| `crosstalkEnabled` | bool | - | Activar/desactivar | Toggle switch |
| `crosstalkWindow` | uint16_t | 10-200ms | Ventana de tiempo para comparar | Slider avanzado |
| `crosstalkRatio` | float | 0.3-0.95 | Ratio de velocity para rechazar | Slider avanzado |
| `crosstalkMask` | uint8_t | bitmask | Qué pads verificar | Matriz de checkboxes |

**Ejemplo GUI - Pantalla "Crosstalk":**
```
┌────────────────────────────────────┐
│  PAD: SNARE                        │
├────────────────────────────────────┤
│                                    │
│  Crosstalk:  [●] Activado         │
│                                    │
│  Verificar contra:                 │
│    [✓] Kick   [ ] Snare           │
│    [✓] HiHat  [✓] Tom             │
│                                    │
│  Ventana: 50ms  ████░░             │
│  Ratio:   70%   ███████░           │
│                                    │
└────────────────────────────────────┘
```

---

### **Categoría 3: Timing**

| Parámetro | Tipo | Rango | Descripción | Uso GUI |
|-----------|------|-------|-------------|---------|
| `peakWindowMs` | uint16_t | 1-5ms | Tiempo máximo para buscar pico | Avanzado (raramente cambiar) |
| `decayTimeMs` | uint16_t | 10-100ms | Timeout de decay | Avanzado |
| `minRetriggerMs` | uint8_t | 5-50ms | Tiempo mínimo entre hits | "Retrigger Speed" |

---

### **Categoría 4: Audio/MIDI**

| Parámetro | Tipo | Rango | Descripción | Uso GUI |
|-----------|------|-------|-------------|---------|
| `midiNote` | uint8_t | 0-127 | Nota MIDI | Picker: "C1/D1/E1..." o teclado virtual |
| `midiChannel` | uint8_t | 1-16 | Canal MIDI | Dropdown |
| `sampleName` | char[32] | - | Nombre archivo WAV | Browser de samples con preview |
| `sampleVolume` | uint8_t | 0-100 | Volumen % | Slider |
| `samplePitch` | int8_t | -12 a +12 | Pitch shift en semitonos | Slider |

**Ejemplo GUI - Pantalla "Sonido":**
```
┌────────────────────────────────────┐
│  PAD: KICK                         │
├────────────────────────────────────┤
│                                    │
│  Sample:  [kick_808_deep.wav  ▼]  │
│           [▶ Preview]              │
│                                    │
│  Volumen:  ██████████ 100%        │
│  Pitch:    0 semitones (original)  │
│                                    │
│  MIDI:  Note: C1 (36)              │
│         Channel: 10 (Drums)        │
│                                    │
│  [📁 Browse Samples]               │
│                                    │
└────────────────────────────────────┘
```

---

### **Categoría 5: Visual (LEDs)**

| Parámetro | Tipo | Rango | Descripción | Uso GUI |
|-----------|------|-------|-------------|---------|
| `ledColorHit` | uint32_t | RGB | Color al golpear | Color picker |
| `ledColorIdle` | uint32_t | RGB | Color en reposo | Color picker |
| `ledBrightness` | uint8_t | 0-100 | Brillo % | Slider |
| `ledFadeDuration` | uint16_t | 50-1000ms | Tiempo de fade | Slider "Fade Speed" |

**Ejemplo GUI - Pantalla "LED":**
```
┌────────────────────────────────────┐
│  PAD: HIHAT                        │
├────────────────────────────────────┤
│                                    │
│  Color Hit:   [🔵] ← Cyan          │
│  Color Idle:  [⚫] ← Dim Cyan      │
│                                    │
│  Brillo:  ████████░░ 80%          │
│  Fade:    ███████░░░ 150ms        │
│                                    │
│  [Test LED]                        │
│                                    │
└────────────────────────────────────┘
```

---

### **Categoría 6: Dual-Zone (Avanzado)**

| Parámetro | Tipo | Rango | Descripción | Uso GUI |
|-----------|------|-------|-------------|---------|
| `dualZoneEnabled` | bool | - | Activar detección rim/edge | Toggle |
| `rimThreshold` | uint16_t | 100-1000 | Threshold para zona rim | Slider |
| `rimMidiNote` | uint8_t | 0-127 | Nota MIDI para rim | Picker |
| `rimSampleName` | char[32] | - | Sample del rim | Browser |

**Ejemplo - Snare con Rimshot:**
```
Golpe centro (head) → MIDI 38 → "snare_center.wav"
Golpe borde  (rim)  → MIDI 40 → "snare_rimshot.wav"
```

---

### **Categoría 7: Metadata**

| Parámetro | Tipo | Descripción |
|-----------|------|-------------|
| `name` | char[16] | Nombre personalizado del pad |
| `padType` | uint8_t | 0=Kick, 1=Snare, 2=Tom, 3=Cymbal, 4=HiHat |
| `enabled` | bool | Habilitar/deshabilitar pad completamente |

---

## 📡 Protocolo de Comunicación UART

### **Mensajes del Main Brain → GUI**

| ID | Tipo | Frecuencia | Payload | Uso |
|----|------|------------|---------|-----|
| 0x01 | `MSG_HIT_EVENT` | Por golpe | padId, velocity, timestamp, peak | Animaciones, meter |
| 0x02 | `MSG_PAD_STATE` | 10Hz (debug) | padId, state, signal, baseline | Oscilloscopio |
| 0x03 | `MSG_SYSTEM_STATUS` | 1Hz | CPU, RAM, temp, uptime | Info sistema |
| 0x04 | `MSG_CONFIG_UPDATE` | Al cambiar | padId + JSON | Sincronizar GUI |
| 0x05 | `MSG_CALIBRATION_DATA` | Durante calibración | baseline, noise, suggested | Asistente calibración |

### **Comandos de GUI → Main Brain**

| ID | Tipo | Payload | Respuesta |
|----|------|---------|-----------|
| 0x20 | `CMD_SET_THRESHOLD` | padId + threshold | ACK + MSG_CONFIG_UPDATE |
| 0x21 | `CMD_SET_VELOCITY_RANGE` | padId + min + max | ACK |
| 0x22 | `CMD_SET_VELOCITY_CURVE` | padId + curve | ACK |
| 0x23 | `CMD_SET_MIDI_NOTE` | padId + note + channel | ACK |
| 0x24 | `CMD_SET_SAMPLE` | padId + filename | ACK |
| 0x25 | `CMD_SET_LED_COLOR` | padId + colors + brightness | ACK |
| 0x27 | `CMD_SET_FULL_CONFIG` | JSON completo | ACK + MSG_CONFIG_DUMP |
| 0x30 | `CMD_GET_CONFIG` | - | MSG_CONFIG_DUMP |
| 0x31 | `CMD_SAVE_CONFIG` | - | ACK (guardado en NVS) |
| 0x40 | `CMD_START_CALIBRATION` | - | Stream de CALIBRATION_DATA |

### **Ejemplo de Flujo - Usuario ajusta threshold:**

```
1. Usuario mueve slider "Sensibilidad" al 75% en GUI
   ↓
2. GUI calcula: threshold = map(75, 0-100, 2000-50) = 550
   ↓
3. GUI envía: CMD_SET_THRESHOLD (0x20)
   Payload: [padId=01, threshold=0226] (550 en hex)
   ↓
4. Main Brain procesa comando:
   - Actualiza config.threshold = 550
   - Responde: MSG_ACK
   - Envía: MSG_CONFIG_UPDATE con JSON del pad
   ↓
5. GUI recibe confirmación y actualiza UI
```

### **Ejemplo de Flujo - Calibración Automática:**

```
1. Usuario presiona [Calibrar] en GUI
   ↓
2. GUI envía: CMD_START_CALIBRATION
   ↓
3. Main Brain entra en modo calibración (30 segundos):
   - "Golpea el pad lo más suave que puedas"
   - Observa baseline y noise por 10s
   ↓
4. Main Brain envía cada 1s:
   MSG_CALIBRATION_DATA {
     padId = 1,
     baseline = 150,
     noiseFloor = 30,
     suggestedThreshold = baseline + 80 = 230
   }
   ↓
5. GUI muestra en tiempo real:
   "Baseline: 150 | Ruido: ±30 | Sugerido: 230"
   ↓
6. Después de 10s:
   - "Ahora golpea lo más fuerte que puedas"
   - Detecta velocityMax
   ↓
7. Al terminar (30s):
   - Main Brain envía valores sugeridos
   - GUI pregunta: "¿Aplicar configuración?"
   - Si Sí: GUI envía CMD_SET_FULL_CONFIG
   - Main Brain aplica y guarda en NVS
```

---

## 🏗️ Arquitectura del Sistema

### **Distribución de Tareas por Core**

```
┌─────────────── ESP32-S3 ───────────────┐
│                                        │
│  CORE 0 (Real-Time)                    │
│  ┌──────────────────────────────────┐  │
│  │ Task: scanPiezosTask             │  │
│  │ Prioridad: 24 (alta)             │  │
│  │ Frecuencia: 2kHz                 │  │
│  │                                  │  │
│  │ • Lee 4 ADCs (~80µs)             │  │
│  │ • Procesa state machine (~30µs)  │  │
│  │ • Envía eventos por Queue        │  │
│  │                                  │  │
│  │ Carga: ~25% (175µs ocupados      │  │
│  │              de 500µs ciclo)     │  │
│  └──────────────────────────────────┘  │
│                                        │
│  ═════════════════════════════════════ │
│                                        │
│  CORE 1 (Application)                  │
│  ┌──────────────────────────────────┐  │
│  │ loop()                           │  │
│  │                                  │  │
│  │ • Recibe hit events              │  │
│  │ • Reproduce audio I2S (DMA)      │  │
│  │ • Actualiza LEDs (NeoPixel)      │  │
│  │ • Procesa UART GUI               │  │
│  │ • Lee encoders y botones         │  │
│  │ • Accede SD card                 │  │
│  │                                  │  │
│  │ Carga: ~40-60% (con audio)       │  │
│  └──────────────────────────────────┘  │
│                                        │
└────────────────────────────────────────┘
```

### **Módulos del Sistema**

```
groove_drum/
├── shared/
│   └── config/
│       ├── edrum_config.h          # Constantes globales (pines, etc)
│       └── pad_config.h            # ✨ NUEVO: Estructura de config por pad
│
├── src/main_brain/
│   ├── main.cpp                    # Loop principal + setup
│   │
│   ├── input/
│   │   ├── trigger_detector.h/cpp  # State machine de detección
│   │   └── trigger_scanner.h/cpp   # FreeRTOS task de lectura ADC
│   │
│   ├── config/
│   │   └── pad_config_manager.cpp  # ✨ NUEVO: Gestión de configs (NVS, JSON)
│   │
│   ├── communication/
│   │   ├── uart_protocol.h         # ✨ NUEVO: Definición de protocolo
│   │   └── uart_protocol.cpp       # ✨ NUEVO: Implementación UART
│   │
│   ├── audio/
│   │   └── sample_player.h/cpp     # (TODO) Reproductor WAV por I2S
│   │
│   └── ui/
│       ├── led_controller.h/cpp    # (TODO) NeoPixel + SK9822
│       └── encoder_handler.h/cpp   # (TODO) Encoders físicos
│
└── platformio.ini
```

---

## 🔧 Integración con Main.cpp

Para usar el nuevo sistema, en `main.cpp`:

```cpp
#include "pad_config.h"
#include "uart_protocol.h"

void setup() {
    Serial.begin(115200);

    // 1. Inicializar configuraciones
    PadConfigManager::init();  // Carga desde NVS o usa defaults

    // 2. Inicializar UART con GUI
    UARTProtocol::begin(Serial2, 921600);  // Serial2 = UART a GUI ESP

    // 3. Inicializar scanner con configs actuales
    TriggerScanner::begin();

    // 4. Enviar config inicial a GUI
    UARTProtocol::sendConfigDump();

    Serial.println("[MAIN] Sistema iniciado");
}

void loop() {
    HitEvent event;

    // Procesar hits
    if (xQueueReceive(hitQueue, &event, 0) == pdTRUE) {
        // Obtener config del pad
        PadConfig& cfg = PadConfigManager::getConfig(event.padId);

        // 1. Audio
        playSample(cfg.sampleName, event.velocity, cfg.sampleVolume);

        // 2. LED
        setLED(event.padId, cfg.ledColorHit, cfg.ledBrightness);

        // 3. Telemetría a GUI
        UARTProtocol::sendHitEvent(event.padId, event.velocity,
                                    event.timestamp, event.peakValue);

        // 4. MIDI (si conectado)
        sendMIDI(cfg.midiChannel, cfg.midiNote, event.velocity);
    }

    // Procesar comandos de GUI
    UARTProtocol::processIncoming();
}
```

---

## 📊 Ventajas del Sistema Propuesto

### **1. Configuración Dinámica**
- ✓ Cambios sin recompilar firmware
- ✓ Ajustes en tiempo real desde GUI
- ✓ Persistencia en NVS (sobrevive reinicios)

### **2. Escalabilidad**
- ✓ Fácil agregar nuevos pads (hasta 8)
- ✓ Nuevos parámetros sin cambiar protocolo (JSON flexible)
- ✓ Firmware update vía OTA posible

### **3. Debugging**
- ✓ Modo calibración con feedback visual
- ✓ Oscilloscopio en tiempo real (MSG_PAD_STATE)
- ✓ Logs detallados por UART

### **4. Experiencia Usuario**
- ✓ GUI táctil intuitiva (vs menú LED + encoder)
- ✓ Previsualización de samples
- ✓ Backup/restore de configs en SD

---

## 🎯 Próximos Pasos

1. **Integrar archivos nuevos en platformio.ini**
   ```ini
   src_filter =
       +<main_brain/main.cpp>
       +<main_brain/input/>
       +<main_brain/config/>
       +<main_brain/communication/>
   ```

2. **Modificar trigger_detector.cpp** para usar `PadConfigManager::getConfig(padId)` en vez de arrays estáticos

3. **Implementar audio player** (sample_player.cpp) con I2S DMA

4. **Desarrollar GUI ESP32** con pantalla TFT y protocolo UART

5. **Testing real** con piezos físicos y ajustes fino de parámetros

---

## 📚 Referencias

- ESP32 ADC: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html
- FreeRTOS Tasks: https://www.freertos.org/taskandcr.html
- Roland TD-27 specs: 3ms latency, 127 velocity levels
- Alesis Strike specs: 2ms latency, crosstalk rejection

---

**¿Preguntas?**

- ¿Cómo funciona el baseline tracking? → Ver ETAPA 4, Estado 1
- ¿Por qué curva sqrt(x)? → Ver ETAPA 4, Estado 3
- ¿Cómo evitar doble trigger? → Ver ETAPA 4, Estado 4
- ¿Qué es crosstalk? → Ver ETAPA 4, Estado 3
- ¿Cómo calibrar threshold? → Ver Protocolo UART, Calibración Automática
