#include "ui_manager.h"

#include "screens/scr_mixer.h"
#include "screens/scr_pad_edit.h"
#include "screens/scr_performance.h"
#include "screens/scr_settings.h"
#include "ui_theme.h"

namespace ui {

UIScreen::UIScreen() : screen(nullptr) {
    lv_disp_t *disp = lv_disp_get_default();
    lv_coord_t width = disp ? lv_disp_get_hor_res(disp) : 240;
    lv_coord_t height = disp ? lv_disp_get_ver_res(disp) : 240;

    screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, width, height);
    lv_obj_remove_style_all(screen);
    lv_obj_add_style(screen, &UITheme::base(), LV_PART_MAIN);
}

lv_obj_t *UIScreen::root() {
    return screen;
}

void UIScreen::onShow() {}
void UIScreen::onHide() {}
void UIScreen::onEncoderLeft(int32_t, bool) {}
void UIScreen::onEncoderRight(int32_t, bool) {}
void UIScreen::onPadHit(uint8_t, uint8_t) {}
void UIScreen::onButton(uint8_t, uint8_t) {}

UIManager::UIManager()
    : screens{nullptr, nullptr, nullptr, nullptr},
      activeView(ViewId::Performance),
      activeScreen(nullptr),
      initialized(false) {}

UIManager &UIManager::instance() {
    static UIManager inst;
    return inst;
}

void UIManager::init() {
    if (initialized) {
        return;
    }

    UITheme::init();

    screens[static_cast<int>(ViewId::Performance)] = new PerformanceScreen();
    screens[static_cast<int>(ViewId::PadEdit)] = new PadEditScreen();
    screens[static_cast<int>(ViewId::Mixer)] = new MixerScreen();
    screens[static_cast<int>(ViewId::Settings)] = new SettingsScreen();

    setView(ViewId::Performance, false);
    initialized = true;
}

void UIManager::setView(ViewId view, bool animated) {
    UIScreen *target = screens[static_cast<int>(view)];
    if (!target || target == activeScreen) {
        return;
    }

    if (activeScreen) {
        activeScreen->onHide();
    }

    loadScreen(target, animated);
    activeView = view;
    activeScreen = target;
    activeScreen->onShow();
}

ViewId UIManager::currentView() const {
    return activeView;
}

void UIManager::onEncoderLeft(int32_t delta, bool pressed) {
    if (activeScreen) {
        activeScreen->onEncoderLeft(delta, pressed);
    }
}

void UIManager::onEncoderRight(int32_t delta, bool pressed) {
    if (activeScreen) {
        activeScreen->onEncoderRight(delta, pressed);
    }
}

void UIManager::onPadHit(uint8_t padId, uint8_t velocity) {
    if (activeScreen) {
        activeScreen->onPadHit(padId, velocity);
    }
}

void UIManager::onButton(uint8_t buttonId, uint8_t state) {
    if (buttonId == 0) {  // KIT button to jump home
        setView(ViewId::Performance);
    }
    if (activeScreen) {
        activeScreen->onButton(buttonId, state);
    }
}

void UIManager::loadScreen(UIScreen *target, bool animated) {
    if (!target) {
        return;
    }

    if (animated) {
        // Sci-fi fade transition - smooth and futuristic
        lv_scr_load_anim(target->root(), LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
    } else {
        lv_scr_load(target->root());
    }
}

}  // namespace ui
