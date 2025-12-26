#include "ui_manager.h"

#include "screens/scr_mixer.h"
#include "screens/scr_pad_edit.h"
#include "screens/scr_performance.h"
#include "screens/scr_settings.h"
#include "screens/scr_sample_browser.h"
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
    : screens{nullptr, nullptr, nullptr, nullptr, nullptr},
      activeView(ViewId::Performance),
      activeScreen(nullptr),
      initialized(false),
      toastBox(nullptr) {}

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
    screens[static_cast<int>(ViewId::SampleBrowser)] = new SampleBrowserScreen();

    setView(ViewId::Performance, false);
    initialized = true;
}

void UIManager::setView(ViewId view, bool animated) {
    if (static_cast<int>(view) >= static_cast<int>(ViewId::ViewCount)) {
        return;
    }

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

void UIManager::onMenuState(const MenuStateMsg& state) {
    // Switch view based on menu state
    ViewId targetView = ViewId::Performance;

    switch (state.state) {
        case MENU_STATE_HIDDEN:
            targetView = ViewId::Performance;
            break;

        case MENU_STATE_PAD_SELECT:
            // Use PadEdit screen but with pad selection mode
            targetView = ViewId::PadEdit;
            break;

        case MENU_STATE_PAD_CONFIG:
            targetView = ViewId::PadEdit;
            break;

        case MENU_STATE_SAMPLE_BROWSE:
            targetView = ViewId::SampleBrowser;
            break;

        case MENU_STATE_SAVING:
            // Show save confirmation toast
            showToast("SAVED!", 1500);
            break;

        default:
            break;
    }

    // Switch view if needed
    if (targetView != activeView && state.state != MENU_STATE_SAVING) {
        setView(targetView, true);
    }

    // Update the active screen with menu data
    if (activeView == ViewId::PadEdit) {
        PadEditScreen* padEdit = static_cast<PadEditScreen*>(screens[static_cast<int>(ViewId::PadEdit)]);
        if (padEdit) {
            padEdit->updateFromMenuState(state);
        }
    }
}

void UIManager::onSampleList(const SampleListMsg& samples) {
    // Update sample browser if it's active
    if (activeView == ViewId::SampleBrowser) {
        SampleBrowserScreen* browser = static_cast<SampleBrowserScreen*>(screens[static_cast<int>(ViewId::SampleBrowser)]);
        if (browser) {
            browser->updateSampleList(samples);
        }
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

void UIManager::showToast(const char* message, uint16_t durationMs) {
    // Remove existing toast if any
    hideToast();

    // Create toast container on active layer
    toastBox = lv_obj_create(lv_layer_top());
    lv_obj_set_size(toastBox, 180, 50);
    lv_obj_align(toastBox, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(toastBox, lv_color_hex(0x00AA00), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(toastBox, LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_radius(toastBox, 12, LV_PART_MAIN);
    lv_obj_set_style_border_width(toastBox, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(toastBox, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(toastBox, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(toastBox, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(toastBox, LV_OPA_50, LV_PART_MAIN);
    lv_obj_clear_flag(toastBox, LV_OBJ_FLAG_SCROLLABLE);

    // Create label
    lv_obj_t* label = lv_label_create(toastBox);
    lv_label_set_text(label, message);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(label);

    // Fade in animation
    lv_anim_t fadeIn;
    lv_anim_init(&fadeIn);
    lv_anim_set_var(&fadeIn, toastBox);
    lv_anim_set_values(&fadeIn, LV_OPA_TRANSP, LV_OPA_90);
    lv_anim_set_time(&fadeIn, 200);
    lv_anim_set_exec_cb(&fadeIn, [](void* obj, int32_t v) {
        lv_obj_set_style_bg_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(v), LV_PART_MAIN);
    });
    lv_anim_start(&fadeIn);

    // Auto-hide timer
    lv_timer_t* timer = lv_timer_create([](lv_timer_t* t) {
        UIManager::instance().hideToast();
        lv_timer_del(t);
    }, durationMs, nullptr);
    LV_UNUSED(timer);
}

void UIManager::hideToast() {
    if (toastBox) {
        // Fade out animation
        lv_anim_t fadeOut;
        lv_anim_init(&fadeOut);
        lv_anim_set_var(&fadeOut, toastBox);
        lv_anim_set_values(&fadeOut, LV_OPA_90, LV_OPA_TRANSP);
        lv_anim_set_time(&fadeOut, 200);
        lv_anim_set_exec_cb(&fadeOut, [](void* obj, int32_t v) {
            lv_obj_set_style_bg_opa(static_cast<lv_obj_t*>(obj), static_cast<lv_opa_t>(v), LV_PART_MAIN);
        });
        lv_anim_set_deleted_cb(&fadeOut, [](lv_anim_t* a) {
            lv_obj_del(static_cast<lv_obj_t*>(a->var));
        });
        lv_anim_start(&fadeOut);
        toastBox = nullptr;
    }
}

}  // namespace ui
