#include "audio_samples.h"
#include "audio_engine.h"
#include <driver/i2s.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <vector>
#include <edrum_config.h>
#include <cstring>

namespace {

struct LoadedSample {
    String name;
    Sample sample;
};

std::vector<LoadedSample> loaded;

// Rutas por defecto (puedes cambiar los nombres de archivo en la SD)
const char* DEFAULT_SAMPLE_PATHS[NUM_PADS] = {
    SAMPLE_PATH_KICK,
    SAMPLE_PATH_SNARE,
    SAMPLE_PATH_HIHAT,
    SAMPLE_PATH_TOM
};

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
    File f = SD.open(path, FILE_READ);
    if (!f) {
        Serial.printf("[SAMPLE] Cannot open %s\n", path);
        return false;
    }

    char riff[4];
    if (f.read((uint8_t*)riff, 4) != 4 || strncmp(riff, "RIFF", 4) != 0) {
        Serial.printf("[SAMPLE] %s missing RIFF\n", path);
        return false;
    }
    f.seek(8); // skip RIFF size
    char wave[4];
    if (f.read((uint8_t*)wave, 4) != 4 || strncmp(wave, "WAVE", 4) != 0) {
        Serial.printf("[SAMPLE] %s not WAVE\n", path);
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
        return false;
    }

    size_t read = f.read((uint8_t*)buf, bytes);
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

size_t beginAndLoadDefaults() {
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, SPI, 25000000)) {
        Serial.println("[SD] init failed");
        return 0;
    }

    loaded.clear();
    for (int i = 0; i < NUM_PADS; ++i) {
        Sample s;
        const char* path = DEFAULT_SAMPLE_PATHS[i];
        if (loadWavToPSRAM(path, s)) {
            loaded.push_back({String(path), s});
        }
    }
    Serial.printf("[SAMPLE] Loaded %u samples\n", (unsigned)loaded.size());
    return loaded.size();
}

const Sample* getSample(const char* name) {
    if (!name) return nullptr;
    for (auto& it : loaded) {
        if (it.name == name) return &it.sample;
    }
    return nullptr;
}

size_t loadedCount() {
    return loaded.size();
}

bool playSample(const char* name, uint8_t velocity, uint8_t volume) {
    const Sample* s = getSample(name);
    if (!s || !s->data || s->frames == 0) {
        return false;
    }

    // Re-sample: si sampleRate != I2S rate, ignoramos y reproducimos tal cual (pequeño pitch drift).
    float scale = (velocity / 127.0f) * (volume / 127.0f);
    if (scale <= 0.0f) scale = 0.01f;

    const uint32_t frames = s->frames;
    const uint8_t ch = s->channels;
    const int16_t* src = s->data;

    const size_t chunkFrames = 256;
    int16_t outBuf[chunkFrames * 2]; // stereo interleaved

    uint32_t pos = 0;
    while (pos < frames) {
        uint32_t n = std::min<uint32_t>(chunkFrames, frames - pos);
        for (uint32_t i = 0; i < n; ++i) {
            int32_t sample = 0;
            if (ch == 1) {
                sample = src[pos + i];
            } else {
                // stereo: average L/R para mono
                sample = (int32_t)src[(pos + i) * 2] + (int32_t)src[(pos + i) * 2 + 1];
                sample /= 2;
            }
            float scaled = (float)sample * scale;
            int16_t s16 = (int16_t)std::max<float>(-32768.f, std::min<float>(32767.f, scaled));
            outBuf[2 * i] = s16;
            outBuf[2 * i + 1] = s16;
        }
        size_t bytesToWrite = n * 2 * sizeof(int16_t);
        size_t written = 0;
        i2s_write(I2S_NUM_0, outBuf, bytesToWrite, &written, pdMS_TO_TICKS(100));
        pos += n;
    }
    return true;
}

} // namespace SampleManager
