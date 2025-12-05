# 💡 Esquema de Colores LED - Universal Idle + Colores Personalizados

## 🎨 Concepto de Diseño

**Idle:** Todos los LEDs en el mismo color blanco azulado suave (coherencia visual)
**Hit:** Cada pad tiene su color único asignado (identificación visual instantánea)

## 🌟 Paleta de Colores

### Estado IDLE (Reposo) - Universal
Todos los pads brillan con el mismo color blanco azulado suave.

```
┌─────────────────────────────────────────────────────────────┐
│                  TODOS LOS PADS EN REPOSO                    │
│                     (40% Brightness)                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│         ▰▰▰   ▰▰▰   ▰▰▰   ▰▰▰                               │
│        Kick  Snare HiHat  Tom                                │
│                                                              │
│     Color Universal: Blanco Azulado                          │
│     RGB(100, 120, 140)                                       │
│     #64788C                                                  │
│                                                              │
│     Efecto: Suave iluminación ambiente uniforme              │
│     que no distrae pero indica que el sistema                │
│     está activo y listo para tocar.                          │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### Estado HIT (Al Golpear) - Colores Únicos
Cada pad explota con su color característico asignado.

```
┌─────────────────────────────────────────────────────────────┐
│                   COLORES AL GOLPEAR                         │
│                  (100-255 Brightness)                        │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  🥁 Kick (Pad 0)    ████████   CIAN 🌊                      │
│                     RGB(0, 255, 255)                         │
│                     #00FFFF                                  │
│                     Azul agua eléctrico                      │
│                                                              │
│  🥁 Snare (Pad 1)   ████████   ROSA 💗                      │
│                     RGB(255, 50, 150)                        │
│                     #FF3296                                  │
│                     Rosa/magenta vibrante                    │
│                                                              │
│  🥁 HiHat (Pad 2)   ████████   AMARILLO ⚡                  │
│                     RGB(255, 255, 0)                         │
│                     #FFFF00                                  │
│                     Amarillo brillante puro                  │
│                                                              │
│  🥁 Tom (Pad 3)     ████████   VERDE 🌿                     │
│                     RGB(0, 255, 100)                         │
│                     #00FF64                                  │
│                     Verde esmeralda brillante                │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## 🎬 Animación Visual

### Secuencia Temporal (300ms fade)

```
   IDLE (Reposo)        Hit Detectado        Fade Suave         Retorno a Idle
       40%                   100%              80% → 40%             40%

    ▰ ▰ ▰ ▰           🌊 ▰ ▰ ▰            🌊 ▰ ▰ ▰          ▰ ▰ ▰ ▰
   Blanco azul         Cian brillante      Fade gradual      Blanco azul
                           ███                ██ → ▰

    │                      │                   │                  │
    └──────────────────────┴───────────────────┴──────────────────┘
    Esperando            0ms                 150ms             300ms
```

### Ejemplo de Secuencia de Batería:

```
   Tiempo:    0ms      100ms     200ms     300ms     400ms

   Kick:      🌊███     🌊██      🌊▰       ▰         ▰

   Snare:     ▰         💗███     💗██      💗▰       ▰

   HiHat:     ▰         ▰         ⚡███     ⚡██      ⚡▰

   Tom:       ▰         ▰         ▰         🌿███     🌿██

   Resultado: Cascada de colores sincronizada con tu ritmo! 🎵
```

## 📊 Tabla de Referencia Completa

### Color Idle (Universal):
| Estado | R   | G   | B   | Hex     | Brightness | Descripción        |
|--------|-----|-----|-----|---------|------------|--------------------|
| Idle   | 100 | 120 | 140 | #64788C | 40%        | Blanco azul suave  |

### Colores Hit (Por Pad):
| Pad | Instrumento | R   | G   | B   | Hex     | Color      | Emoji |
|-----|-------------|-----|-----|-----|---------|------------|-------|
| 0   | Kick        | 0   | 255 | 255 | #00FFFF | Cian       | 🌊    |
| 1   | Snare       | 255 | 50  | 150 | #FF3296 | Rosa       | 💗    |
| 2   | HiHat       | 255 | 255 | 0   | #FFFF00 | Amarillo   | ⚡    |
| 3   | Tom         | 0   | 255 | 100 | #00FF64 | Verde      | 🌿    |

## 🎯 Ventajas de Este Esquema

### ✅ Coherencia Visual
- En reposo: Todos iguales = sistema unificado y profesional
- No hay colores que compitan por atención cuando no estás tocando

### ✅ Identificación Instantánea
- Al golpear: Color único por pad = sabes exactamente qué golpeaste
- Ideal para aprender patrones complejos

### ✅ Efecto Dramático
- Contraste fuerte entre idle (blanco azulado) y hit (colores vibrantes)
- El cambio es muy visible y satisfactorio visualmente

### ✅ Personalizable
- Fácil cambiar colores editando el array `PAD_LED_HIT_COLORS`
- Usuarios pueden configurar sus colores favoritos

## 🔧 Personalización de Colores

### Cambiar colores al golpear

Edita en [main.cpp](src/main_brain/main.cpp) líneas 70-75:

```cpp
const CRGB PAD_LED_HIT_COLORS[4] = {
    CRGB(0, 255, 255),    // Pad 0: Kick - Cian
    CRGB(255, 50, 150),   // Pad 1: Snare - Rosa
    CRGB(255, 255, 0),    // Pad 2: HiHat - Amarillo
    CRGB(0, 255, 100)     // Pad 3: Tom - Verde
};
```

**Ejemplos de otros colores:**

