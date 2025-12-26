#include "audio_engine.h"
#include "audio_samples.h"
#include <edrum_config.h>
#include <driver/i2s.h>
#include <math.h>
#include <algorithm>

namespace AudioEngine {

// ============================================================
// VARIABLES INTERNAS
// ============================================================

static AudioVoice voices[AUDIO_MAX_VOICES];
static TaskHandle_t mixerTaskHandle = nullptr;
static SemaphoreHandle_t mixerMutex = nullptr;
static bool initialized = false;

// Buffer de mezcla (Stereo Interleaved)
// 2 canales * AUDIO_BUFFER_SIZE muestras
static int16_t outputBuffer[AUDIO_BUFFER_SIZE * 2];

// ============================================================
// TAREA DE MEZCLA (Core 1)
// ============================================================

void mixerTask(void* parameter) {
    Serial.println("[AUDIO] Mixer task started on Core 1");

    uint32_t lastDebugTime = 0;

    while (true) {
        // Limpiar buffer (silencio)
        memset(outputBuffer, 0, sizeof(outputBuffer));

        bool signalPresent = false;

        // Tomar el mutex para leer las voces de forma segura
        if (xSemaphoreTake(mixerMutex, portMAX_DELAY)) {
            // ... (rest of logic) ...
            
            // Iterar sobre cada muestra del buffer
            for (int i = 0; i < AUDIO_BUFFER_SIZE; i++) {
                int32_t leftAccumulator = 0;
                int32_t rightAccumulator = 0;

                // Mezclar todas las voces activas
                for (int v = 0; v < AUDIO_MAX_VOICES; v++) {
                    AudioVoice& voice = voices[v];

                    if (voice.active) {
                        // Leer muestra actual (Mono source -> Stereo output)
                        // TODO: Si los samples son stereo, ajustar aquí. Asumimos mono por ahora.
                        int16_t sample = voice.data[voice.position];
                        
                        // Aplicar volumen y velocidad
                        // Optimización: Usar punto fijo si fuera necesario, pero ESP32 tiene FPU rápida
                        float gain = voice.volume * voice.velocity;
                        int32_t processedSample = (int32_t)(sample * gain);

                        leftAccumulator += processedSample;
                        rightAccumulator += processedSample; // Mono a Stereo

                        // Avanzar posición
                        voice.position++;
                        if (voice.position >= voice.length) {
                            voice.active = false; // Fin del sample
                        }
                    }
                }

                // Soft Clipping / Limiting
                // Evitar wrap-around distorsion
                leftAccumulator = std::max(-32767, std::min(32767, leftAccumulator));
                rightAccumulator = std::max(-32767, std::min(32767, rightAccumulator));
                
                if (leftAccumulator != 0 || rightAccumulator != 0) signalPresent = true;

                outputBuffer[i * 2] = (int16_t)leftAccumulator;
                outputBuffer[i * 2 + 1] = (int16_t)rightAccumulator;
            }

            xSemaphoreGive(mixerMutex);
        }
        
        // Debug print once per second if signal is flowing
        if (signalPresent && (millis() - lastDebugTime > 1000)) {
            Serial.println("[AUDIO] Signal flowing to I2S...");
            lastDebugTime = millis();
        }

        // Escribir al I2S (Bloqueante si el buffer DMA está lleno, lo cual regula la velocidad)
        size_t bytesWritten;
        i2s_write(I2S_NUM_0, outputBuffer, sizeof(outputBuffer), &bytesWritten, portMAX_DELAY);
        
        // Pequeño yield para evitar starvation si I2S es muy rápido
        // pero normalmente i2s_write regula el tiempo
    }
}

// ============================================================
// FUNCIONES PÚBLICAS
// ============================================================

void playTestTone() {
    Serial.println("[AUDIO] Playing synthetic test tone (440Hz)...");
    
    // Create a temporary synthetic voice in slot 0
    if (xSemaphoreTake(mixerMutex, 100) == pdTRUE) {
        AudioVoice& v = voices[0];
        // We can't use a data pointer since we generate it, 
        // but for this quick test we will hijack the loop slightly or just
        // inject directly into buffer.
        // Better: let's create a small static sine wave buffer
        static int16_t sineWave[1000];
        static bool sineInit = false;
        if (!sineInit) {
            for(int i=0; i<1000; i++) {
                sineWave[i] = (int16_t)(sin(2 * M_PI * i * 440.0 / 44100.0) * 10000);
            }
            sineInit = true;
        }
        
        v.data = sineWave;
        v.length = 1000;
        v.position = 0;
        v.volume = 1.0f;
        v.velocity = 1.0f;
        v.active = true;
        v.chokeGroup = 0;
        
        xSemaphoreGive(mixerMutex);
    }
}

bool begin() {
    if (initialized) return true;

    // Crear Mutex
    mixerMutex = xSemaphoreCreateMutex();
    if (!mixerMutex) {
        Serial.println("[AUDIO] Failed to create mutex");
        return false;
    }

    // Configurar I2S
    i2s_config_t config = {};
    config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    config.sample_rate = AUDIO_SAMPLE_RATE;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_I2S_MSB; // Try MSB (common for PCM5102)
    config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    config.dma_buf_count = 8;
    config.dma_buf_len = 256; 
    config.use_apll = true;
    config.tx_desc_auto_clear = true; 

    if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) {
        Serial.println("[AUDIO] I2S Install Failed");
        return false;
    }

