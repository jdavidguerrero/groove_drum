#ifndef AUDIO_SAMPLES_H
#define AUDIO_SAMPLES_H

#include <Arduino.h>

// SD.h only needed when not using embedded samples
#ifndef USE_EMBEDDED_SAMPLES
#include <SD.h>
#endif

struct Sample {
    int16_t* data = nullptr;   // PCM signed 16-bit
    uint32_t frames = 0;       // frames = samples per channel
    uint32_t sampleRate = 44100;
    uint8_t channels = 1;      // 1=mono, 2=stereo
};

namespace SampleManager {

// Inicializa samples (embebidos o desde SD) y los carga.
// @return número de samples cargados correctamente.
size_t beginAndLoadDefaults();

// Busca un sample cargado por nombre (ruta).
const Sample* getSample(const char* name);

// Cantidad de samples actualmente cargados
size_t loadedCount();

// Carga o recarga un sample específico desde SD
// @param path Ruta del archivo WAV en SD
// @return true si se cargó correctamente
bool loadSample(const char* path);

// Descarga un sample de memoria
void unloadSample(const char* path);

} // namespace SampleManager

#endif // AUDIO_SAMPLES_H