```cpp
// Esquema "Fire" (fuego):
CRGB(255, 0, 0),      // Rojo
CRGB(255, 100, 0),    // Naranja
CRGB(255, 200, 0),    // Amarillo fuego
CRGB(255, 50, 0)      // Naranja rojizo

// Esquema "Ocean" (océano):
CRGB(0, 100, 255),    // Azul profundo
CRGB(0, 200, 255),    // Azul cielo
CRGB(0, 255, 200),    // Turquesa
CRGB(100, 255, 255)   // Azul claro

// Esquema "Sunset" (atardecer):
CRGB(255, 50, 100),   // Rosa atardecer
CRGB(255, 100, 50),   // Naranja
CRGB(200, 0, 100),    // Púrpura
CRGB(255, 200, 0)     // Dorado

// Esquema "Neon":
CRGB(255, 0, 255),    // Magenta neón
CRGB(0, 255, 0),      // Verde neón
CRGB(255, 255, 0),    // Amarillo neón
CRGB(0, 255, 255)     // Cian neón
```

### Cambiar color idle

Edita en [main.cpp](src/main_brain/main.cpp) línea 67:

```cpp
const CRGB PAD_LED_IDLE_COLOR = CRGB(100, 120, 140);  // ← Cambia estos valores
```

**Ejemplos:**

```cpp
// Más azul:
CRGB(50, 80, 150)     // Azul más saturado

// Más blanco:
CRGB(120, 130, 140)   // Casi blanco puro

// Tinte verde:
CRGB(100, 140, 120)   // Blanco verdoso

// Tinte rosa:
CRGB(140, 100, 120)   // Blanco rosado
```

### Ajustar brillo

En [main.cpp](src/main_brain/main.cpp):

**Brillo Idle (línea 154):**
```cpp
NeoPixelController::setIdleColor(i, idleColor, 40);  // ← 40%
//                                              ^^
//                                              Valores: 0-100
//                                              20 = muy tenue
//                                              40 = suave (actual)
//                                              60 = medio
//                                              80 = brillante
```

**Velocidad de fade (línea 231):**
```cpp
NeoPixelController::flashPad(event.padId, hitColor, brightness, 300);
//                                                              ^^^
//                                                              Milisegundos
//                                                              100 = muy rápido
//                                                              300 = suave (actual)
//                                                              500 = lento/dramático
```

## 📐 Implementación en el Código

### Inicialización (setup):
```cpp
// Un solo color para todos en idle
uint32_t idleColor = ((uint32_t)PAD_LED_IDLE_COLOR.r << 16) |
                     ((uint32_t)PAD_LED_IDLE_COLOR.g << 8) |
                     PAD_LED_IDLE_COLOR.b;

for (uint8_t i = 0; i < NUM_PADS; i++) {
    NeoPixelController::setIdleColor(i, idleColor, 40);
}
```

### Al detectar golpe (processHitEvents):
```cpp
// Color específico del pad
CRGB color = PAD_LED_HIT_COLORS[event.padId];
uint32_t hitColor = ((uint32_t)color.r << 16) |
                    ((uint32_t)color.g << 8) |
                    color.b;

// Brillo según velocidad del golpe
uint8_t brightness = map(event.velocity, 0, 127, 100, 255);

// Flash con fade de 300ms
NeoPixelController::flashPad(event.padId, hitColor, brightness, 300);
```

## 🎨 Visualización del Efecto

### Vista desde arriba de tu batería:

```
           ┌─────────────────────┐
           │    DRUM MODULE      │
           └─────────────────────┘
                    │
        ┌───────────┴───────────┐
        │                       │
    ┌───▼───┐               ┌───▼───┐
    │ ▰ ▰ ▰ │               │ ▰ ▰ ▰ │
    │ Kick  │               │ Snare │
    └───────┘               └───────┘
      Idle                    Idle

    ┌───────┐               ┌───────┐
    │ ▰ ▰ ▰ │               │ ▰ ▰ ▰ │
    │ HiHat │               │ Tom   │
    └───────┘               └───────┘
      Idle                    Idle
```

### Cuando tocas una secuencia (Kick → Snare → HiHat):

```
    ┌───────┐               ┌───────┐
    │ 🌊 ███ │               │ ▰ ▰ ▰ │     ← Kick golpeado (CIAN)
    │ Kick  │               │ Snare │
    └───────┘               └───────┘

    ┌───────┐               ┌───────┐
    │ 🌊 ▰ ▰ │               │ 💗 ███ │     ← Snare golpeado (ROSA)
    │ Kick  │               │ Snare │       Kick volviendo a idle
    └───────┘               └───────┘

    ┌───────┐               ┌───────┐
    │ ▰ ▰ ▰ │               │ 💗 ▰ ▰ │     ← HiHat golpeado (AMARILLO)
    │ HiHat │               │ Snare │       Otros volviendo a idle
    └───────┘               └───────┘
    ⚡ ███
```

## 🚀 Próximos Pasos

### Integración con GUI:
En el futuro, estos colores serán configurables desde la interfaz gráfica:

```cpp
// Usuario selecciona color desde GUI
PadConfig& cfg = PadConfigManager::getConfig(padId);
cfg.ledColorHit = userSelectedColor;  // Color personalizado
cfg.ledColorIdle = PAD_LED_IDLE_COLOR;  // O también configurable

// Guardar en NVS
PadConfigManager::saveToNVS();
```

---

**Esquema actual:** Idle universal (blanco azul) | Hit colors (Cian/Rosa/Amarillo/Verde)
**Fade duration:** 300ms
**Idle brightness:** 40%
**Hit brightness:** 100-255% (velocity-mapped)
**Última actualización:** 2025-12-05

¡Disfruta del show de luces! 🥁✨
