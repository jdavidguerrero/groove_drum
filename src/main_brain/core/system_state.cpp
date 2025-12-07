#include "system_state.h"

namespace SystemState {

static OutputMode currentMode = OutputMode::Midi;  // default keeps previous behavior

void setOutputMode(OutputMode mode) {
    currentMode = mode;
}

OutputMode getOutputMode() {
    return currentMode;
}

bool isMidiPreferred() {
    return currentMode == OutputMode::Midi;
}

}  // namespace SystemState
