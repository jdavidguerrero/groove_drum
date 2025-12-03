#pragma once

#include "../ui_manager.h"
#include "../ui_theme.h"
#include <array>
#include <cstdint>

namespace ui {

class PerformanceScreen : public UIScreen {
public:
    PerformanceScreen();

    void setKit(uint8_t kitNumber, const char *name);
    void setBpm(uint16_t bpm);
    void onPadHit(uint8_t padId, uint8_t velocity) override;

private:
    void buildLayout();
    void flashArc(uint8_t padId, uint8_t velocity);
    void updateWaveform(uint8_t padId, uint8_t velocity);

    lv_obj_t *card;
    lv_obj_t *lblKitNumber;
    lv_obj_t *lblKitName;
    lv_obj_t *lblBpm;
    std::array<lv_obj_t *, 4> padArcs;

    // Waveform visualization
    lv_obj_t *waveformChart;
    lv_chart_series_t *waveformSeries;
};

}  // namespace ui
