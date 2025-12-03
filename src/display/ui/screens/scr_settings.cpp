#include "scr_settings.h"

namespace ui {

SettingsScreen::SettingsScreen() : roller(nullptr) {
    buildLayout();
}

void SettingsScreen::buildLayout() {
    roller = lv_roller_create(root());
    lv_obj_set_size(roller, 200, 160);  // Wider and taller
    lv_obj_center(roller);
    lv_roller_set_visible_row_count(roller, 4);  // Show 4 rows instead of 3
    lv_roller_set_options(roller, "MIDI CH:10\nVEL CURVE:LOG\nSENS:80\nUSB:DEVICE\nCLICK:-12dB", LV_ROLLER_MODE_NORMAL);
    lv_obj_add_style(roller, &UITheme::labelSmall(), LV_PART_MAIN);  // Smaller text
    lv_obj_set_style_bg_color(roller, UITheme::palette().background, LV_PART_MAIN);
    lv_obj_set_style_border_color(roller, UITheme::palette().accent, LV_PART_MAIN);
    lv_obj_set_style_border_width(roller, 2, LV_PART_MAIN);  // Thicker border for visibility
}

void SettingsScreen::setOptions(const char *options) {
    lv_roller_set_options(roller, options, LV_ROLLER_MODE_NORMAL);
}

void SettingsScreen::setSelected(uint16_t index) {
    lv_roller_set_selected(roller, index, LV_ANIM_ON);
}

void SettingsScreen::onEncoderLeft(int32_t delta, bool pressed) {
    LV_UNUSED(pressed);
    uint16_t cur = lv_roller_get_selected(roller);
    uint16_t count = lv_roller_get_option_cnt(roller);

    if (delta < 0 && cur > 0) {
        lv_roller_set_selected(roller, cur - 1, LV_ANIM_ON);
    } else if (delta > 0 && cur + 1 < count) {
        lv_roller_set_selected(roller, cur + 1, LV_ANIM_ON);
    }
}

void SettingsScreen::onEncoderRight(int32_t delta, bool pressed) {
    LV_UNUSED(pressed);
    // Hook: adjust value of selected item
    LV_UNUSED(delta);
}

}  // namespace ui
