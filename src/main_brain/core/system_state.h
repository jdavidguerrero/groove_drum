#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdint.h>

namespace SystemState {

// High-level routing preference for pad hits.
enum class OutputMode : uint8_t {
    Audio = 0,
    Midi = 1
};

void setOutputMode(OutputMode mode);
OutputMode getOutputMode();
bool isMidiPreferred();

}  // namespace SystemState

#endif  // SYSTEM_STATE_H
