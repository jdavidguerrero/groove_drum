#pragma once

#include <lvgl.h>
#include <cstdint>

namespace ui {

enum class ViewId : uint8_t {
    Performance = 0,
    PadEdit = 1,
    Mixer = 2,
    Settings = 3
};

class UIScreen {
public:
    UIScreen();
    virtual ~UIScreen() = default;

    lv_obj_t *root();

    virtual void onShow();
    virtual void onHide();
    virtual void onEncoderLeft(int32_t delta, bool pressed);
    virtual void onEncoderRight(int32_t delta, bool pressed);
    virtual void onPadHit(uint8_t padId, uint8_t velocity);
    virtual void onButton(uint8_t buttonId, uint8_t state);

protected:
    lv_obj_t *screen;
};

class UIManager {
public:
    static UIManager &instance();

    void init();
    void setView(ViewId view, bool animated = true);
    ViewId currentView() const;

    void onEncoderLeft(int32_t delta, bool pressed);
    void onEncoderRight(int32_t delta, bool pressed);
    void onPadHit(uint8_t padId, uint8_t velocity);
    void onButton(uint8_t buttonId, uint8_t state);

private:
    UIManager();
    void loadScreen(UIScreen *target, bool animated);

    UIScreen *screens[4];
    ViewId activeView;
    UIScreen *activeScreen;
    bool initialized;
};

}  // namespace ui
