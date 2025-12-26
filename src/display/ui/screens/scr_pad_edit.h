#pragma once

#include "../ui_manager.h"
#include "../ui_theme.h"
#include <gui_protocol.h>
#include <cstdint>

namespace ui {

class PadEditScreen : public UIScreen {
public:
    PadEditScreen();

    void setPadName(const char *name, uint8_t padIndex);
    void setParameterName(const char *param);
    void setValueText(const char *value);
    void setValueNormalized(uint8_t percent);
    void setEditing(bool editing);
    void setHasChanges(bool hasChanges);

    // Update from menu state message
    void updateFromMenuState(const MenuStateMsg& state);

    void onEncoderLeft(int32_t delta, bool pressed) override;
    void onEncoderRight(int32_t delta, bool pressed) override;

private:
    void buildLayout();
    void updateEditIndicator();

    lv_obj_t *header;
    lv_obj_t *paramLabel;
    lv_obj_t *valueLabel;
    lv_obj_t *valueArc;
    lv_obj_t *editIndicator;
    lv_obj_t *changesIndicator;
    lv_obj_t *hintLabel;

    bool isEditing;
    bool hasUnsavedChanges;
    uint8_t currentOption;
};

}  // namespace ui
