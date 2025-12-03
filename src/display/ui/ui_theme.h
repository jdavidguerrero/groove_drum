#pragma once

#include <lvgl.h>

namespace ui {

struct Palette {
    lv_color_t background;
    lv_color_t primary;
    lv_color_t accent;
    lv_color_t value;
    lv_color_t alert;
};

class UITheme {
public:
    static void init();

    static const Palette &palette();
    static lv_style_t &base();
    static lv_style_t &card();
    static lv_style_t &labelLarge();
    static lv_style_t &labelMedium();
    static lv_style_t &labelSmall();
    static lv_style_t &arc();

private:
    static bool initialized;
    static Palette colors;
    static lv_style_t styleBase;
    static lv_style_t styleCard;
    static lv_style_t styleLabelLarge;
    static lv_style_t styleLabelMedium;
    static lv_style_t styleLabelSmall;
    static lv_style_t styleArc;
};

}  // namespace ui
