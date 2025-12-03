#pragma once

#include "../ui_manager.h"
#include "../ui_theme.h"
#include <cstdint>

namespace ui {

class SettingsScreen : public UIScreen {
public:
    SettingsScreen();

    void setOptions(const char *options);
    void setSelected(uint16_t index);

    void onEncoderLeft(int32_t delta, bool pressed) override;
    void onEncoderRight(int32_t delta, bool pressed) override;

private:
    void buildLayout();

    lv_obj_t *roller;
};

}  // namespace ui
