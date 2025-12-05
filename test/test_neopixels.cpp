/**
 * @file test_neopixels.cpp
 * @brief Test simple para verificar NeoPixels (WS2812B)
 *
 * Este test verifica:
 * - Conexión correcta de los 4 LEDs
 * - Orden de colores (GRB vs RGB)
 * - Brillo y alimentación
 * - Cada LED individualmente
 */

#include <Arduino.h>
#include <FastLED.h>

// Configuración
#define LED_PIN 48        // GPIO 48 (LED_PADS_PIN)
#define NUM_LEDS 4        // 4 NeoPixels (uno por pad)
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB   // Típico para WS2812B

// Array de LEDs
CRGB leds[NUM_LEDS];

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n╔════════════════════════════════════════╗");
    Serial.println("║   TEST NEOPIXELS - WS2812B            ║");
    Serial.println("╚════════════════════════════════════════╝\n");

    // Inicializar FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(255);  // Máximo brillo
    FastLED.clear();
    FastLED.show();

    Serial.println("✅ FastLED inicializado");
    Serial.printf("   Pin: GPIO %d\n", LED_PIN);
    Serial.printf("   LEDs: %d\n", NUM_LEDS);
    Serial.printf("   Tipo: WS2812B (%s)\n\n",
                  COLOR_ORDER == GRB ? "GRB" :
                  COLOR_ORDER == RGB ? "RGB" : "Otro");

    Serial.println("Iniciando secuencia de test...\n");
    delay(1000);
}

void loop() {
    Serial.println("═══════════════════════════════════════");
    Serial.println(" TEST 1: Todos los LEDs - Rojo");
    Serial.println("═══════════════════════════════════════");
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(2000);

    Serial.println("\n═══════════════════════════════════════");
    Serial.println(" TEST 2: Todos los LEDs - Verde");
    Serial.println("═══════════════════════════════════════");
    fill_solid(leds, NUM_LEDS, CRGB::Green);
    FastLED.show();
    delay(2000);

    Serial.println("\n═══════════════════════════════════════");
    Serial.println(" TEST 3: Todos los LEDs - Azul");
    Serial.println("═══════════════════════════════════════");
    fill_solid(leds, NUM_LEDS, CRGB::Blue);
    FastLED.show();
    delay(2000);

    Serial.println("\n═══════════════════════════════════════");
    Serial.println(" TEST 4: Todos los LEDs - Blanco");
    Serial.println("═══════════════════════════════════════");
    fill_solid(leds, NUM_LEDS, CRGB::White);
    FastLED.show();
    delay(2000);

    Serial.println("\n═══════════════════════════════════════");
    Serial.println(" TEST 5: LEDs Individuales");
    Serial.println("═══════════════════════════════════════");
    FastLED.clear();
    FastLED.show();
    delay(500);

    for (int i = 0; i < NUM_LEDS; i++) {
        Serial.printf("   LED %d encendido (Rojo)\n", i);
        leds[i] = CRGB::Red;
        FastLED.show();
        delay(1000);
        leds[i] = CRGB::Black;
    }
    FastLED.show();
    delay(500);

    Serial.println("\n═══════════════════════════════════════");
    Serial.println(" TEST 6: Rainbow en cada LED");
    Serial.println("═══════════════════════════════════════");
    for (int hue = 0; hue < 255; hue += 5) {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CHSV(hue + (i * 64), 255, 255);
        }
        FastLED.show();
        delay(20);
    }
    delay(1000);

    Serial.println("\n═══════════════════════════════════════");
    Serial.println(" TEST 7: Test de Brillo (100% → 10%)");
    Serial.println("═══════════════════════════════════════");
    fill_solid(leds, NUM_LEDS, CRGB::White);
    for (int brightness = 255; brightness >= 25; brightness -= 10) {
        FastLED.setBrightness(brightness);
        FastLED.show();
        Serial.printf("   Brillo: %d%%\n", (brightness * 100) / 255);
        delay(200);
    }
    FastLED.setBrightness(255);  // Restaurar brillo máximo
    delay(1000);

    Serial.println("\n═══════════════════════════════════════");
    Serial.println(" TEST 8: Simulación de Hit (Flash)");
    Serial.println("═══════════════════════════════════════");
    FastLED.clear();
    FastLED.show();
    delay(500);

    for (int i = 0; i < NUM_LEDS; i++) {
        Serial.printf("   Pad %d: Flash!\n", i);

        // Flash instantáneo
        leds[i] = CRGB::Red;
        FastLED.show();

        // Fade a negro
        for (int brightness = 255; brightness >= 0; brightness -= 5) {
            leds[i].fadeToBlackBy(5);
            FastLED.show();
            delay(10);
        }

        delay(300);
    }

    Serial.println("\n═══════════════════════════════════════");
    Serial.println(" TEST 9: Todos los LEDs - Diferentes Colores");
    Serial.println("═══════════════════════════════════════");
    leds[0] = CRGB::Red;      // Pad 0: Rojo
    leds[1] = CRGB::Green;    // Pad 1: Verde
    leds[2] = CRGB::Cyan;     // Pad 2: Cyan
    leds[3] = CRGB::Blue;     // Pad 3: Azul
    FastLED.show();
    Serial.println("   LED 0 (Kick):  Rojo");
    Serial.println("   LED 1 (Snare): Verde");
    Serial.println("   LED 2 (HiHat): Cyan");
    Serial.println("   LED 3 (Tom):   Azul");
    delay(3000);

    Serial.println("\n═══════════════════════════════════════");
    Serial.println(" TEST COMPLETADO - Apagando LEDs");
    Serial.println("═══════════════════════════════════════\n");
    FastLED.clear();
    FastLED.show();
    delay(3000);

    Serial.println("🔄 Reiniciando test en 2 segundos...\n\n");
    delay(2000);
}
