#pragma once

#include "../ui_manager.h"
#include "../ui_theme.h"
#include <array>
#include <cstdint>

namespace ui {

class MixerScreen : public UIScreen {
public:
    MixerScreen();

    void setLevel(uint8_t padIndex, uint8_t level);
    void setSelected(uint8_t padIndex);

    void onEncoderLeft(int32_t delta, bool pressed) override;
    void onEncoderRight(int32_t delta, bool pressed) override;

private:
    void buildLayout();
    void updateSelectionColor();

    std::array<lv_obj_t *, 4> levelArcs;
    std::array<lv_obj_t *, 4> labels;
    uint8_t selected;
};

}  // namespace ui
