#include "scr_pad_edit.h"

#include <cstdio>
#include <cstring>

namespace ui {

// Pad colors - one per pad
static const lv_color_t PAD_COLORS[4] = {
    lv_color_hex(0xFF0000),  // PAD1 - Red
    lv_color_hex(0xFF8800),  // PAD2 - Orange
    lv_color_hex(0xFFFF00),  // PAD3 - Yellow
    lv_color_hex(0x0088FF)   // PAD4 - Blue
};

PadEditScreen::PadEditScreen()
    : header(nullptr), paramLabel(nullptr), valueLabel(nullptr), valueArc(nullptr),
      editIndicator(nullptr), changesIndicator(nullptr), hintLabel(nullptr),
      isEditing(false), hasUnsavedChanges(false), currentOption(0) {
    buildLayout();
}

void PadEditScreen::buildLayout() {
    const auto &colors = UITheme::palette();

    // THE DIAL - 270 deg progress arc wrapping the edge (thinner for round display)
    valueArc = lv_arc_create(root());
    lv_obj_set_size(valueArc, 230, 230);
    lv_obj_center(valueArc);
    lv_arc_set_bg_angles(valueArc, 135, 405);  // 270 deg sweep
    lv_arc_set_range(valueArc, 0, 100);
    lv_arc_set_value(valueArc, 50);

    // Arc styling - thinner for compact display
    lv_obj_set_style_arc_width(valueArc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(valueArc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(valueArc, colors.value, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(valueArc, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(valueArc, LV_OPA_100, LV_PART_INDICATOR);
    lv_obj_clear_flag(valueArc, LV_OBJ_FLAG_CLICKABLE);

    // Header (pad name) - top center, with color indicator
    header = lv_label_create(root());
    lv_label_set_text(header, "KICK");
    lv_obj_add_style(header, &UITheme::labelMedium(), LV_PART_MAIN);
    lv_obj_set_style_text_color(header, colors.accent, LV_PART_MAIN);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 25);

    // Unsaved changes indicator (small dot)
    changesIndicator = lv_obj_create(root());
    lv_obj_set_size(changesIndicator, 8, 8);
    lv_obj_set_style_radius(changesIndicator, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(changesIndicator, lv_color_hex(0xFF6600), LV_PART_MAIN);
    lv_obj_set_style_border_width(changesIndicator, 0, LV_PART_MAIN);
    lv_obj_align(changesIndicator, LV_ALIGN_TOP_RIGHT, -30, 28);
    lv_obj_add_flag(changesIndicator, LV_OBJ_FLAG_HIDDEN);

    // Parameter name - centered
    paramLabel = lv_label_create(root());
    lv_label_set_text(paramLabel, "THRESHOLD");
    lv_obj_add_style(paramLabel, &UITheme::labelSmall(), LV_PART_MAIN);
    lv_obj_set_style_text_color(paramLabel, colors.accent, LV_PART_MAIN);
    lv_obj_align(paramLabel, LV_ALIGN_CENTER, 0, -25);

    // Value display (large) - center
    valueLabel = lv_label_create(root());
    lv_label_set_text(valueLabel, "200");
    lv_obj_add_style(valueLabel, &UITheme::labelLarge(), LV_PART_MAIN);
    lv_obj_set_style_text_color(valueLabel, colors.value, LV_PART_MAIN);
    lv_obj_align(valueLabel, LV_ALIGN_CENTER, 0, 15);

    // Edit indicator (shows when in edit mode)
    editIndicator = lv_label_create(root());
    lv_label_set_text(editIndicator, "EDITING");
    lv_obj_add_style(editIndicator, &UITheme::labelSmall(), LV_PART_MAIN);
    lv_obj_set_style_text_color(editIndicator, colors.value, LV_PART_MAIN);
    lv_obj_align(editIndicator, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_flag(editIndicator, LV_OBJ_FLAG_HIDDEN);

    // Bottom hint - compact
    hintLabel = lv_label_create(root());
    lv_label_set_text(hintLabel, "ENC:nav PRESS:edit");
    lv_obj_add_style(hintLabel, &UITheme::labelSmall(), LV_PART_MAIN);
    lv_obj_set_style_text_opa(hintLabel, LV_OPA_50, LV_PART_MAIN);
    lv_obj_align(hintLabel, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void PadEditScreen::setPadName(const char *name, uint8_t padIndex) {
    if (!name) return;

    lv_label_set_text(header, name);

    // Set color based on pad index
    lv_color_t padColor = (padIndex < 4) ? PAD_COLORS[padIndex] : UITheme::palette().accent;
    lv_obj_set_style_text_color(header, padColor, LV_PART_MAIN);
}

void PadEditScreen::setParameterName(const char *param) {
    lv_label_set_text(paramLabel, param ? param : "--");
}

void PadEditScreen::setValueText(const char *value) {
    lv_label_set_text(valueLabel, value ? value : "--");
    lv_obj_align(valueLabel, LV_ALIGN_CENTER, 0, 15);
}

void PadEditScreen::setValueNormalized(uint8_t percent) {
    lv_arc_set_value(valueArc, percent);
}

void PadEditScreen::setEditing(bool editing) {
    isEditing = editing;
    updateEditIndicator();
}

void PadEditScreen::setHasChanges(bool hasChanges) {
    hasUnsavedChanges = hasChanges;
    if (hasChanges) {
        lv_obj_clear_flag(changesIndicator, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(changesIndicator, LV_OBJ_FLAG_HIDDEN);
    }
}

void PadEditScreen::updateEditIndicator() {
    if (isEditing) {
        lv_obj_clear_flag(editIndicator, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(hintLabel, "ENC:adjust PRESS:done");

        // Pulsing animation for arc when editing
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, valueArc);
        lv_anim_set_values(&a, LV_OPA_60, LV_OPA_100);
        lv_anim_set_time(&a, 400);
        lv_anim_set_playback_time(&a, 400);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
            lv_obj_set_style_arc_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(v), LV_PART_INDICATOR);
        });
        lv_anim_start(&a);
    } else {
        lv_obj_add_flag(editIndicator, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_arc_opa(valueArc, LV_OPA_100, LV_PART_INDICATOR);
        lv_label_set_text(hintLabel, "ENC:nav PRESS:edit");
        lv_anim_del(valueArc, nullptr);
    }
}

void PadEditScreen::updateFromMenuState(const MenuStateMsg& state) {
    setPadName(state.padName, state.selectedPad);
    setParameterName(state.optionName);
    setEditing(state.editing != 0);
    setHasChanges(state.hasChanges != 0);
    currentOption = state.selectedOption;

    // Update value display based on option
    if (state.selectedOption == MENU_OPT_SAMPLE) {
        // For sample, extract just filename for compact display
        const char* sampleName = state.sampleName;
        const char* lastSlash = strrchr(sampleName, '/');
        if (lastSlash) sampleName = lastSlash + 1;

        // Truncate if too long
        char truncated[16];
        strncpy(truncated, sampleName, 15);
        truncated[15] = '\0';
        // Remove .wav extension
        char* dot = strrchr(truncated, '.');
        if (dot) *dot = '\0';

        setValueText(truncated);
        setValueNormalized(50);
    } else {
        char valueBuf[8];
        snprintf(valueBuf, sizeof(valueBuf), "%d", state.currentValue);
        setValueText(valueBuf);

        // Normalize value for arc display
        uint8_t normalized = 50;
        switch (state.selectedOption) {
            case MENU_OPT_THRESHOLD:
                normalized = (state.currentValue - 50) * 100 / 950;
                break;
            case MENU_OPT_SENSITIVITY:
                normalized = (state.currentValue - 50) * 100 / 450;
                break;
            case MENU_OPT_MAX_PEAK:
                normalized = (state.currentValue - 500) * 100 / 3500;
                break;
            default:
                normalized = 50;
                break;
        }
        if (normalized > 100) normalized = 100;
        setValueNormalized(normalized);
    }
}

void PadEditScreen::onEncoderLeft(int32_t delta, bool pressed) {
    LV_UNUSED(delta);
    LV_UNUSED(pressed);
}

void PadEditScreen::onEncoderRight(int32_t delta, bool pressed) {
    LV_UNUSED(delta);
    LV_UNUSED(pressed);
}

}  // namespace ui
