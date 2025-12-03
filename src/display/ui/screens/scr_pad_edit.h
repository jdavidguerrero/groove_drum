#pragma once

#include "../ui_manager.h"
#include "../ui_theme.h"
#include <cstdint>

namespace ui {

class PadEditScreen : public UIScreen {
public:
    PadEditScreen();

    void setPadName(const char *name);
    void setParameterName(const char *param);
    void setValueText(const char *value);
    void setValueNormalized(uint8_t percent);

    void onEncoderLeft(int32_t delta, bool pressed) override;
    void onEncoderRight(int32_t delta, bool pressed) override;

private:
    void buildLayout();

    lv_obj_t *header;
    lv_obj_t *paramLabel;
    lv_obj_t *valueLabel;
    lv_obj_t *valueArc;
};

}  // namespace ui