    i2s_pin_config_t pins = {};
    pins.bck_io_num = I2S_BCLK_PIN;
    pins.ws_io_num = I2S_LRCK_PIN;
    pins.data_out_num = I2S_DOUT_PIN;
    pins.data_in_num = I2S_PIN_NO_CHANGE;

    if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) {
        Serial.println("[AUDIO] I2S Pin Config Failed");
        return false;
    }
    
    // Iniciar tarea de mezcla en Core 1 (Audio Core)
    xTaskCreatePinnedToCore(
        mixerTask,
        "AudioMixer",
        4096,     // Stack size
        nullptr,
        20,       // Priority
        &mixerTaskHandle,
        1         // Core 1
    );

    initialized = true;
    Serial.println("[AUDIO] Polyphonic Engine Initialized");
    
    // Auto-play test tone on boot
    playTestTone();
    
    return true;
}

void play(const char* sampleName, uint8_t velocity, uint8_t volume, uint8_t chokeGroup) {
    if (!initialized || !sampleName) return;

    // Obtener datos del sample desde SampleManager
    const Sample* s = SampleManager::getSample(sampleName);
    if (!s || !s->data || s->frames == 0) return;

    if (xSemaphoreTake(mixerMutex, 10) == pdTRUE) { // Esperar máx 10 ticks
        
        // 1. CHOKE GROUP LOGIC
        // Si el sonido pertenece a un grupo, detener otros de ese grupo
        if (chokeGroup > 0) {
            for (int i = 0; i < AUDIO_MAX_VOICES; i++) {
                if (voices[i].active && voices[i].chokeGroup == chokeGroup) {
                    voices[i].active = false; // Hard cut (TODO: Fade out rápido)
                }
            }
        }

        // 2. BUSCAR VOZ LIBRE
        int voiceIndex = -1;
        
        // Primero buscar una inactiva
        for (int i = 0; i < AUDIO_MAX_VOICES; i++) {
            if (!voices[i].active) {
                voiceIndex = i;
                break;
            }
        }

        // Si no hay libres, robar la más vieja (o la más avanzada en reproducción)
        // Estrategia simple: robar la que tenga mayor posición (más cerca del final)
        if (voiceIndex == -1) {
            uint32_t maxPos = 0;
            for (int i = 0; i < AUDIO_MAX_VOICES; i++) {
                if (voices[i].position > maxPos) {
                    maxPos = voices[i].position;
                    voiceIndex = i;
                }
            }
        }

        // 3. CONFIGURAR VOZ
        if (voiceIndex != -1) {
            AudioVoice& v = voices[voiceIndex];
            v.data = s->data;
            v.length = s->frames;
            v.position = 0;
            v.volume = (float)volume / 127.0f;
            v.velocity = (float)velocity / 127.0f;
            v.chokeGroup = chokeGroup; // Asignar grupo para futuros chokes
            v.active = true;
        }

        xSemaphoreGive(mixerMutex);
    }
}

void choke(uint8_t chokeGroup) {
    if (!initialized || chokeGroup == 0) return;

    if (xSemaphoreTake(mixerMutex, 10) == pdTRUE) {
        for (int i = 0; i < AUDIO_MAX_VOICES; i++) {
            if (voices[i].active && voices[i].chokeGroup == chokeGroup) {
                voices[i].active = false;
            }
        }
        xSemaphoreGive(mixerMutex);
    }
}

void stopAll() {
    if (!initialized) return;
    
    if (xSemaphoreTake(mixerMutex, 10) == pdTRUE) {
        for (int i = 0; i < AUDIO_MAX_VOICES; i++) {
            voices[i].active = false;
        }
        xSemaphoreGive(mixerMutex);
    }
}

} // namespace AudioEngine