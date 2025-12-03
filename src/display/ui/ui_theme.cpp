#include "ui_theme.h"

namespace ui {

bool UITheme::initialized = false;
Palette UITheme::colors = {};
lv_style_t UITheme::styleBase;
lv_style_t UITheme::styleCard;
lv_style_t UITheme::styleLabelLarge;
lv_style_t UITheme::styleLabelMedium;
lv_style_t UITheme::styleLabelSmall;
lv_style_t UITheme::styleArc;

void UITheme::init() {
    if (initialized) {
        return;
    }

    colors.background = lv_color_hex(0x000000);
    colors.primary = lv_color_hex(0xFFFFFF);
    colors.accent = lv_color_hex(0x00FFFF);
    colors.value = lv_color_hex(0xFFA500);
    colors.alert = lv_color_hex(0xFF0000);

    lv_style_init(&styleBase);
    lv_style_set_bg_color(&styleBase, colors.background);
    lv_style_set_bg_grad_color(&styleBase, colors.background);
    lv_style_set_bg_opa(&styleBase, LV_OPA_COVER);
    lv_style_set_text_color(&styleBase, colors.primary);
    lv_style_set_border_width(&styleBase, 0);

    lv_style_init(&styleCard);
    lv_style_set_bg_color(&styleCard, lv_color_darken(colors.background, 10));
    lv_style_set_bg_opa(&styleCard, LV_OPA_60);
    lv_style_set_border_width(&styleCard, 1);
    lv_style_set_border_color(&styleCard, colors.accent);
    lv_style_set_pad_all(&styleCard, 8);
    lv_style_set_radius(&styleCard, 12);

    lv_style_init(&styleLabelLarge);
    lv_style_set_text_color(&styleLabelLarge, colors.primary);
    lv_style_set_text_font(&styleLabelLarge, &lv_font_montserrat_32);

    lv_style_init(&styleLabelMedium);
    lv_style_set_text_color(&styleLabelMedium, colors.primary);
    lv_style_set_text_font(&styleLabelMedium, &lv_font_montserrat_20);

    lv_style_init(&styleLabelSmall);
    lv_style_set_text_color(&styleLabelSmall, colors.primary);
    lv_style_set_text_font(&styleLabelSmall, &lv_font_montserrat_14);

    lv_style_init(&styleArc);
    lv_style_set_arc_width(&styleArc, 14);  // Thicker arcs for "reactor" look
    lv_style_set_arc_color(&styleArc, colors.accent);
    lv_style_set_arc_rounded(&styleArc, false);  // Sharp edges, more technical
    lv_style_set_bg_opa(&styleArc, LV_OPA_TRANSP);

    initialized = true;
}

const Palette &UITheme::palette() {
    return colors;
}

lv_style_t &UITheme::base() { return styleBase; }

lv_style_t &UITheme::card() { return styleCard; }

lv_style_t &UITheme::labelLarge() { return styleLabelLarge; }

lv_style_t &UITheme::labelMedium() { return styleLabelMedium; }

lv_style_t &UITheme::labelSmall() { return styleLabelSmall; }

lv_style_t &UITheme::arc() { return styleArc; }

}  // namespace ui
