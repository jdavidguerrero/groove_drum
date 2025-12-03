# FreeRTOS en E-Drum Controller - Tutorial Completo

## 📚 Índice

1. [¿Qué es FreeRTOS?](#qué-es-freertos)
2. [¿Por qué necesitamos FreeRTOS?](#por-qué-necesitamos-freertos)
3. [Conceptos Fundamentales](#conceptos-fundamentales)
4. [Implementación en Nuestro Código](#implementación-en-nuestro-código)
5. [Análisis Línea por Línea](#análisis-línea-por-línea)
6. [Flujo Temporal](#flujo-temporal)
7. [Debugging y Troubleshooting](#debugging-y-troubleshooting)

---

## ¿Qué es FreeRTOS?

**FreeRTOS** = **Free Real-Time Operating System**

Es un sistema operativo miniatura (kernel) diseñado para microcontroladores que permite:

### Sin FreeRTOS (Arduino clásico)
```cpp
void setup() {
    // Inicialización
}

void loop() {
    // TODO SE EJECUTA AQUÍ EN SECUENCIA
    leerSensores();    // 500µs
    actualizarLEDs();  // 2000µs ← ¡BLOQUEA TODO!
    enviarMIDI();      // 100µs
    // Total: 2600µs por ciclo
}
```

**Problemas**:
- Todo es **secuencial** (una cosa después de otra)
- Si una función **se bloquea**, todo se detiene
- No hay **prioridades** (todo es igual de importante)
- Difícil garantizar **timing preciso**

### Con FreeRTOS
```cpp
void setup() {
    // Crear múltiples "tareas" independientes
    xTaskCreate(leerSensoresTask, ...);  // Prioridad ALTA
    xTaskCreate(actualizarLEDsTask, ...); // Prioridad BAJA
    xTaskCreate(enviarMIDITask, ...);     // Prioridad MEDIA
}

void loop() {
    // Vacío - FreeRTOS maneja todo
}

// Cada tarea corre "simultáneamente"
void leerSensoresTask() {
    while(1) {
        leerSensores();
        vTaskDelay(1); // Cede CPU a otras tareas
    }
}
```

**Ventajas**:
- Tareas **paralelas** (multitasking)
- Sistema de **prioridades**
- **Timing preciso** con delays no bloqueantes
- **Comunicación segura** entre tareas (queues, semáforos)

---

## ¿Por Qué Necesitamos FreeRTOS?

### Problema Real en Nuestro E-Drum

Imagina este escenario **SIN** FreeRTOS:

```cpp
void loop() {
    // 1. Leer piezos
    for (int i = 0; i < 4; i++) {
        int val = analogRead(piezo[i]);  // 40µs × 4 = 160µs
        procesarTrigger(val);
    }

    // 2. Actualizar LEDs
    FastLED.show();  // ¡2000µs! (2 milisegundos)

    // 3. Enviar MIDI
    if (hayEventos) {
        Serial1.write(...);  // 100µs
    }

    // Ciclo total: ~2260µs (442 Hz)
}
```

**¿Qué pasa si golpeas un pad DURANTE `FastLED.show()`?**

```
Tiempo (µs)
0      500    1000   1500   2000   2500
│       │       │       │       │       │
├─ADC──┤       │       │       │       │
        ├───────FastLED.show()─────────┤  ← ¡BLOQUEADO!
                │       │       │       │
                🥁 ← Golpe aquí se PIERDE
                     (ADC no lee durante 1500µs)
```

**Resultado**: Triggers perdidos, latencia variable (0-2000µs).

### Solución: FreeRTOS con Dual-Core

```
ESP32-S3 tiene 2 CPUs físicas (cores)

CORE 0                          CORE 1
┌─────────────────┐            ┌─────────────────┐
│ triggerScanTask │            │ ledAnimationTask│
│ Prioridad: 24   │            │ Prioridad: 5    │
│ (Máxima)        │            │ (Baja)          │
│                 │            │                 │
│ while(1) {      │            │ while(1) {      │
│   readADC();    │◄───────────┼─ Queue ────┐   │
│   detect();     │            │            │   │
│   sendQueue()───┼────────────┼───────────►│   │
│   delay(500µs); │            │   show();  │   │
│ }               │            │   delay(16ms);  │
└─────────────────┘            └─────────────────┘
     NUNCA SE                       PUEDE
     BLOQUEA                      BLOQUEARSE
```

**Resultado**:
- Core 0 **SIEMPRE** escanea cada 500µs (garantizado)
- Core 1 puede bloquearse en `FastLED.show()` **sin afectar** triggers
- Latencia constante: <2ms

---

## Conceptos Fundamentales

### 1. Task (Tarea)

Una **task** es como un "programa" independiente que corre en paralelo con otros.

```cpp
void miTarea(void* parametro) {
    // Inicialización de la tarea
    int contador = 0;

    while (1) {  // ¡Loop infinito!
        // Hacer algo
        Serial.printf("Contador: %d\n", contador++);

        // Esperar (ceder CPU a otras tareas)
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 segundo
    }

    // NUNCA llegar aquí (la tarea NUNCA termina)
}
```

**Características**:
- Cada task tiene su propio **stack** (memoria para variables locales)
- Cada task tiene una **prioridad** (0-24, mayor número = más importante)
- El **scheduler** decide qué task ejecutar en cada momento

### 2. Scheduler (Planificador)

El scheduler es el "cerebro" de FreeRTOS que decide:
- ¿Qué task ejecutar?
- ¿Por cuánto tiempo?
- ¿Cuándo cambiar a otra task?

**Reglas básicas**:
1. La task con **mayor prioridad lista** corre primero
2. Si dos tasks tienen la **misma prioridad**, se turnan (round-robin)
3. Una task puede **ceder** la CPU voluntariamente (`vTaskDelay`)
4. Una task **bloqueada** (esperando queue, semáforo) no consume CPU

### 3. Queue (Cola)

Una **queue** es un buffer FIFO (First In, First Out) que permite comunicación **thread-safe** entre tasks.

```cpp
// Crear queue
QueueHandle_t miQueue = xQueueCreate(
    10,              // 10 slots
    sizeof(int)      // Cada slot almacena un int
);

// Task 1: Enviar datos
void taskProductor(void* param) {
    while(1) {
        int dato = leerSensor();
        xQueueSend(miQueue, &dato, 0);  // Enviar sin bloquear
        vTaskDelay(100);
    }
}

// Task 2: Recibir datos
void taskConsumidor(void* param) {
    int dato;
    while(1) {
        if (xQueueReceive(miQueue, &dato, portMAX_DELAY)) {
            // Procesar dato
            Serial.printf("Recibido: %d\n", dato);
        }
    }
}
```

**Ventajas**:
- **Thread-safe**: No hay race conditions
- **Bloqueo inteligente**: Task consumidora duerme si no hay datos
- **Desacoplamiento**: Productor y consumidor no se conocen

### 4. Delay vs vTaskDelay

```cpp
// ❌ Arduino delay() - BLOQUEA TODO
delay(1000);  // CPU ocupada haciendo nada por 1 segundo

// ✅ FreeRTOS vTaskDelay() - CEDE CPU
vTaskDelay(pdMS_TO_TICKS(1000));
//         ^^^^^^^^^^^^^^^^^^^
//         Convierte ms a "ticks" del sistema
```

**¿Qué pasa internamente?**

```
Con delay(1000):
CPU: [Busy-Wait────────────────────────] ← Desperdicia energía
     0ms                            1000ms

Con vTaskDelay(1000):
CPU: [Sleep] ← Otras tasks pueden correr
     0ms    ↓                       1000ms
            [Task despierta exactamente aquí]
```

### 5. vTaskDelayUntil (Delay Absoluto)

**Diferencia crucial entre `vTaskDelay` y `vTaskDelayUntil`**:

```cpp
// vTaskDelay - Delay RELATIVO
void taskA() {
    while(1) {
        hacerAlgo();  // Toma tiempo variable (5-10ms)
        vTaskDelay(pdMS_TO_TICKS(100));  // Espera 100ms DESDE AHORA
    }
}
// Período: 105-110ms (variable)

// vTaskDelayUntil - Delay ABSOLUTO
void taskB() {
    TickType_t lastWake = xTaskGetTickCount();
    while(1) {
        hacerAlgo();  // Toma tiempo variable (5-10ms)
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(100));
    }
}
// Período: EXACTAMENTE 100ms (preciso)
```

**Visualización**:

```
vTaskDelay (relativo):
Ciclo 1: [Trabajo 5ms] [Delay 100ms] = 105ms total
Ciclo 2: [Trabajo 10ms] [Delay 100ms] = 110ms total ← ¡Deriva!

vTaskDelayUntil (absoluto):
Ciclo 1: [Trabajo 5ms] [Delay 95ms] = 100ms total
Ciclo 2: [Trabajo 10ms] [Delay 90ms] = 100ms total ← Preciso
```

**Para nuestro trigger scanner, necesitamos `vTaskDelayUntil`** para mantener exactamente 2kHz (500µs).

---

## Implementación en Nuestro Código

### Arquitectura Global

```cpp
// En main.cpp

TaskHandle_t taskHandle_TriggerScan = NULL;
TaskHandle_t taskHandle_EventProcessor = NULL;
QueueHandle_t queue_HitEvents = NULL;

void setup() {
    // 1. Crear queue
    queue_HitEvents = xQueueCreate(16, sizeof(HitEvent));

    // 2. Crear Task en Core 0 (trigger scanning)
    xTaskCreatePinnedToCore(
        triggerScanTask,        // Función de la task
        "TriggerScan",          // Nombre (para debug)
        4096,                   // Stack size (bytes)
        NULL,                   // Parámetros
        24,                     // Prioridad (máxima)
        &taskHandle_TriggerScan,// Handle
        0                       // Core 0
    );

    // 3. Crear Task en Core 1 (event processing)
    xTaskCreatePinnedToCore(
        eventProcessorTask,
        "EventProcessor",
        4096,
        NULL,
        10,                     // Prioridad media
        &taskHandle_EventProcessor,
        1                       // Core 1
    );
}

void loop() {
    // Vacío - FreeRTOS maneja todo
    vTaskDelay(portMAX_DELAY);  // Dormir infinitamente
}
```

### Task 1: Trigger Scanning (Core 0)

**Archivo**: `src/main_brain/input/trigger_scanner.cpp`

```cpp
void triggerScanTask(void* parameter) {
    // 1. Obtener el tick actual del sistema
    TickType_t lastWakeTime = xTaskGetTickCount();
    //         ^^^^^^^^^^^ Timestamp en "ticks"

    // 2. Calcular período en ticks (500µs = 0.5ms)
    const TickType_t scanPeriodTicks = pdUS_TO_TICKS(500);
    //                                 ^^^^^^^^^^^^^^
    //                                 Macro que convierte µs → ticks

    Serial.println("[triggerScanTask] Started on Core 0");

    // 3. Loop infinito
    while (true) {
        // A. Ejecutar el scan
        triggerScanner.scanLoop();

        // B. Esperar hasta el próximo período EXACTO
        vTaskDelayUntil(&lastWakeTime, scanPeriodTicks);
        //              ^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^
        //              Por referencia  Período deseado
        //              (se actualiza   (500µs en ticks)
        //               automáticamente)
    }
}
```

**¿Cómo funciona `vTaskDelayUntil`?**

```cpp
// Internamente (simplificado):
void vTaskDelayUntil(TickType_t* previousWakeTime, TickType_t increment) {
    TickType_t now = xTaskGetTickCount();
    TickType_t nextWakeTime = *previousWakeTime + increment;

    if (nextWakeTime > now) {
        // Dormir hasta nextWakeTime
        vTaskDelay(nextWakeTime - now);
    }

    // Actualizar para el próximo ciclo
    *previousWakeTime = nextWakeTime;
}
```

**Ejemplo real con números**:

```
Sistema corriendo a 240 MHz
Tick period: 1ms (configurable)
scanPeriodTicks = 0.5 ticks (500µs)

Ciclo 1:
  lastWakeTime = 1000 ticks (1000ms desde boot)
  scanLoop() toma 160µs (0.16 ticks)
  vTaskDelayUntil espera: 0.5 - 0.16 = 0.34 ticks (340µs)

Ciclo 2:
  lastWakeTime = 1000.5 ticks (auto-actualizado)
  scanLoop() toma 200µs (0.20 ticks)
  vTaskDelayUntil espera: 0.5 - 0.20 = 0.30 ticks (300µs)

SIEMPRE despierta exactamente cada 500µs (2kHz)
```

### Task 2: Event Processor (Core 1)

**Archivo**: `src/main_brain/main.cpp`

```cpp
void eventProcessorTask(void* parameter) {
    HitEvent event;  // Estructura para almacenar el evento

    Serial.println("[eventProcessorTask] Started on Core 1");

    while (true) {
        // Esperar INDEFINIDAMENTE hasta que llegue un evento
        if (xQueueReceive(queue_HitEvents, &event, portMAX_DELAY)) {
            //              ^^^^^^^^^^^^^^^^  ^^^^^  ^^^^^^^^^^^^^
            //              Queue             Donde  Timeout
            //                                guardar (infinito)

            // Procesar el evento
            Serial.printf(">> HIT: Pad=%d (%s), Velocity=%d, Time=%lu µs\n",
                          event.padId,
                          PAD_NAMES[event.padId],
                          event.velocity,
                          event.timestamp);

            // Futuro: triggerLEDFlash(), sendMIDI(), etc.
        }
    }
}
```

**¿Qué hace `xQueueReceive`?**

```cpp
BaseType_t xQueueReceive(
    QueueHandle_t queue,     // De qué queue leer
    void* buffer,            // Donde copiar el dato
    TickType_t timeout       // Cuánto esperar si está vacía
);

// Retorna:
// - pdTRUE (1) si recibió un dato
// - pdFALSE (0) si timeout expiró sin datos
```

**Comportamiento según timeout**:

```cpp
// 1. Sin bloqueo (polling)
if (xQueueReceive(queue, &data, 0)) {
    // Dato recibido
} else {
    // Queue vacía, continuar inmediatamente
}

// 2. Bloqueo con timeout
if (xQueueReceive(queue, &data, pdMS_TO_TICKS(100))) {
    // Dato recibido en <100ms
} else {
    // Timeout: no llegó nada en 100ms
}

// 3. Bloqueo infinito (nuestro caso)
xQueueReceive(queue, &data, portMAX_DELAY);
// Task DUERME hasta que llegue un dato
// No consume CPU mientras espera
```

**Estados de la task según queue**:

```
Queue vacía:
┌──────────────────┐
│ eventProcessorTask│ Estado: BLOCKED
│ xQueueReceive()  │ CPU usage: 0%
│ Waiting...       │ Esperando datos
└──────────────────┘

Llega evento (desde Core 0):
┌──────────────────┐
│ triggerScanTask  │ xQueueSend(queue, &event)
│ (Core 0)         │         │
└──────────────────┘         │
                             ▼
                    [Queue tiene datos]
                             │
                             ▼
┌──────────────────┐
│ eventProcessorTask│ Estado: READY → RUNNING
│ Despierta!       │ CPU usage: ~5%
│ Procesa evento   │ Imprime en serial
└──────────────────┘
```

### Queue: Comunicación entre Cores

**Creación de la queue**:

```cpp
// En setup()
queue_HitEvents = xQueueCreate(
    16,              // Capacidad: 16 eventos
    sizeof(HitEvent) // Tamaño por elemento: 8 bytes
);

// Estructura del evento
struct HitEvent {
    uint8_t padId;      // 1 byte
    uint8_t velocity;   // 1 byte
    uint32_t timestamp; // 4 bytes
    // + 2 bytes padding = 8 bytes total
};

// Memoria usada: 16 × 8 = 128 bytes
```

**Envío desde Core 0**:

```cpp
// En trigger_detector.cpp
void TriggerDetector::sendHitEvent(uint8_t padId, uint8_t velocity, uint32_t timestamp) {
    HitEvent event(padId, velocity, timestamp);

    BaseType_t result = xQueueSend(
        hitEventQueue,  // Queue
        &event,         // Puntero al dato (se COPIA)
        0               // No bloquear (timeout = 0)
    );
    //  ^^^^^^^^^^^ ¡CRÍTICO! Core 0 NUNCA debe bloquearse

    if (result != pdPASS) {
        // Queue llena (más de 16 eventos sin procesar)
        Serial.println("[WARNING] Queue overflow!");
        // Evento perdido, pero sistema sigue funcionando
    }
}
```

**Recepción en Core 1**:

```cpp
// En main.cpp
while (true) {
    if (xQueueReceive(queue_HitEvents, &event, portMAX_DELAY)) {
        //                                       ^^^^^^^^^^^^^
        //                          Core 1 SÍ puede bloquearse

        // event ahora contiene una COPIA del dato enviado desde Core 0
        Serial.printf("Pad %d hit\n", event.padId);
    }
}
```

**Visualización del flujo**:

```
Tiempo →

Core 0:                  Core 1:
  │                        │
  ├─ Detecta golpe         │
  │  padId=0, vel=85       │
  │                        │
  ├─ xQueueSend()          │
  │    │                   │
  │    └──[Queue]─────────►│
  │       [Event 0]        │
  │                        ├─ xQueueReceive()
  │                        │  Despierta!
  │                        │
  ├─ Continúa scanning     ├─ Procesa evento
  │  (sin esperar)         │  Print, MIDI, etc.
  │                        │
  ├─ Detecta otro golpe    │
  │  padId=1, vel=100      │
  │                        │
  ├─ xQueueSend()          │
  │    │                   │
  │    └──[Queue]─────────►│
  │       [Event 0]        │
  │       [Event 1]        │ (en cola)
  │                        │
  │                        ├─ Termina evento 0
  │                        │
  │                        ├─ xQueueReceive()
  │                        │  Recibe evento 1
  │                        │
```

---

## Análisis Línea por Línea

### main.cpp - setup()

```cpp
void setup() {
    // 1. Inicializar serial para debug
    Serial.begin(DEBUG_BAUD_RATE);  // 115200
    delay(500);  // Esperar a que serial esté listo

    // 2. Banner de bienvenida
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║   E-DRUM CONTROLLER - MAIN BRAIN      ║");
    Serial.println("╚════════════════════════════════════════╝");

    // 3. Advertencia de hardware
    Serial.println("⚠️  HARDWARE WARNING ⚠️");
    Serial.println("ENSURE PIEZO PROTECTION CIRCUITS ARE INSTALLED!");
    delay(1000);

    // 4. Inicializar hardware (ADC, GPIO, UART, SPI)
    if (!systemInit()) {
        Serial.println("[FATAL ERROR] System initialization failed!");
        while (true) { delay(1000); }  // Halt
    }

    // 5. CREAR QUEUE (ANTES de las tasks que la usan)
    Serial.println("\n--- Creating FreeRTOS Queues ---");
    queue_HitEvents = xQueueCreate(
        QUEUE_SIZE_HIT_EVENTS,  // 16 slots
        sizeof(HitEvent)        // 8 bytes por slot
    );

    // Verificar que se creó correctamente
    if (queue_HitEvents == NULL) {
        Serial.println("[FATAL ERROR] Failed to create queue!");
        while (true) delay(1000);
    }
    Serial.printf("[OK] Hit events queue created (size: %d)\n",
                  QUEUE_SIZE_HIT_EVENTS);

    // 6. Inicializar trigger scanner (le pasamos la queue)
    if (!triggerScanner.begin(queue_HitEvents)) {
        Serial.println("[FATAL ERROR] Trigger scanner init failed!");
        while (true) delay(1000);
    }

    // 7. CREAR TASKS
    Serial.println("\n--- Creating FreeRTOS Tasks ---");

    // Task 1: Trigger Scanning (Core 0, prioridad máxima)
    BaseType_t result = xTaskCreatePinnedToCore(
        triggerScanTask,              // Función
        "TriggerScan",                // Nombre (max 16 chars)
        TASK_STACK_TRIGGER_SCAN,      // 4096 bytes de stack
        NULL,                         // Sin parámetros
        TASK_PRIORITY_TRIGGER_SCAN,   // Prioridad 24
        &taskHandle_TriggerScan,      // Handle (para control)
        TASK_CORE_TRIGGER_SCAN        // Core 0
    );
    //  ^^^^^^^^ Retorna pdPASS si éxito, pdFAIL si error

    if (result != pdPASS) {
        Serial.println("[FATAL ERROR] Failed to create trigger scan task!");
        while (true) delay(1000);
    }
    Serial.println("[OK] Trigger scan task created on Core 0");

    // Task 2: Event Processor (Core 1, prioridad media)
    result = xTaskCreatePinnedToCore(
        eventProcessorTask,
        "EventProcessor",
        4096,
        NULL,
        10,                           // Prioridad 10 (menor que Core 0)
        &taskHandle_EventProcessor,
        1                             // Core 1
    );

    if (result != pdPASS) {
        Serial.println("[FATAL ERROR] Failed to create event processor!");
        while (true) delay(1000);
    }
    Serial.println("[OK] Event processor task created on Core 1");

    // 8. Sistema listo
    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║  SYSTEM READY - STAGE 1: PAD READING   ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println("\nPress 'h' for serial commands help\n");

    // 9. Info del sistema
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Tasks running: %d\n\n", uxTaskGetNumberOfTasks());

    // setup() termina aquí, pero las tasks YA están corriendo!
}
```

**Orden de ejecución**:

```
1. setup() empieza (Core 1)
2. systemInit()
3. xQueueCreate() → queue creada
4. xTaskCreatePinnedToCore() task 1
   └─> triggerScanTask empieza en Core 0
5. xTaskCreatePinnedToCore() task 2
   └─> eventProcessorTask empieza en Core 1
6. setup() termina
7. loop() empieza (pero está casi vacío)

Ahora TRES cosas corren en paralelo:
- Core 0: triggerScanTask (scanning ADC)
- Core 1: eventProcessorTask (procesando eventos)
- Core 1: loop() (idle, casi dormida)
```

### trigger_scanner.cpp - scanLoop()

```cpp
void TriggerScanner::scanLoop() {
    // 1. Timestamp al inicio del scan
    uint32_t scanStartUs = micros();
    //       ^^^^^^^^^^^^^ Microsegundos desde boot

    // 2. Leer todos los pads
    readAllPads();

    // 3. Calcular tiempo que tomó el scan
    uint32_t scanTimeUs = micros() - scanStartUs;

    // 4. Actualizar estadísticas (avg, max, min)
    updateStats(scanTimeUs);

    // 5. Imprimir stats cada 10 segundos (opcional)
    if (millis() - lastStatsTime > 10000) {
        #ifdef DEBUG_TRIGGER_TIMING
        printStats();
        #endif
        lastStatsTime = millis();
    }
}
```

### trigger_scanner.cpp - readAllPads()

```cpp
void TriggerScanner::readAllPads() {
    // 1. Obtener timestamp común para los 4 pads
    uint32_t timestamp = micros();

    // 2. Leer secuencialmente los 4 pads
    for (int pad = 0; pad < NUM_PADS; pad++) {
        // A. Leer ADC
        uint16_t rawValue = analogRead(PAD_ADC_PINS[pad]);
        //       ^^^^^^^^^ Bloquea por ~40µs

        // B. Debug (cada 1000 scans = ~0.5 sec)
        #ifdef DEBUG_TRIGGER_RAW
        if (scanCount % 1000 == 0) {
            Serial.printf("Pad %d: %d\n", pad, rawValue);
        }
        #endif

        // C. Safety check
        checkADCSafety(rawValue, pad);
        //  Verifica que rawValue < 3800 (protección funcionando)

        // D. Procesar muestra con detector
        triggerDetector.processSample(pad, rawValue, timestamp);
        //                            ^^^  ^^^^^^^^  ^^^^^^^^^
        //                            Qué  Valor     Cuándo
        //                            pad  ADC       (para latencia)
    }
}
```

**Tiempo de ejecución**:
```
for pad in 0..3:
    analogRead() : 40µs
    processSample(): 5µs
Total: 4 × 45µs = 180µs (peor caso)
```

### trigger_detector.cpp - processSample()

Ya explicado en detalle en [implementation_explained.md](implementation_explained.md), pero aquí el enfoque en FreeRTOS:

```cpp
void TriggerDetector::processSample(uint8_t padId, uint16_t rawValue, uint32_t timestamp) {
    // ... algoritmo de detección ...

    if (/* peak detectado */) {
        // Enviar evento a queue (NO bloquear)
        sendHitEvent(padId, velocity, timestamp);
    }
}

void TriggerDetector::sendHitEvent(uint8_t padId, uint8_t velocity, uint32_t timestamp) {
    HitEvent event(padId, velocity, timestamp);

    // CRÍTICO: timeout = 0 (no bloquear NUNCA)
    BaseType_t result = xQueueSend(hitEventQueue, &event, 0);
    //                                                     ^
    //                                         No esperar si queue llena

    if (result != pdPASS) {
        // Queue overflow: más de 16 eventos sin procesar
        // Esto solo pasa si el usuario golpea MUY rápido
        // O si Core 1 está muy ocupado
        Serial.printf("[WARNING] Queue full! Lost event from Pad %d\n", padId);
    }
}
```

**¿Por qué timeout = 0?**

```
Si usáramos timeout > 0:

Core 0 corriendo:
  ├─ Detecta golpe
  ├─ xQueueSend(queue, &event, 100)  ← Timeout 100 ticks
  │  Queue llena...
  │  Task BLOQUEA por hasta 100 ticks esperando espacio
  │  ↓
  │  ¡Durante este tiempo NO está scanning ADC!
  │  ¡Triggers perdidos!
  └─ Malo ❌

Con timeout = 0:
  ├─ Detecta golpe
  ├─ xQueueSend(queue, &event, 0)
  │  Queue llena
  │  Retorna inmediatamente con pdFAIL
  │  ↓
  │  Evento perdido, PERO scanning continúa
  └─ Mejor ✅ (evento perdido > sistema bloqueado)
```

---

## Flujo Temporal

### Timeline Completo de un Golpe

```
Tiempo (µs)      Core 0                           Core 1
───────────────────────────────────────────────────────────────
0                triggerScanTask                 eventProcessorTask
                 │                                │ (dormida,
                 │                                │  esperando queue)
                 │
500              ├─ vTaskDelayUntil despierta
                 ├─ scanLoop()
                 │  ├─ micros() = 500
                 │  ├─ readAllPads()
                 │  │  ├─ Pad 0: analogRead() → 50
                 │  │  ├─ processSample(0, 50)
                 │  │  │  └─ IDLE state, signal<threshold
                 │  │  │
540              │  │  ├─ Pad 1: analogRead() → 2500 ← ¡GOLPE!
                 │  │  ├─ processSample(1, 2500)
                 │  │  │  ├─ signal=2450 (después de baseline)
                 │  │  │  ├─ Threshold crossed!
                 │  │  │  └─ State: IDLE → RISING
                 │  │  │
580              │  │  ├─ Pad 2: analogRead() → 45
620              │  │  └─ Pad 3: analogRead() → 30
                 │  │
625              │  ├─ scanTimeUs = 125µs
                 │  └─ updateStats()
                 │
                 ├─ vTaskDelayUntil(375µs)
                 │  (dormida)
                 │
1000             ├─ Despierta (exactamente 500µs después)
                 ├─ scanLoop()
                 │  ├─ readAllPads()
1040             │  │  ├─ Pad 1: analogRead() → 3000 ← Peak subiendo
                 │  │  ├─ processSample(1, 3000)
                 │  │  │  └─ State RISING: peakValue=3000
                 │  │     ...
                 │
... (más scans buscando el peak)
                 │
2540             ├─ Scan #5 después del golpe
                 │  ├─ Pad 1: analogRead() → 2100 ← Cayendo
                 │  ├─ processSample(1, 2100)
                 │  │  ├─ signal < peak*0.7
                 │  │  ├─ ¡Peak encontrado! peak=3000
                 │  │  ├─ velocity = peakToVelocity(3000) = 85
                 │  │  ├─ No crosstalk
                 │  │  │
                 │  │  └─ sendHitEvent(1, 85, 2540)
                 │  │     ├─ xQueueSend(queue, event, 0)
                 │  │     │  Queue: [Event]
                 │  │     │         ↓
2541             │  │     │  ┌──────────────────┐
                 │  │     └──┼─►eventProcessorTask
                 │  │        │  ¡Despierta!      │
                 │  │        │                   │
                 │  │        ├─ xQueueReceive() retorna
                 │  │        │  event = {1, 85, 2540}
                 │  │        │
                 │  │        ├─ Serial.printf(...)
2600             │  │        │  ">> HIT: Pad=1, Vel=85"
                 │  │        │
                 │  │        └─ vTaskDelay o loop again
                 │  │           (esperando próximo evento)
                 │  │
```

**Latencia total**: 2540µs - 540µs = **2000µs** (2ms) desde golpe hasta evento procesado.

---

## Debugging y Troubleshooting

### Ver Tasks Activas

```cpp
// En cualquier momento durante ejecución
void printTaskInfo() {
    Serial.printf("Tasks running: %d\n", uxTaskGetNumberOfTasks());

    // Info de una task específica
    UBaseType_t priority = uxTaskPriorityGet(taskHandle_TriggerScan);
    Serial.printf("TriggerScan priority: %d\n", priority);

    // Stack libre (high water mark)
    UBaseType_t stackFree = uxTaskGetStackHighWaterMark(taskHandle_TriggerScan);
    Serial.printf("TriggerScan stack free: %d bytes\n", stackFree * 4);
}
```

### Detectar Stack Overflow

Si una task usa más stack del asignado → **stack overflow** → crash.

**Síntomas**:
- Reset random
- Comportamiento errático
- Mensajes: "***ERROR*** A stack overflow in task X has been detected"

**Cómo detectar**:

```cpp
// En setup() después de crear tasks
void checkStacks() {
    UBaseType_t freeStack;

    freeStack = uxTaskGetStackHighWaterMark(taskHandle_TriggerScan);
    Serial.printf("TriggerScan: %d bytes free (of 4096)\n", freeStack * 4);

    if (freeStack < 512) {  // <512 bytes libres
        Serial.println("[WARNING] TriggerScan stack low!");
    }
}
```

**Solución**: Aumentar stack size en `xTaskCreatePinnedToCore`.

### Detectar Queue Overflow

```cpp
// Verificar si queue está llena
UBaseType_t waiting = uxQueueMessagesWaiting(queue_HitEvents);
UBaseType_t spaces = uxQueueSpacesAvailable(queue_HitEvents);

Serial.printf("Queue: %d events waiting, %d spaces free\n", waiting, spaces);

if (spaces == 0) {
    Serial.println("[WARNING] Queue full! Events may be lost!");
}
```

### Medir Tiempo de Ejecución de Task

```cpp
void triggerScanTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    uint32_t maxExecTime = 0;

    while (true) {
        uint32_t start = micros();

        triggerScanner.scanLoop();

        uint32_t execTime = micros() - start;
        if (execTime > maxExecTime) {
            maxExecTime = execTime;
            Serial.printf("[DEBUG] New max exec time: %lu µs\n", maxExecTime);
        }

        vTaskDelayUntil(&lastWakeTime, pdUS_TO_TICKS(500));
    }
}
```

### Verificar en Qué Core Corre Cada Task

```cpp
void printCoreInfo() {
    // Desde dentro de una task
    Serial.printf("TriggerScan running on core: %d\n",
                  xPortGetCoreID());
}

// Output esperado:
// TriggerScan running on core: 0
// EventProcessor running on core: 1
```

### Comandos de Debug Útiles

```cpp
// En loop() o task de monitoreo
void monitorSystem() {
    static uint32_t lastPrint = 0;

    if (millis() - lastPrint > 5000) {  // Cada 5 segundos
        Serial.println("\n=== System Status ===");
        Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
        Serial.printf("Min free heap: %d bytes\n", ESP.getMinFreeHeap());
        Serial.printf("Tasks: %d\n", uxTaskGetNumberOfTasks());
        Serial.printf("Queue events waiting: %d\n",
                      uxQueueMessagesWaiting(queue_HitEvents));

        lastPrint = millis();
    }
}
```

---

## Preguntas Frecuentes

### ¿Por qué usar Core 0 para triggers?

**ESP32-S3 Dual-Core**:
- **Core 0**: Llamado "Protocol CPU" o "PRO_CPU"
- **Core 1**: Llamado "Application CPU" o "APP_CPU"

Por defecto, WiFi/Bluetooth usan Core 0. Pero nosotros **no usamos WiFi/BT**, así que Core 0 está **100% dedicado** a nuestro trigger scanning.

### ¿Qué pasa si no uso `vTaskDelayUntil`?

Sin `vTaskDelayUntil`, el período derifaría:

```cpp
// Malo
while(1) {
    scanLoop();  // Toma 160µs
    vTaskDelay(pdUS_TO_TICKS(500));  // Espera 500µs DESDE AHORA
    // Período real: 660µs → 1515 Hz (no 2000 Hz)
}

// Bueno
while(1) {
    scanLoop();  // Toma 160µs
    vTaskDelayUntil(&lastWake, pdUS_TO_TICKS(500));
    // Período real: 500µs → 2000 Hz (exacto)
}
```

### ¿Puedo tener más tasks?

¡Sí! Puedes crear tantas como quieras (limitado por RAM).

```cpp
// Ejemplo: Task para LEDs (Etapa 2)
xTaskCreatePinnedToCore(
    ledAnimationTask,
    "LEDAnimation",
    4096,
    NULL,
    5,                 // Prioridad baja
    NULL,
    1                  // Core 1
);

void ledAnimationTask(void* param) {
    while(1) {
        updatePadLEDs();
        updateEncoderLEDs();
        FastLED.show();
        vTaskDelay(pdMS_TO_TICKS(16));  // 60 FPS
    }
}
```

### ¿Cómo sé si necesito más prioridad?

**Regla general**:
- **Alta prioridad (20-24)**: Tiempo real crítico (triggers, I2S audio)
- **Media prioridad (10-19)**: Comunicación (UART, MIDI)
- **Baja prioridad (1-9)**: UI, LEDs, logging

Si una task de baja prioridad no corre → aumentar prioridad.

### ¿Queue o variable global?

```cpp
// ❌ Malo: Variable global (race condition)
volatile int lastHitPad = -1;

// Core 0
lastHitPad = 1;  // Escribe

// Core 1
if (lastHitPad >= 0) {  // Lee
    // Puede leer valor antiguo o corrupto
}

// ✅ Bueno: Queue (thread-safe)
xQueueSend(queue, &pad);    // Core 0
xQueueReceive(queue, &pad); // Core 1 (sincronizado)
```

---

## Resumen

### ¿Qué aprendimos?

1. **FreeRTOS** permite multitasking real en microcontroladores
2. **Tasks** corren en paralelo con prioridades
3. **Queues** permiten comunicación thread-safe entre tasks
4. **vTaskDelayUntil** garantiza timing preciso
5. **Dual-core** permite separar trabajo crítico (Core 0) de aplicación (Core 1)

### ¿Por qué esta arquitectura?

- **Latencia predecible**: Siempre <2ms
- **Sin bloqueos**: Core 0 nunca se detiene
- **Escalable**: Agregar features sin afectar triggers
- **Robusto**: Queue maneja picos de eventos

### Próximos pasos

1. Compilar y probar firmware actual
2. Experimentar con prioridades
3. Agregar más tasks en Etapa 2 (LEDs)
4. Medir timing real con osciloscopio

---

**Versión**: 1.0
**Fecha**: 2025-12-02
**Nivel**: Intermedio
**Prerequisitos**: C/C++, conceptos básicos de multithreading

¿Preguntas? Revisa la documentación oficial de FreeRTOS: https://www.freertos.org/
