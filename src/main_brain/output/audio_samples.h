#ifndef AUDIO_SAMPLES_H
#define AUDIO_SAMPLES_H

#include <Arduino.h>
#include <SD.h>

struct Sample {
    int16_t* data = nullptr;   // PCM signed 16-bit
    uint32_t frames = 0;       // frames = samples per channel
    uint32_t sampleRate = 44100;
    uint8_t channels = 1;      // 1=mono, 2=stereo
};

namespace SampleManager {

// Inicializa SD (SPI) y carga un conjunto por defecto en PSRAM.
// @return número de samples cargados correctamente.
size_t beginAndLoadDefaults();

// Busca un sample cargado por nombre (ruta).
const Sample* getSample(const char* name);

// Cantidad de samples actualmente cargados
size_t loadedCount();

// Reproduce un sample cargado, escalando por velocity/volume (bloqueante).
bool playSample(const char* name, uint8_t velocity, uint8_t volume);

} // namespace SampleManager

#endif // AUDIO_SAMPLES_H
