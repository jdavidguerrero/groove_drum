#include "audio_mixer.h"
#include <algorithm>

namespace {
constexpr size_t AUDIO_MIXER_CHUNK = 128;
constexpr size_t AUDIO_MIXER_VOICES = 4;
static AudioVoice voices[AUDIO_MIXER_VOICES];
static int16_t mixBuffer[AUDIO_MIXER_CHUNK * 2];

float computeGain(uint8_t velocity, uint8_t volume) {
    float vel = std::max(1u, (unsigned)velocity) / 127.0f;
    float vol = std::max(1u, (unsigned)volume) / 127.0f;
    return std::max(0.01f, vel * vol);
}

void mixVoices(int16_t* outBuf, size_t frames) {
    if (frames == 0) return;
    for (size_t i = 0; i < frames * 2; ++i) {
        outBuf[i] = 0;
    }

    for (size_t v = 0; v < AUDIO_MIXER_VOICES; ++v) {
        AudioVoice& voice = voices[v];
        if (!voice.active || !voice.sample || !voice.sample->data) continue;

        for (size_t i = 0; i < frames; ++i) {
            if (voice.position >= voice.sample->frames) {
                voice.active = false;
                break;
            }

            const int16_t* data = voice.sample->data;
            int16_t left = 0;
            int16_t right = 0;

            if (voice.sample->channels == 1) {
                int16_t val = data[voice.position];
                left = val;
                right = val;
            } else {
                uint32_t idx = voice.position * 2;
                left = data[idx];
                right = data[idx + 1];
            }

            int32_t mixL = outBuf[2 * i] + static_cast<int32_t>(left * voice.gain);
            int32_t mixR = outBuf[2 * i + 1] + static_cast<int32_t>(right * voice.gain);

            outBuf[2 * i] = static_cast<int16_t>(std::max(-32768, std::min(32767, mixL)));
            outBuf[2 * i + 1] = static_cast<int16_t>(std::max(-32768, std::min(32767, mixR)));

            voice.position++;
        }
    }
}

bool hasActiveVoices() {
    for (size_t v = 0; v < AUDIO_MIXER_VOICES; ++v) {
        if (voices[v].active) return true;
    }
    return false;
}

} // namespace

void AudioMixer_begin() {
    for (auto& voice : voices) {
        voice.active = false;
        voice.sample = nullptr;
        voice.position = 0;
        voice.gain = 0.0f;
    }
}

void AudioMixer_startVoice(const AudioRequest& request) {
    if (!request.sampleName[0]) {
        return;
    }
    const Sample* sample = SampleManager::getSample(request.sampleName);
    if (!sample || !sample->data || sample->frames == 0) {
        return;
    }

    AudioVoice* slot = nullptr;
    for (size_t i = 0; i < AUDIO_MIXER_VOICES; ++i) {
        if (!voices[i].active) {
            slot = &voices[i];
            break;
        }
    }
    if (!slot) {
        slot = &voices[0];
    }

    slot->sample = sample;
    slot->position = 0;
    slot->gain = computeGain(request.velocity, request.volume);
    slot->active = true;
}

void AudioMixer_update() {
    mixVoices(mixBuffer, AUDIO_MIXER_CHUNK);
    size_t written = 0;
    i2s_write(I2S_NUM_0, mixBuffer, sizeof(mixBuffer), &written, pdMS_TO_TICKS(20));

    if (!hasActiveVoices()) {
        vTaskDelay(1);
    }
}
