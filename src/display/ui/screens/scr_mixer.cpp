#include "scr_mixer.h"

#include <cstdio>

namespace ui {

MixerScreen::MixerScreen() : levelArcs{nullptr, nullptr, nullptr, nullptr}, labels{nullptr, nullptr, nullptr, nullptr}, selected(0) {
    buildLayout();
}

void MixerScreen::buildLayout() {
    static const char *padNames[4] = {"KICK", "SNARE", "HH", "TOM"};

    // Neon colors for each segment - THE MIXER REACTOR
    lv_color_t segmentColors[4] = {
        lv_color_hex(0xFF00FF),  // Magenta - KICK (top-right)
        lv_color_hex(0x00FFFF),  // Cyan - SNARE (top-left)
        lv_color_hex(0xFFFF00),  // Yellow - HH (bottom-left)
        lv_color_hex(0xFFFFFF)   // White - TOM (bottom-right)
    };

    // Segment angles - 4 quadrants of 90° each
    uint16_t startAngles[4] = {0, 90, 180, 270};  // 4 quadrants

    // Label positions (center of each quadrant)
    lv_coord_t labelOffsets[4][2] = {
        {50, -50},   // KICK: top-right
        {-50, -50},  // SNARE: top-left
        {-50, 50},   // HH: bottom-left
        {50, 50}     // TOM: bottom-right
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *arc = lv_arc_create(root());
        lv_obj_set_size(arc, 220, 220);  // Same size for all segments
        lv_obj_center(arc);

        // 90° segments covering full circle
        lv_arc_set_bg_angles(arc, startAngles[i], startAngles[i] + 90);
        lv_arc_set_range(arc, 0, 127);
        lv_arc_set_value(arc, 90);  // Initial value
        lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);

        // Segment styling - thick and prominent
        lv_obj_set_style_arc_width(arc, 14, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc, segmentColors[i], LV_PART_MAIN);
        lv_obj_set_style_arc_color(arc, segmentColors[i], LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(arc, LV_OPA_20, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(arc, LV_OPA_80, LV_PART_INDICATOR);

        lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
        levelArcs[i] = arc;

        // Label in center of each quadrant
        lv_obj_t *label = lv_label_create(root());
        lv_label_set_text(label, padNames[i]);
        lv_obj_add_style(label, &UITheme::labelSmall(), LV_PART_MAIN);
        lv_obj_set_style_text_color(label, segmentColors[i], LV_PART_MAIN);
        lv_obj_align(label, LV_ALIGN_CENTER, labelOffsets[i][0], labelOffsets[i][1]);
        labels[i] = label;
    }

    updateSelectionColor();
}

void MixerScreen::setLevel(uint8_t padIndex, uint8_t level) {
    if (padIndex >= levelArcs.size() || levelArcs[padIndex] == nullptr) {
        return;
    }
    lv_arc_set_value(levelArcs[padIndex], level);
}

void MixerScreen::setSelected(uint8_t padIndex) {
    if (padIndex >= levelArcs.size()) {
        return;
    }
    selected = padIndex;
    updateSelectionColor();
}

void MixerScreen::updateSelectionColor() {
    // Neon colors per segment (same as in buildLayout)
    lv_color_t segmentColors[4] = {
        lv_color_hex(0xFF00FF),  // Magenta - KICK
        lv_color_hex(0x00FFFF),  // Cyan - SNARE
        lv_color_hex(0xFFFF00),  // Yellow - HH
        lv_color_hex(0xFFFFFF)   // White - TOM
    };

    for (size_t i = 0; i < levelArcs.size(); i++) {
        // Selected segment: full brightness, others dimmed
        lv_opa_t opa = (i == selected) ? LV_OPA_100 : LV_OPA_60;
        lv_obj_set_style_arc_opa(levelArcs[i], opa, LV_PART_INDICATOR);

        // Keep original neon color
        lv_obj_set_style_arc_color(levelArcs[i], segmentColors[i], LV_PART_INDICATOR);
    }
}

void MixerScreen::onEncoderLeft(int32_t delta, bool pressed) {
    LV_UNUSED(pressed);
    if (delta > 0 && selected < levelArcs.size() - 1) {
        setSelected(selected + 1);
    } else if (delta < 0 && selected > 0) {
        setSelected(selected - 1);
    }
}

void MixerScreen::onEncoderRight(int32_t delta, bool pressed) {
    LV_UNUSED(pressed);
    int newLevel = static_cast<int>(lv_arc_get_value(levelArcs[selected])) + static_cast<int>(delta);
    if (newLevel < 0) newLevel = 0;
    if (newLevel > 127) newLevel = 127;
    lv_arc_set_value(levelArcs[selected], newLevel);
}

}  // namespace ui
