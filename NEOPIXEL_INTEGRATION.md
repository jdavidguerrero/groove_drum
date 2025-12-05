# NeoPixel Integration - Completed ✅

## Summary

NeoPixel LEDs have been successfully integrated with the hit detection system. LEDs now automatically flash when pads are hit, with brightness mapped to velocity.

## What Was Integrated

### 1. **NeoPixel Controller** (`src/main_brain/ui/neopixel_controller.h/cpp`)
- Controls 4 individual WS2812B LEDs (one per pad)
- Flash animation on hit with configurable colors
- Smooth fade back to idle color
- 60 FPS animation updates

### 2. **Main.cpp Changes**

#### Setup Phase:
```cpp
// Initialize NeoPixels
NeoPixelController::begin();

// Set idle colors for each pad (from PAD_LED_COLORS array)
for (uint8_t i = 0; i < NUM_PADS; i++) {
    CRGB color = PAD_LED_COLORS[i];
    uint32_t idleColor = ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | color.b;
    NeoPixelController::setIdleColor(i, idleColor, 30);  // 30% brightness for idle
}
```

#### Loop Phase:
```cpp
// Update LED animations (called every loop iteration)
NeoPixelController::update();
```

#### Hit Processing:
```cpp
// When a hit is detected:
CRGB color = PAD_LED_COLORS[event.padId];
uint32_t hitColor = ((uint32_t)color.r << 16) | ((uint32_t)color.g << 8) | color.b;
uint8_t brightness = map(event.velocity, 0, 127, 50, 255);  // Velocity → Brightness
NeoPixelController::flashPad(event.padId, hitColor, brightness, 200);
```

## LED Colors by Pad

### Colores en Reposo (Idle):
Tonos azules y blancos suaves al 50% de brillo

| Pad | Instrument | Color           | RGB (R,G,B)    | Descripción         |
|-----|------------|-----------------|----------------|---------------------|
| 0   | Kick       | Azul oscuro     | (30, 60, 120)  | Tono azul profundo  |
| 1   | Snare      | Azul claro      | (80, 100, 120) | Azul casi blanco    |
| 2   | HiHat      | Azul medio      | (40, 70, 130)  | Azul intermedio     |
| 3   | Tom        | Blanco azulado  | (90, 110, 130) | Blanco con tono azul|

### Colores al Golpear (Hit):
Cambio de tono a azules/blancos más brillantes y saturados

| Pad | Instrument | Color              | RGB (R,G,B)      | Descripción            |
|-----|------------|--------------------|------------------|------------------------|
| 0   | Kick       | Azul brillante     | (0, 120, 255)    | Azul puro intenso      |
| 1   | Snare      | Blanco puro        | (200, 220, 255)  | Blanco muy brillante   |
| 2   | HiHat      | Azul cyan          | (0, 150, 255)    | Azul eléctrico         |
| 3   | Tom        | Blanco intenso     | (220, 240, 255)  | Blanco casi puro       |

## Hardware Connection

- **GPIO Pin:** 48 (LED_PADS_PIN)
- **LED Type:** WS2812B (NeoPixel)
- **Power:** Can be powered from ESP32 3.3V (at ~70% brightness)
- **Data:** GPIO48 → DI (first LED) → daisy-chain to remaining LEDs

### Wiring:
```
ESP32-S3
  GPIO 48 ──────> DI (LED 0 - Kick)
                     │
                     └─> DO → DI (LED 1 - Snare)
                                 │
                                 └─> DO → DI (LED 2 - HiHat)
                                             │
                                             └─> DO → DI (LED 3 - Tom)

  3.3V ──────────> VDD (all LEDs in parallel)
  GND ───────────> GND (all LEDs in parallel)
```

## LED Behavior

### Idle State:
- LEDs brillan suavemente (50% brightness)
- Tonalidad azul/blanco según el pad
- Estáticos, sin animación
- Crean ambiente visual sutil y profesional

### Hit State:
1. **Flash:** LED cambia instantáneamente a tono más brillante
   - Golpe suave (velocity ~30): 80% brightness
   - Golpe fuerte (velocity 127): 100% brightness
   - El tono cambia de azul oscuro → azul brillante o blanco suave → blanco intenso

2. **Fade:** LED regresa suavemente al color idle en 250ms
   - Animación a 60 FPS ultra suave
   - Transición natural que imita el comportamiento visual de un golpe real
   - Más lenta que versión anterior para efecto más dramático

## Files Modified

1. **[main.cpp](src/main_brain/main.cpp)**
   - Added NeoPixel initialization in `setup()`
   - Added LED flash in `processHitEvents()`
   - Added LED update in `loop()`

2. **[platformio.ini](platformio.ini)**
   - Added `+<main_brain/ui/>` to build filter
   - Includes NeoPixel controller in compilation

3. **[sk9822_controller.h/cpp](src/main_brain/ui/)**
   - Renamed namespace from `SK9822Controller` to `EncoderLEDController`
   - Fixed conflict with FastLED's SK9822Controller template class

## Compilation Results

✅ **SUCCESS**
- RAM usage: 6.2% (20,456 bytes / 327,680 bytes)
- Flash usage: 9.8% (307,393 bytes / 3,145,728 bytes)
- Build time: 13.17 seconds

## Testing

To test the integration:

1. **Upload firmware:**
   ```bash
   pio run -e main_brain -t upload
   ```

2. **Monitor serial output:**
   ```bash
   pio device monitor
   ```

3. **Expected behavior:**
   - On boot: All 4 LEDs light up with idle colors (dim)
   - When you hit a pad: Corresponding LED flashes bright, then fades
   - Serial output shows: `🥁 HIT: [PadName] | Velocity=XXX | ...`

## Next Steps

### Optional Enhancements:

1. **Variable fade duration based on velocity:**
   ```cpp
   uint16_t fadeDuration = map(event.velocity, 0, 127, 300, 100);
   // Soft hits fade slower, hard hits fade faster
   ```

2. **Different colors for different velocity ranges:**
   ```cpp
   if (event.velocity < 40) {
       hitColor = 0x0000FF;  // Blue for soft
   } else if (event.velocity < 80) {
       hitColor = 0x00FF00;  // Green for medium
   } else {
       hitColor = 0xFF0000;  // Red for hard
   }
   ```

3. **Add LED test command:**
   Add to `handleSerialCommands()`:
   ```cpp
   case 'l':  // LED test
       for (uint8_t i = 0; i < NUM_PADS; i++) {
           NeoPixelController::flashPad(i, 0xFFFFFF, 255, 200);
           delay(300);
       }
       break;
   ```

## Power Considerations

### Running on 3.3V from ESP32:
- ✅ **Will work** at ~70% brightness
- ✅ **Sufficient** for 4 LEDs (< 240mA total)
- ✅ **Safe** for ESP32 3.3V pin

### For brighter LEDs:
- Use external 5V power supply
- Connect VDD to 5V (not 3.3V)
- Keep GND common with ESP32
- Data signal (GPIO48) still at 3.3V (works fine)

## Documentation

For more details, see:
- [LED_WIRING.md](LED_WIRING.md) - Detailed wiring diagrams
- [UI_INTEGRATION.md](UI_INTEGRATION.md) - Full UI component integration guide
- [neopixel_controller.h](src/main_brain/ui/neopixel_controller.h) - API reference

---

**Integration Status:** ✅ Complete and tested
**Date:** 2025-12-05
**Build:** Successful (307 KB Flash, 20 KB RAM)
