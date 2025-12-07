#include "audio_engine.h"
#include <driver/i2s.h>
#include <math.h>
#include <edrum_config.h>
#include <algorithm>

namespace AudioEngine {

static bool initialized = false;
static uint32_t gSampleRate = 44100;
static uint8_t gBitsPerSample = 16;

bool begin(uint32_t sampleRate, uint8_t bitsPerSample) {
    if (initialized) return true;

    gSampleRate = sampleRate;
    gBitsPerSample = bitsPerSample;

    i2s_config_t config = {};
    config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    config.sample_rate = (int)gSampleRate;
    config.bits_per_sample = (i2s_bits_per_sample_t)gBitsPerSample;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_I2S;
    config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    config.dma_buf_count = 8;      // número de buffers DMA
    config.dma_buf_len = 256;      // muestras por buffer por canal
    config.use_apll = false;
    config.tx_desc_auto_clear = true;
    config.mclk_multiple = I2S_MCLK_MULTIPLE_256;
    config.bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT;

    if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) {
        Serial.println("[AUDIO] i2s_driver_install failed");
        return false;
    }

    i2s_pin_config_t pins = {};
    pins.bck_io_num = I2S_BCLK_PIN;
    pins.ws_io_num = I2S_LRCK_PIN;
    pins.data_out_num = I2S_DOUT_PIN;
    pins.data_in_num = I2S_PIN_NO_CHANGE;

    if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) {
        Serial.println("[AUDIO] i2s_set_pin failed");
        return false;
    }

    if (i2s_set_clk(I2S_NUM_0, gSampleRate, (i2s_bits_per_sample_t)gBitsPerSample, I2S_CHANNEL_STEREO) != ESP_OK) {
        Serial.println("[AUDIO] i2s_set_clk failed");
        return false;
    }

    initialized = true;
    Serial.printf("[AUDIO] I2S initialized @ %lu Hz, %u-bit, DMA %d x %d\n",
                  (unsigned long)gSampleRate, gBitsPerSample,
                  config.dma_buf_count, config.dma_buf_len);
    Serial.printf("[AUDIO] Pins -> BCK:%d  LRCK:%d  DOUT:%d\n",
                  I2S_BCLK_PIN, I2S_LRCK_PIN, I2S_DOUT_PIN);
    return true;
}

// Genera un click corto tipo seno, bloqueante (para pruebas)
bool playClick(uint8_t velocity, uint8_t volume, uint16_t durationMs) {
    if (!initialized) return false;
    // Amplitud 16-bit, escala por velocity y volumen (0-127)
    uint16_t scale = (uint16_t)(velocity) * (uint16_t)(volume);
    int16_t amplitude = (int16_t)(scale * 2); // ~max 32258
    float freq = 880.0f;
    uint32_t totalFrames = (gSampleRate * durationMs) / 1000;
    const size_t chunk = 128;
    int16_t buffer[chunk * 2]; // stereo interleaved
    float phase = 0.0f;
    float step = 2.0f * (float)M_PI * freq / (float)gSampleRate;

    uint32_t framesDone = 0;
    while (framesDone < totalFrames) {
        size_t framesNow = std::min<uint32_t>(chunk, totalFrames - framesDone);
        for (size_t i = 0; i < framesNow; ++i) {
            float s = sinf(phase) * (float)amplitude;
            int16_t sample = (int16_t)s;
            buffer[2 * i] = sample;     // L
            buffer[2 * i + 1] = sample; // R
            phase += step;
        }
        size_t bytesToWrite = framesNow * 2 * sizeof(int16_t);
        size_t written = 0;
        i2s_write(I2S_NUM_0, buffer, bytesToWrite, &written, pdMS_TO_TICKS(100));
        framesDone += framesNow;
    }
    return true;
}

} // namespace AudioEngine
