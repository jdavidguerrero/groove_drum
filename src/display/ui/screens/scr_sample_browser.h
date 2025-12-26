#pragma once

#include "../ui_manager.h"
#include "../ui_theme.h"
#include <gui_protocol.h>

namespace ui {

class SampleBrowserScreen : public UIScreen {
public:
    SampleBrowserScreen();

    void onShow() override;
    void onHide() override;
    void onEncoderLeft(int32_t delta, bool pressed) override;

    // Update with sample list data
    void updateSampleList(const SampleListMsg& samples);

    // Set currently selected index (for highlight)
    void setSelectedIndex(uint8_t index);

private:
    void buildLayout();
    void refreshList();

    lv_obj_t* header;
    lv_obj_t* sampleList;
    lv_obj_t* countLabel;
    lv_obj_t* items[4];  // Up to 4 visible items

    uint8_t totalCount;
    uint8_t startIndex;
    uint8_t visibleCount;
    uint8_t selectedIdx;
    SampleEntryMsg currentSamples[4];
};

}  // namespace ui
