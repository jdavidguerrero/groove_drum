#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>

namespace AudioEngine {

// Inicializa I2S para el PCM5102A
bool begin(uint32_t sampleRate = 44100, uint8_t bitsPerSample = 16);

// Genera un pequeño click/sine de prueba (bloqueante corta duración)
bool playClick(uint8_t velocity, uint8_t volume, uint16_t durationMs = 50);

}  // namespace AudioEngine

#endif  // AUDIO_ENGINE_H
