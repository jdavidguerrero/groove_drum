#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include <Arduino.h>
#include <driver/i2s.h>
#include <edrum_config.h>
#include "../output/audio_samples.h"
#include "../core/event_dispatcher.h"

struct AudioVoice {
    const Sample* sample = nullptr;
    uint32_t position = 0;
    float gain = 0.0f;
    bool active = false;
};

void AudioMixer_begin();
void AudioMixer_startVoice(const AudioRequest& request);
void AudioMixer_update();

#endif // AUDIO_MIXER_H
