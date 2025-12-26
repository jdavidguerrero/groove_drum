#include "scr_sample_browser.h"
#include <cstdio>
#include <cstring>

namespace ui {

SampleBrowserScreen::SampleBrowserScreen()
    : header(nullptr), sampleList(nullptr), countLabel(nullptr),
      totalCount(0), startIndex(0), visibleCount(0), selectedIdx(0) {
    memset(items, 0, sizeof(items));
    memset(currentSamples, 0, sizeof(currentSamples));
    buildLayout();
}

void SampleBrowserScreen::buildLayout() {
    const auto& colors = UITheme::palette();

    // Circular border decoration
    lv_obj_t* border = lv_arc_create(root());
    lv_obj_set_size(border, 230, 230);
    lv_obj_center(border);
    lv_arc_set_bg_angles(border, 0, 360);
    lv_arc_set_value(border, 0);
    lv_obj_set_style_arc_width(border, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_color(border, colors.accent, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(border, LV_OPA_30, LV_PART_MAIN);
    lv_obj_clear_flag(border, LV_OBJ_FLAG_CLICKABLE);

    // Header
    header = lv_label_create(root());
    lv_label_set_text(header, "SAMPLES");
    lv_obj_add_style(header, &UITheme::labelSmall(), LV_PART_MAIN);
    lv_obj_set_style_text_color(header, colors.accent, LV_PART_MAIN);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 20);

    // Sample count indicator (e.g., "3/12")
    countLabel = lv_label_create(root());
    lv_label_set_text(countLabel, "0/0");
    lv_obj_add_style(countLabel, &UITheme::labelSmall(), LV_PART_MAIN);
    lv_obj_set_style_text_opa(countLabel, LV_OPA_70, LV_PART_MAIN);
    lv_obj_align(countLabel, LV_ALIGN_TOP_MID, 0, 40);

    // Create 4 item slots - centered for round display
    int startY = 65;
    int lineHeight = 32;

    for (int i = 0; i < 4; i++) {
        items[i] = lv_label_create(root());
        lv_label_set_text(items[i], "");
        lv_obj_add_style(items[i], &UITheme::labelSmall(), LV_PART_MAIN);
        lv_obj_set_width(items[i], 180);
        lv_obj_align(items[i], LV_ALIGN_TOP_MID, 0, startY + i * lineHeight);
        lv_obj_set_style_text_align(items[i], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_label_set_long_mode(items[i], LV_LABEL_LONG_DOT);
    }

    // Bottom hint - compact
    lv_obj_t* hint = lv_label_create(root());
    lv_label_set_text(hint, "PRESS:sel CLICK:back");
    lv_obj_add_style(hint, &UITheme::labelSmall(), LV_PART_MAIN);
    lv_obj_set_style_text_opa(hint, LV_OPA_50, LV_PART_MAIN);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void SampleBrowserScreen::onShow() {
    refreshList();
}

void SampleBrowserScreen::onHide() {
}

void SampleBrowserScreen::onEncoderLeft(int32_t delta, bool pressed) {
    LV_UNUSED(delta);
    LV_UNUSED(pressed);
}

void SampleBrowserScreen::updateSampleList(const SampleListMsg& samples) {
    totalCount = samples.totalCount;
    startIndex = samples.startIndex;
    visibleCount = samples.count;

    for (uint8_t i = 0; i < 4; i++) {
        if (i < samples.count) {
            currentSamples[i] = samples.samples[i];
            if (samples.samples[i].selected) {
                selectedIdx = samples.samples[i].index;
            }
        } else {
            memset(&currentSamples[i], 0, sizeof(SampleEntryMsg));
        }
    }

    refreshList();
}

void SampleBrowserScreen::setSelectedIndex(uint8_t index) {
    selectedIdx = index;
    refreshList();
}

void SampleBrowserScreen::refreshList() {
    const auto& colors = UITheme::palette();

    // Update count label
    char countBuf[12];
    snprintf(countBuf, sizeof(countBuf), "%d/%d", selectedIdx + 1, totalCount);
    lv_label_set_text(countLabel, countBuf);

    // Update item labels
    for (int i = 0; i < 4; i++) {
        if (i < visibleCount) {
            bool isSelected = (currentSamples[i].selected != 0);

            // Extract just the filename without path
            const char* name = currentSamples[i].displayName;

            // Truncate if needed
            char displayText[20];
            if (strlen(name) > 18) {
                strncpy(displayText, name, 15);
                displayText[15] = '\0';
                strcat(displayText, "...");
            } else {
                strncpy(displayText, name, 19);
                displayText[19] = '\0';
            }

            lv_label_set_text(items[i], displayText);

            // Style based on selection
            if (isSelected) {
                lv_obj_set_style_text_color(items[i], colors.value, LV_PART_MAIN);
                lv_obj_set_style_bg_color(items[i], colors.accent, LV_PART_MAIN);
                lv_obj_set_style_bg_opa(items[i], LV_OPA_40, LV_PART_MAIN);
                lv_obj_set_style_pad_all(items[i], 4, LV_PART_MAIN);
                lv_obj_set_style_radius(items[i], 4, LV_PART_MAIN);
            } else {
                lv_obj_set_style_text_color(items[i], colors.primary, LV_PART_MAIN);
                lv_obj_set_style_bg_opa(items[i], LV_OPA_TRANSP, LV_PART_MAIN);
                lv_obj_set_style_pad_all(items[i], 0, LV_PART_MAIN);
            }

            lv_obj_clear_flag(items[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(items[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

}  // namespace ui
