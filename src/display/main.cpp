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

    // Step 3: Color test pattern
    Serial.println("[3/6] Testing display colors...");
    tft.fillScreen(TFT_RED);
    Serial.println("      RED");
    delay(500);

    tft.fillScreen(TFT_GREEN);
    Serial.println("      GREEN");
    delay(500);

    tft.fillScreen(TFT_BLUE);
    Serial.println("      BLUE");
    delay(500);

    tft.fillScreen(TFT_BLACK);
    Serial.println("      BLACK");
    delay(200);

    // Step 4: LVGL init
    Serial.println("[4/6] Initializing LVGL 8.4.0...");
    lv_init();
    Serial.println("      lv_init() done");

    // Step 5: LVGL display driver
    Serial.println("[5/6] Configuring LVGL display driver...");
    lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, kScreenWidth * 60);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = kScreenWidth;
    disp_drv.ver_res = kScreenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("      Display driver registered");

    // Step 6: Test LVGL with simple label
    Serial.println("[6/6] Creating test label...");
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "LVGL OK!");
    lv_obj_center(label);
    Serial.println("      Test label created");

    // Give LVGL time to render
    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        delay(50);
    }

    Serial.println("\n=================================");
    Serial.println("=== BOOT SUCCESSFUL ===");
    Serial.println("=================================\n");

    // Wait 2 seconds to see the test label
    delay(2000);

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
