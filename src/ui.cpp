/*
 * ui.cpp
 *
 * https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/Examples/Basics/2-TouchTest/2-TouchTest.ino
 * https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/Examples/Basics/4-BacklightControlTest/4-BacklightControlTest.ino
 *
 * ESP8266 / ESP32 Environmental Sensor
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <xythobuz@xythobuz.de> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you
 * think this stuff is worth it, you can buy me a beer in return.   Thomas Buck
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>

#include "config.h"
#include "mqtt.h"
#include "ui.h"

#ifdef FEATURE_UI

#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

#define LEDC_CHANNEL_0 0
#define LEDC_TIMER_12_BIT 12
#define LEDC_BASE_FREQ 5000

#define TOUCH_LEFT 180
#define TOUCH_RIGHT 3750

#define TOUCH_TOP 230
#define TOUCH_BOTTOM 3800

#define BTN_W 120
#define BTN_H 60
#define BTN_GAP 20

#define BTNS_OFF_X ((LCD_WIDTH - (2 * BTN_W) - (1 * BTN_GAP)) / 2)
#define BTNS_OFF_Y ((LCD_HEIGHT - (3 * BTN_H) - (2 * BTN_GAP)) / 2)

#define INVERT_BOOL(x) (x) = !(x);

static SPIClass mySpi = SPIClass(HSPI);
static XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
static TFT_eSPI tft = TFT_eSPI();

struct ui_status ui_status = {0};

enum ui_pages {
    UI_START = 0,
    UI_LIVINGROOM1,
    UI_LIVINGROOM2,

    UI_NUM_PAGES
};

static enum ui_pages ui_page = UI_START;

static TS_Point touchToScreen(TS_Point p) {
    p.x = map(p.x, TOUCH_LEFT, TOUCH_RIGHT, 0, LCD_WIDTH);
    p.y = map(p.y, TOUCH_TOP, TOUCH_BOTTOM, 0, LCD_HEIGHT);
    return p;
}

static void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
    uint32_t duty = (4095 / valueMax) * min(value, valueMax);
    ledcWrite(channel, duty);
}

static void draw_button(const char *name, uint32_t x, uint32_t y, uint32_t color) {
    tft.fillRect(x - BTN_W / 2, y - BTN_H / 2, BTN_W, BTN_H, color);

    tft.setTextDatum(MC_DATUM); // middle center
    tft.drawString(name, x, y, 2);
}

static void draw_livingroom1(void) {
    draw_button("Lights Corner", BTNS_OFF_X + BTN_W / 2, BTNS_OFF_Y + BTN_H / 2, ui_status.light_corner ? TFT_GREEN : TFT_RED);
    draw_button("Lights Workspace", BTNS_OFF_X + BTN_W / 2, BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP, ui_status.light_workspace ? TFT_GREEN : TFT_RED);
    draw_button("Lights Kitchen", BTNS_OFF_X + BTN_W / 2, BTNS_OFF_Y + BTN_H / 2 + (BTN_H + BTN_GAP) * 2, ui_status.light_kitchen ? TFT_GREEN : TFT_RED);

    draw_button("Sound Amp.", BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP, BTNS_OFF_Y + BTN_H / 2, ui_status.sound_amplifier ? TFT_GREEN : TFT_RED);
    draw_button("All Lights Off", BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP, BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP, TFT_RED);
}

static void draw_livingroom2(void) {
    draw_button("Lights PC", BTNS_OFF_X + BTN_W / 2, BTNS_OFF_Y + BTN_H / 2, ui_status.light_corner ? TFT_GREEN : TFT_RED);
    draw_button("Lights Bench", BTNS_OFF_X + BTN_W / 2, BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP, ui_status.light_workspace ? TFT_GREEN : TFT_RED);
    //draw_button("Lights Amp.", BTNS_OFF_X + BTN_W / 2, BTNS_OFF_Y + BTN_H / 2 + (BTN_H + BTN_GAP) * 2, ui_status.light_kitchen ? TFT_GREEN : TFT_RED);

    draw_button("Lights Amp.", BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP, BTNS_OFF_Y + BTN_H / 2, ui_status.sound_amplifier ? TFT_GREEN : TFT_RED);
    draw_button("Lights Box", BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP, BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP, TFT_RED); // TODO both
}

void ui_init(void) {
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);

    tft.init();
    tft.setRotation(1);

    ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
    ledcAttachPin(TFT_BL, LEDC_CHANNEL_0);
    ledcAnalogWrite(LEDC_CHANNEL_0, 255);

    ui_progress(UI_INIT);
}

void ui_progress(enum ui_state state) {
    int x = LCD_WIDTH / 2;
    int y = LCD_HEIGHT / 2;
    int fontSize = 2;

    switch (state) {
        case UI_INIT: {
            tft.fillScreen(TFT_BLACK);
            tft.setTextDatum(MC_DATUM); // middle center
            tft.drawString("Initializing ESP-ENV", x, y - 32, fontSize);
            tft.drawString("xythobuz.de", x, y, fontSize);
        } break;

        case UI_WIFI_CONNECT: {
            tft.setTextDatum(MC_DATUM); // middle center
            tft.drawString("Connecting to '" WIFI_SSID "'", x, y + 32, fontSize);
        } break;

        case UI_WIFI_CONNECTING: {
            static int n = 0;
            const char anim[] = { '\\', '|', '/', '-' };
            n++;
            if (n >= sizeof(anim)) {
                n = 0;
            }
            char s[2] = { anim[n], '\0' };
            tft.drawCentreString(s, x, y + 64, fontSize);
        } break;

        case UI_WIFI_CONNECTED: {
            tft.setTextDatum(MC_DATUM); // middle center
            tft.drawString("Connected!", x, y + 64, fontSize);
        } break;

        case UI_READY: {
            tft.fillScreen(TFT_BLACK);
            tft.setTextDatum(MC_DATUM); // middle center
            tft.drawString("Ready", x, y, fontSize);
        } break;
    }
}

void ui_draw_menu(void) {
    switch (ui_page) {
        case UI_START:
            tft.fillScreen(TFT_BLACK);
            ui_page = UI_LIVINGROOM1;
            // fall-through

        case UI_LIVINGROOM1:
            draw_livingroom1();
            break;

        case UI_LIVINGROOM2:
            draw_livingroom2();
            break;

        default:
            ui_page = UI_START;
    }

    draw_button("Next...", BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP, BTNS_OFF_Y + BTN_H / 2 + (BTN_H + BTN_GAP) * 2, TFT_MAGENTA);
}

void ui_run(void) {
    if (ts.tirqTouched() && ts.touched()) {
        TS_Point p = touchToScreen(ts.getPoint());

        if ((p.x >= BTNS_OFF_X) && (p.x <= BTNS_OFF_X + BTN_W) && (p.y >= BTNS_OFF_Y) && (p.y <= BTNS_OFF_Y + BTN_H)) {
            INVERT_BOOL(ui_status.light_corner);
        } else if ((p.x >= BTNS_OFF_X) && (p.x <= BTNS_OFF_X + BTN_W) && (p.y >= (BTNS_OFF_Y + BTN_H + BTN_GAP)) && (p.y <= (BTNS_OFF_Y + BTN_H + BTN_GAP + BTN_H))) {
            INVERT_BOOL(ui_status.light_workspace);
        } else if ((p.x >= BTNS_OFF_X) && (p.x <= BTNS_OFF_X + BTN_W) && (p.y >= (BTNS_OFF_Y + BTN_H * 2 + BTN_GAP * 2)) && (p.y <= (BTNS_OFF_Y + BTN_H * 2 + BTN_GAP * 2 + BTN_H))) {
            INVERT_BOOL(ui_status.light_kitchen);
        } else if ((p.x >= BTNS_OFF_X + BTN_W + BTN_GAP) && (p.x <= BTNS_OFF_X + BTN_W + BTN_GAP + BTN_W) && (p.y >= BTNS_OFF_Y) && (p.y <= BTNS_OFF_Y + BTN_H)) {
            INVERT_BOOL(ui_status.sound_amplifier);
        } else if ((p.x >= BTNS_OFF_X + BTN_W + BTN_GAP) && (p.x <= BTNS_OFF_X + BTN_W + BTN_GAP + BTN_W) && (p.y >= (BTNS_OFF_Y + BTN_H + BTN_GAP)) && (p.y <= (BTNS_OFF_Y + BTN_H + BTN_GAP + BTN_H))) {
            // TODO should act on both TV lights (box and amp)
        } else if ((p.x >= BTNS_OFF_X + BTN_W + BTN_GAP) && (p.x <= BTNS_OFF_X + BTN_W + BTN_GAP + BTN_W) && (p.y >= (BTNS_OFF_Y + BTN_H * 2 + BTN_GAP * 2)) && (p.y <= (BTNS_OFF_Y + BTN_H * 2 + BTN_GAP * 2 + BTN_H))) {
            // switch to next page
            ui_page = (enum ui_pages)((ui_page + 1) % UI_NUM_PAGES);
            if (ui_page == UI_START) {
                // skip init screen
                ui_page = (enum ui_pages)((ui_page + 1) % UI_NUM_PAGES);
            }
            tft.fillScreen(TFT_BLACK);
        }

        ui_draw_menu();
        delay(100); // TODO
    }
}

#endif // FEATURE_UI
