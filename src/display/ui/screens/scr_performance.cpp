#include "scr_performance.h"

#include <cstdio>

namespace ui {

PerformanceScreen::PerformanceScreen()
    : card(nullptr), lblKitNumber(nullptr), lblKitName(nullptr), lblBpm(nullptr),
      padArcs{nullptr, nullptr, nullptr, nullptr}, waveformChart(nullptr), waveformSeries(nullptr) {
    buildLayout();
}

void PerformanceScreen::buildLayout() {
    const auto &colors = UITheme::palette();

    lv_obj_center(root());

    // Pad arcs (orbital segments) - THE REACTOR
    uint16_t startAngles[4] = {315, 45, 135, 225};  // Aligned to cardinal directions
    for (int i = 0; i < 4; i++) {
        lv_obj_t *arc = lv_arc_create(root());
        lv_obj_set_size(arc, 228, 228);  // Larger, almost edge-to-edge
        lv_obj_center(arc);

        // Configure arc to show only the segment (not full circle)
        lv_arc_set_bg_angles(arc, startAngles[i], startAngles[i] + 70);  // Wider segments
        lv_arc_set_rotation(arc, 0);
        lv_arc_set_range(arc, 0, 127);
        lv_arc_set_value(arc, 64);  // Initial value to show the arc
        lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);  // Normal mode

        // Arc styling - make background match the segment shape
        lv_obj_set_style_arc_width(arc, 16, LV_PART_MAIN);  // Extra thick
        lv_obj_set_style_arc_width(arc, 16, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(arc, colors.accent, LV_PART_MAIN);  // Background same color
        lv_obj_set_style_arc_color(arc, colors.accent, LV_PART_INDICATOR);
        lv_obj_set_style_arc_opa(arc, LV_OPA_20, LV_PART_MAIN);  // Dimmed background
        lv_obj_set_style_arc_opa(arc, LV_OPA_10, LV_PART_INDICATOR);  // Start dimmed

        lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
        padArcs[i] = arc;
    }

    // Center text - top position
    // Kit number (medium - was large)
    lblKitNumber = lv_label_create(root());
    lv_label_set_text(lblKitNumber, "01");
    lv_obj_add_style(lblKitNumber, &UITheme::labelMedium(), LV_PART_MAIN);
    lv_obj_align(lblKitNumber, LV_ALIGN_TOP_MID, 0, 30);

    // Kit name (small - was medium)
    lblKitName = lv_label_create(root());
    lv_label_set_text(lblKitName, "INIT KIT");
    lv_obj_add_style(lblKitName, &UITheme::labelSmall(), LV_PART_MAIN);
    lv_obj_align(lblKitName, LV_ALIGN_TOP_MID, 0, 55);

    // BPM (small) - bottom position (raised higher)
    lblBpm = lv_label_create(root());
    lv_label_set_text(lblBpm, "120");
    lv_obj_add_style(lblBpm, &UITheme::labelSmall(), LV_PART_MAIN);
    lv_obj_set_style_text_color(lblBpm, colors.accent, LV_PART_MAIN);  // Cyan accent
    lv_obj_align(lblBpm, LV_ALIGN_BOTTOM_MID, 0, -40);  // Raised from -20 to -40

    // Waveform chart - CENTER between arcs
    waveformChart = lv_chart_create(root());
    lv_obj_set_size(waveformChart, 180, 60);  // Wide and short
    lv_obj_align(waveformChart, LV_ALIGN_CENTER, 0, 0);  // Dead center
    lv_chart_set_type(waveformChart, LV_CHART_TYPE_LINE);
    lv_chart_set_range(waveformChart, LV_CHART_AXIS_PRIMARY_Y, -100, 100);
    lv_chart_set_point_count(waveformChart, 50);  // 50 points for smooth wave
    lv_chart_set_update_mode(waveformChart, LV_CHART_UPDATE_MODE_SHIFT);

    // Styling - transparent background, thin neon line
    lv_obj_set_style_bg_opa(waveformChart, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(waveformChart, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(waveformChart, 0, LV_PART_MAIN);
    lv_obj_set_style_line_width(waveformChart, 2, LV_PART_ITEMS);

    // Create series for waveform
    waveformSeries = lv_chart_add_series(waveformChart, colors.accent, LV_CHART_AXIS_PRIMARY_Y);

    // Initialize with flat line (zero)
    for (int i = 0; i < 50; i++) {
        lv_chart_set_next_value(waveformChart, waveformSeries, 0);
    }

    // Create dummy card for compatibility
    card = lv_obj_create(root());
    lv_obj_add_flag(card, LV_OBJ_FLAG_HIDDEN);  // Hide it
}

void PerformanceScreen::setKit(uint8_t kitNumber, const char *name) {
    char kitBuffer[8];
    snprintf(kitBuffer, sizeof(kitBuffer), "%02u", static_cast<unsigned>(kitNumber));
    lv_label_set_text(lblKitNumber, kitBuffer);
    lv_label_set_text(lblKitName, name ? name : "----");
}

void PerformanceScreen::setBpm(uint16_t bpm) {
    char bpmBuffer[8];
    snprintf(bpmBuffer, sizeof(bpmBuffer), "%u", static_cast<unsigned>(bpm));
    lv_label_set_text(lblBpm, bpmBuffer);
}

void PerformanceScreen::onPadHit(uint8_t padId, uint8_t velocity) {
    flashArc(padId, velocity);
    updateWaveform(padId, velocity);
}

void PerformanceScreen::flashArc(uint8_t padId, uint8_t velocity) {
    if (padId >= padArcs.size() || padArcs[padId] == nullptr) {
        return;
    }

    lv_obj_t *arc = padArcs[padId];

    // Set indicator value to match velocity (fills the arc segment)
    lv_arc_set_value(arc, velocity);

    // Bright neon flash animation for the indicator
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_values(&a, LV_OPA_100, LV_OPA_10);  // Bright flash to dim
    lv_anim_set_time(&a, 200);
    lv_anim_set_delay(&a, 0);
    lv_anim_set_exec_cb(&a, [](void *obj, int32_t v) {
        lv_obj_set_style_arc_opa(static_cast<lv_obj_t *>(obj), static_cast<lv_opa_t>(v), LV_PART_INDICATOR);
    });
    lv_anim_start(&a);
}

void PerformanceScreen::updateWaveform(uint8_t padId, uint8_t velocity) {
    if (!waveformChart || !waveformSeries) {
        return;
    }

    LV_UNUSED(padId);  // Can use later to change waveform color per pad

    // Generate pulse waveform based on velocity
    // Velocity 0-127 maps to amplitude 0-100
    int amplitude = (velocity * 100) / 127;

    // Create attack-decay envelope (like a drum hit)
    // Fast attack (2 points up), slower decay (8 points down)
    lv_chart_set_next_value(waveformChart, waveformSeries, amplitude);         // Peak
    lv_chart_set_next_value(waveformChart, waveformSeries, amplitude * 0.8);   // 80%
    lv_chart_set_next_value(waveformChart, waveformSeries, amplitude * 0.6);   // 60%
    lv_chart_set_next_value(waveformChart, waveformSeries, amplitude * 0.4);   // 40%
    lv_chart_set_next_value(waveformChart, waveformSeries, amplitude * 0.25);  // 25%
    lv_chart_set_next_value(waveformChart, waveformSeries, amplitude * 0.15);  // 15%
    lv_chart_set_next_value(waveformChart, waveformSeries, amplitude * 0.08);  // 8%
    lv_chart_set_next_value(waveformChart, waveformSeries, amplitude * 0.03);  // 3%
    lv_chart_set_next_value(waveformChart, waveformSeries, 0);                 // Back to zero
    lv_chart_set_next_value(waveformChart, waveformSeries, 0);

    lv_chart_refresh(waveformChart);
}

}  // namespace ui
