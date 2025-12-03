#include "scr_pad_edit.h"

#include <cstdio>

namespace ui {

PadEditScreen::PadEditScreen() : header(nullptr), paramLabel(nullptr), valueLabel(nullptr), valueArc(nullptr) {
    buildLayout();
}

void PadEditScreen::buildLayout() {
    const auto &colors = UITheme::palette();

    // THE DIAL - 270° progress arc wrapping the edge
    valueArc = lv_arc_create(root());
    lv_obj_set_size(valueArc, 232, 232);  // Almost full screen
    lv_obj_center(valueArc);
    lv_arc_set_bg_angles(valueArc, 135, 405);  // 270° sweep (bottom-left to bottom-right)
    lv_arc_set_range(valueArc, 0, 100);
    lv_arc_set_value(valueArc, 75);  // Example: 75%

    // Arc styling - thick and prominent
    lv_obj_set_style_arc_width(valueArc, 18, LV_PART_MAIN);
    lv_obj_set_style_arc_width(valueArc, 18, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(valueArc, colors.value, LV_PART_INDICATOR);  // Amber
    lv_obj_set_style_arc_opa(valueArc, LV_OPA_10, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(valueArc, LV_OPA_100, LV_PART_INDICATOR);

    lv_obj_clear_flag(valueArc, LV_OBJ_FLAG_CLICKABLE);

    // Center text - directly on root for proper centering
    // Header (pad name) - top
    header = lv_label_create(root());
    lv_label_set_text(header, "PAD 1");
    lv_obj_add_style(header, &UITheme::labelSmall(), LV_PART_MAIN);
    lv_obj_align(header, LV_ALIGN_CENTER, 0, -50);

    // Parameter name (e.g., "PITCH", "VOLUME") - center
    paramLabel = lv_label_create(root());
    lv_label_set_text(paramLabel, "PITCH");
    lv_obj_add_style(paramLabel, &UITheme::labelMedium(), LV_PART_MAIN);
    lv_obj_align(paramLabel, LV_ALIGN_CENTER, 0, -10);

    // Value display (large number) - below parameter
    valueLabel = lv_label_create(root());
    lv_label_set_text(valueLabel, "+12");
    lv_obj_add_style(valueLabel, &UITheme::labelLarge(), LV_PART_MAIN);
    lv_obj_set_style_text_color(valueLabel, colors.value, LV_PART_MAIN);  // Match arc color (Amber)
    lv_obj_align(valueLabel, LV_ALIGN_CENTER, 0, 25);
}

void PadEditScreen::setPadName(const char *name) {
    lv_label_set_text(header, name ? name : "PAD");
}

void PadEditScreen::setParameterName(const char *param) {
    lv_label_set_text(paramLabel, param ? param : "--");
}

void PadEditScreen::setValueText(const char *value) {
    lv_label_set_text(valueLabel, value ? value : "--");
}

void PadEditScreen::setValueNormalized(uint8_t percent) {
    lv_arc_set_value(valueArc, percent);
}

void PadEditScreen::onEncoderLeft(int32_t delta, bool pressed) {
    LV_UNUSED(delta);
    LV_UNUSED(pressed);
    // Hook: navigate parameter list
}

void PadEditScreen::onEncoderRight(int32_t delta, bool pressed) {
    LV_UNUSED(delta);
    LV_UNUSED(pressed);
    // Hook: adjust current parameter value
}

}  // namespace ui
