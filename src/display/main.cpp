#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <edrum_config.h>
#include <SPI.h>

#include "ui/ui_manager.h"
#include "comm/uart_link.h"
#include "drivers/ring_led_controller.h"

static const uint16_t kScreenWidth = 240;
static const uint16_t kScreenHeight = 240;
static const int kBacklightPin = 2;

// LVGL 8.3.x uses different buffer management
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[kScreenWidth * 60];  // ~28 KB line buffer
static lv_disp_drv_t disp_drv;

static TFT_eSPI tft = TFT_eSPI(kScreenWidth, kScreenHeight);
HardwareSerial SerialLink(1);

// LVGL display flush callback for TFT_eSPI (LVGL 8.3.x API)
void my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void setup() {
    // CRITICAL: Initialize USB serial FIRST and wait
    Serial.begin(115200);
    delay(2000);  // Wait for USB serial to stabilize

    Serial.println("\n\n=================================");
    Serial.println("=== E-Drum Display MCU Init ===");
    Serial.println("=================================");
    Serial.flush();

    // Force HSPI pin assignment to avoid default-pin warning on S3
    SPI.begin(TFT_SCLK, -1, TFT_MOSI, -1);

    Serial.println("[UART] Configurando enlace serie con Main Brain...");
    display::comm::UARTLink::begin(SerialLink, UART_BAUD);

    // Step 1: Backlight init
    Serial.println("[1/6] Initializing backlight...");
    pinMode(kBacklightPin, OUTPUT);
    digitalWrite(kBacklightPin, HIGH);
    Serial.println("      Backlight ON");
    delay(100);

    // Step 2: TFT init
    Serial.println("[2/6] Initializing TFT...");
    tft.init();
    tft.setSwapBytes(true);
    tft.setRotation(0);
    Serial.println("      TFT initialized");
    delay(100);

    // Step 3: LVGL init
    Serial.println("[3/6] Initializing LVGL 8.4.0...");
    tft.fillScreen(TFT_BLACK);
    lv_init();
    Serial.println("      lv_init() done");

    // Step 4: LVGL display driver
    Serial.println("[4/6] Configuring LVGL display driver...");
    lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, kScreenWidth * 60);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = kScreenWidth;
    disp_drv.ver_res = kScreenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("      Display driver registered");

    // Step 5: Create splash screen with loading animation
    Serial.println("[5/6] Creating splash screen...");

    lv_obj_t* splashScreen = lv_scr_act();
    lv_obj_set_style_bg_color(splashScreen, lv_color_hex(0x0D0D0D), LV_PART_MAIN);

    // Brand name - "GROOVE FORGE"
    lv_obj_t* brandLabel = lv_label_create(splashScreen);
    lv_label_set_text(brandLabel, "GROOVE FORGE");
    lv_obj_set_style_text_color(brandLabel, lv_color_hex(0xFF6600), LV_PART_MAIN);
    lv_obj_set_style_text_font(brandLabel, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(brandLabel, LV_ALIGN_CENTER, 0, -40);

    // Product name - "E-DRUM"
    lv_obj_t* productLabel = lv_label_create(splashScreen);
    lv_label_set_text(productLabel, "E-DRUM");
    lv_obj_set_style_text_color(productLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(productLabel, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_align(productLabel, LV_ALIGN_CENTER, 0, -10);

    // Loading bar background
    lv_obj_t* barBg = lv_obj_create(splashScreen);
    lv_obj_set_size(barBg, 160, 8);
    lv_obj_align(barBg, LV_ALIGN_CENTER, 0, 40);
    lv_obj_set_style_bg_color(barBg, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_radius(barBg, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(barBg, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(barBg, 0, LV_PART_MAIN);
    lv_obj_clear_flag(barBg, LV_OBJ_FLAG_SCROLLABLE);

    // Loading bar fill
    lv_obj_t* barFill = lv_obj_create(barBg);
    lv_obj_set_size(barFill, 0, 8);
    lv_obj_align(barFill, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(barFill, lv_color_hex(0xFF6600), LV_PART_MAIN);
    lv_obj_set_style_radius(barFill, 4, LV_PART_MAIN);
    lv_obj_set_style_border_width(barFill, 0, LV_PART_MAIN);
    lv_obj_clear_flag(barFill, LV_OBJ_FLAG_SCROLLABLE);

    // Status text
    lv_obj_t* statusLabel = lv_label_create(splashScreen);
    lv_label_set_text(statusLabel, "Initializing...");
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, 60);

    // Version info
    lv_obj_t* versionLabel = lv_label_create(splashScreen);
    lv_label_set_text(versionLabel, "v1.0");
    lv_obj_set_style_text_color(versionLabel, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_text_font(versionLabel, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(versionLabel, LV_ALIGN_BOTTOM_MID, 0, -15);

    // Animate loading bar (~5 seconds total)
    const char* loadingSteps[] = {
        "Initializing...",
        "Loading drivers...",
        "Connecting UART...",
        "Loading UI...",
        "Ready!"
    };

    // First render to show initial state
    lv_timer_handler();
    lv_tick_inc(5);
    delay(100);

    // 50 steps, ~100ms each = 5 seconds total
    for (int i = 0; i <= 50; i++) {
        int percent = (i * 100) / 50;
        int barWidth = (percent * 160) / 100;
        lv_obj_set_width(barFill, barWidth);

        // Update status text at key points (every 20%)
        int stepIndex = percent / 20;
        if (stepIndex > 4) stepIndex = 4;
        lv_label_set_text(statusLabel, loadingSteps[stepIndex]);

        // Force LVGL to process and render
        lv_tick_inc(100);
        lv_timer_handler();
        delay(100);
    }

    // Hold on "Ready!" briefly
    lv_tick_inc(500);
    lv_timer_handler();
    delay(500);

    Serial.println("[6/6] Splash complete");
    Serial.println("\n=================================");
    Serial.println("=== BOOT SUCCESSFUL ===");
    Serial.println("=================================\n");

    // Now load UI Manager
    Serial.println("Initializing UI Manager...");
    ui::UIManager::instance().init();
    Serial.println("UI Manager initialized - Ready!");
    Serial.flush();

    display::RingLEDController::begin();
    display::comm::UARTLink::requestConfigDump();
}

void loop() {
    display::comm::UARTLink::process();
    lv_timer_handler();
    lv_tick_inc(5);
    display::RingLEDController::update();
    delay(5);
}
