#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// Configuración del motor
#define AUDIO_MAX_VOICES 12        // Polifonía máxima (12 sonidos simultáneos)
#define AUDIO_BUFFER_SIZE 256      // Tamaño del buffer de mezcla (muestras por frame)
#define AUDIO_SAMPLE_RATE 44100

// Estructura de una voz individual
struct AudioVoice {
    bool active = false;           // Si está sonando o no
    const int16_t* data = nullptr; // Puntero al inicio del sample en PSRAM
    uint32_t length = 0;           // Longitud total en muestras
    uint32_t position = 0;         // Posición actual de reproducción
    float volume = 1.0f;           // Volumen (0.0 a 1.0) base
    float velocity = 1.0f;         // Velocidad del golpe (0.0 a 1.0)
    uint8_t chokeGroup = 0;        // Grupo de exclusión (0 = ninguno)
    bool loop = false;             // (Futuro) Para loops
};

namespace AudioEngine {

    // Inicializa I2S y la tarea de mezcla
    bool begin();
    
    // Reproduce un tono de prueba sintético (440Hz sine wave)
    void playTestTone();

    // Dispara un sonido (Non-blocking)
    // sampleName: nombre del archivo cargado
    // velocity: fuerza del golpe (0-127)
    // chokeGroup: ID de grupo de exclusión (ej. 1 para HiHat). 0 = sin exclusión.
    void play(const char* sampleName, uint8_t velocity, uint8_t volume = 127, uint8_t chokeGroup = 0);

    // Detiene todos los sonidos de un grupo específico (ej. cerrar HiHat)
    void choke(uint8_t chokeGroup);

    // Detiene todo (Panic)
    void stopAll();

}  // namespace AudioEngine

#endif  // AUDIO_ENGINE_H