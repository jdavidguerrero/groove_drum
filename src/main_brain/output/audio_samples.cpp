#include "audio_samples.h"
#include <esp_heap_caps.h>
#include <algorithm>
#include <vector>
#include <edrum_config.h>
#include <cstring>
#include "pad_config.h"

// Use SD card for samples (requires pull-up resistors on GPIO45/46)
#define USE_EMBEDDED_SAMPLES 0

namespace {

struct LoadedSample {
    String name;
    Sample sample;
};

std::vector<LoadedSample> loaded;

uint32_t readLE32(File& f) {
    uint8_t b[4];
    if (f.read(b, 4) != 4) return 0;
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

uint16_t readLE16(File& f) {
    uint8_t b[2];
    if (f.read(b, 2) != 2) return 0;
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
}

bool loadWavToPSRAM(const char* path, Sample& out) {
    if (!SD.exists(path)) {
        Serial.printf("[SAMPLE] File not found: %s\n", path);
        return false;
    }

    File f = SD.open(path, FILE_READ);
    if (!f) {
        Serial.printf("[SAMPLE] Cannot open %s\n", path);
        return false;
    }

    char riff[4];
    if (f.read((uint8_t*)riff, 4) != 4 || strncmp(riff, "RIFF", 4) != 0) {
        Serial.printf("[SAMPLE] %s missing RIFF\n", path);
        f.close();
        return false;
    }
    f.seek(8); // skip RIFF size
    char wave[4];
    if (f.read((uint8_t*)wave, 4) != 4 || strncmp(wave, "WAVE", 4) != 0) {
        Serial.printf("[SAMPLE] %s not WAVE\n", path);
        f.close();
        return false;
    }

    uint16_t audioFormat = 0;
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataSize = 0;
    uint32_t dataPos = 0;

    // Parse chunks
    while (f.available()) {
        char chunkId[4];
        if (f.read((uint8_t*)chunkId, 4) != 4) break;
        uint32_t chunkSize = readLE32(f);

        if (strncmp(chunkId, "fmt ", 4) == 0) {
            audioFormat = readLE16(f);
            numChannels = readLE16(f);
            sampleRate = readLE32(f);
            f.seek(f.position() + 6); // skip byteRate + blockAlign
            bitsPerSample = readLE16(f);
            if (chunkSize > 16) f.seek(f.position() + (chunkSize - 16));
        } else if (strncmp(chunkId, "data", 4) == 0) {
            dataSize = chunkSize;
            dataPos = f.position();
            f.seek(f.position() + chunkSize);
        } else {
            f.seek(f.position() + chunkSize);
        }
    }

    if (audioFormat != 1 || bitsPerSample != 16 || dataSize == 0) {
        Serial.printf("[SAMPLE] %s unsupported format (fmt=%u bits=%u data=%u)\n",
                      path, audioFormat, bitsPerSample, dataSize);
        f.close();
        return false;
    }

    f.seek(dataPos);
    size_t bytes = dataSize;
    int16_t* buf = (int16_t*)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
    if (!buf) {
        buf = (int16_t*)heap_caps_malloc(bytes, MALLOC_CAP_DEFAULT);
    }
    if (!buf) {
        Serial.printf("[SAMPLE] No memory for %s\n", path);
        f.close();
        return false;
    }

    size_t read = f.read((uint8_t*)buf, bytes);
    f.close();
    
    if (read != bytes) {
        Serial.printf("[SAMPLE] Short read %s (%u/%u)\n", path, (unsigned)read, (unsigned)bytes);
        free(buf);
        return false;
    }

    out.data = buf;
    out.frames = dataSize / (numChannels * sizeof(int16_t));
    out.sampleRate = sampleRate;
    out.channels = numChannels;

    Serial.printf("[SAMPLE] Loaded %s: %lu frames, %u ch, %lu Hz\n",
                  path, (unsigned long)out.frames, out.channels, (unsigned long)out.sampleRate);
    return true;
}

} // namespace

namespace SampleManager {

// Helper: Check if sample is already loaded
bool isLoaded(const char* name) {
    for (auto& it : loaded) {
        if (it.name.equals(name)) return true;
    }
    return false;
}

// Helper: Load a single sample file
bool loadSample(const char* path) {
    if (isLoaded(path)) return true; // Already loaded

    Sample s;
    if (loadWavToPSRAM(path, s)) {
        loaded.push_back({String(path), s});
        return true;
    }
    return false;
}

size_t beginAndLoadDefaults() {
#if USE_EMBEDDED_SAMPLES
    Serial.println("[SAMPLE] Embedded samples enabled");
    return 0;
#else
    Serial.printf("[SYSTEM] Free Heap: %d, Free PSRAM: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());

    // Asegurar estado inicial de pines SPI
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(50);

    // Inicializar SPI explícitamente
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    // Intentar montar SD con frecuencia segura (4MHz)
    if (!SD.begin(SD_CS_PIN, SPI, 4000000, "/sd")) {
        Serial.println("[SD] init failed! Retrying with lower frequency...");
        delay(100);
        if (!SD.begin(SD_CS_PIN, SPI, 1000000, "/sd")) {
             Serial.println("[SD] init failed again. Check wiring/card.");
             return 0;
        }
    }
    Serial.println("[SD] Card initialized.");
    Serial.printf("[SYSTEM] Post-SD Heap: %d, Free PSRAM: %d\n", ESP.getFreeHeap(), ESP.getFreePsram());

    loaded.clear();
    
    // Load samples requested by Pad Configuration
    Serial.println("[SAMPLE] Loading samples defined in PadConfig...");
    
    for (int i = 0; i < NUM_PADS; ++i) {
        PadConfig& cfg = PadConfigManager::getConfig(i);
        
        // Ensure sample name is not empty
        if (strlen(cfg.sampleName) > 0) {
            Serial.printf("[SAMPLE] Pad %d needs: %s\n", i, cfg.sampleName);
            if (!loadSample(cfg.sampleName)) {
                Serial.printf("[SAMPLE] Failed to load %s for Pad %d\n", cfg.sampleName, i);
            }
        }
        
        // Also load rim samples if dual zone enabled
        if (cfg.dualZoneEnabled && strlen(cfg.rimSampleName) > 0) {
            if (!loadSample(cfg.rimSampleName)) {
                Serial.printf("[SAMPLE] Failed to load rim %s for Pad %d\n", cfg.rimSampleName, i);
            }
        }
    }
    
    Serial.printf("[SAMPLE] Total loaded unique samples: %u\n", (unsigned)loaded.size());
    return loaded.size();
#endif
}


const Sample* getSample(const char* name) {
    if (!name) return nullptr;
    for (auto& it : loaded) {
        if (it.name.equals(name)) return &it.sample;
    }
    return nullptr;
}

size_t loadedCount() {
    return loaded.size();
}

} // namespace SampleManager
