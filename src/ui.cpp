/*
 * ui.cpp
 *
 * https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/Examples/Basics/2-TouchTest/2-TouchTest.ino
 * https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/Examples/Basics/4-BacklightControlTest/4-BacklightControlTest.ino
 *
 * ESP8266 / ESP32 Environmental
 * Touch UI for ESP32 CYD (Cheap Yellow Display).
 *
 * LDR circuit on this board is really strange and does not give me useful information.
 * To get it working remove R19 and replace R15 with 100k.
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <xythobuz@xythobuz.de> wrote this file.  As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you
 * think this stuff is worth it, you can buy me a beer in return.   Thomas Buck
 * ----------------------------------------------------------------------------
 */

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "config.h"
#include "DebugLog.h"
#include "mqtt.h"
#include "memory.h"
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

#define LDR_PIN 34
#define BTN_PIN 0

#define BTN_W 120
#define BTN_H 60
#define BTN_GAP 20

#define BTNS_OFF_X ((LCD_WIDTH - (2 * BTN_W) - (1 * BTN_GAP)) / 2)
#define BTNS_OFF_Y ((LCD_HEIGHT - (3 * BTN_H) - (2 * BTN_GAP)) / 2)

#define INVERT_BOOL(x) (x) = !(x)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define LDR_CHECK_MS 100
#define LDR_DARK_VALUE 1200
#define LDR_BRIGHT_VALUE 0
#define LDR_LOWPASS_FACT 0.025f

#define STANDBY_BRIGHTNESS 10
#define LCD_MIN_BRIGHTNESS (STANDBY_BRIGHTNESS * 2)
#define LCD_MAX_BRIGHTNESS 255

#define MIN_TOUCH_DELAY_MS 200
#define TOUCH_PRESSURE_MIN 1000
#define FULL_BRIGHT_MS (1000 * 30)
#define NO_BRIGHT_MS (1000 * 2)

#define NTP_SERVER "pool.ntp.org"
#define STANDBY_REDRAW_MS 500

#define CALIB_1_X 42
#define CALIB_1_Y 42
#define CALIB_2_X LCD_WIDTH - 1 - CALIB_1_X
#define CALIB_2_Y LCD_HEIGHT - 1 - CALIB_1_Y

// TODO auto-detect?!
#warning hard-coded timezone and daylight savings offset
#define gmtOffset_sec (60 * 60)
#define daylightOffset_sec (60 * 60)

#if (LCD_MIN_BRIGHTNESS <= STANDBY_BRIGHTNESS)
#error STANDBY_BRIGHTNESS needs to be bigger than LCD_MIN_BRIGHTNESS
#endif

static SPIClass mySpi = SPIClass(HSPI);
static XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
static TFT_eSPI tft = TFT_eSPI();

struct ui_status ui_status = {0};

enum ui_pages {
    UI_START = 0,
    UI_LIVINGROOM1,
    UI_LIVINGROOM2,
    UI_BATHROOM,
    UI_INFO,

    UI_NUM_PAGES
};

static enum ui_pages ui_page = UI_START;
static bool is_touched = false;
static unsigned long last_ldr = 0;
static int ldr_value = 0;
static unsigned long last_touch_time = 0;
static int curr_brightness = LCD_MAX_BRIGHTNESS;
static int set_max_brightness = LCD_MAX_BRIGHTNESS;
static unsigned long last_standby_draw = 0;

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
    // 1
    draw_button("Lights Corner",
                BTNS_OFF_X + BTN_W / 2,
                BTNS_OFF_Y + BTN_H / 2,
                ui_status.light_corner ? TFT_GREEN : TFT_RED);

    // 2
    draw_button("Lights Workspace",
                BTNS_OFF_X + BTN_W / 2,
                BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP,
                ui_status.light_workspace ? TFT_GREEN : TFT_RED);

    // 3
    draw_button("Lights Sink",
                BTNS_OFF_X + BTN_W / 2,
                BTNS_OFF_Y + BTN_H / 2 + (BTN_H + BTN_GAP) * 2,
                ui_status.light_sink ? TFT_GREEN : TFT_RED);

    // 4
    draw_button("Sound Amp.",
                BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP,
                BTNS_OFF_Y + BTN_H / 2,
                ui_status.sound_amplifier ? TFT_GREEN : TFT_RED);

    // 5
    bool on = ui_status.light_corner || ui_status.light_sink || ui_status.light_workspace
            || ui_status.light_amp || ui_status.light_bench || ui_status.light_box
            || ui_status.light_kitchen || ui_status.light_pc || ui_status.pc_displays;
    draw_button(on ? "All Lights Off" : "Wake Up Lights",
                BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP,
                BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP,
                TFT_MAGENTA);
}

static void draw_livingroom2(void) {
    // 1
    draw_button("Lights PC",
                BTNS_OFF_X + BTN_W / 2,
                BTNS_OFF_Y + BTN_H / 2,
                ui_status.light_pc ? TFT_GREEN : TFT_RED);

    // 2
    draw_button("Lights Bench",
                BTNS_OFF_X + BTN_W / 2,
                BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP,
                ui_status.light_bench ? TFT_GREEN : TFT_RED);

    // 3
    draw_button("Lights Kitchen",
                BTNS_OFF_X + BTN_W / 2,
                BTNS_OFF_Y + BTN_H / 2 + (BTN_H + BTN_GAP) * 2,
                ui_status.light_kitchen ? TFT_GREEN : TFT_RED);

    // 4
    draw_button("Lights Amp.",
                BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP,
                BTNS_OFF_Y + BTN_H / 2,
                ui_status.light_amp ? TFT_GREEN : TFT_RED);

    // 5
    draw_button("Lights Box",
                BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP,
                BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP,
                ui_status.light_box ? TFT_GREEN : TFT_RED);
}

static void draw_bathroom(void) {
    // 1
    draw_button("Bath Lights Auto",
                BTNS_OFF_X + BTN_W / 2,
                BTNS_OFF_Y + BTN_H / 2,
                ui_status.bathroom_lights == BATH_LIGHT_NONE ? TFT_GREEN : TFT_RED);

    // 2
    draw_button("Bath Lights Big",
                BTNS_OFF_X + BTN_W / 2,
                BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP,
                ui_status.bathroom_lights == BATH_LIGHT_BIG ? TFT_GREEN : TFT_RED);

    // 3
    // TODO own page?
    draw_button("PC Displays",
                BTNS_OFF_X + BTN_W / 2,
                BTNS_OFF_Y + BTN_H / 2 + (BTN_H + BTN_GAP) * 2,
                ui_status.pc_displays ? TFT_GREEN : TFT_RED);

    // 4
    draw_button("Bath Lights Off",
                BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP,
                BTNS_OFF_Y + BTN_H / 2,
                ui_status.bathroom_lights == BATH_LIGHT_OFF ? TFT_GREEN : TFT_RED);

    // 5
    draw_button("Bath Lights Small",
                BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP,
                BTNS_OFF_Y + BTN_H / 2 + BTN_H + BTN_GAP,
                ui_status.bathroom_lights == BATH_LIGHT_SMALL ? TFT_GREEN : TFT_RED);
}

static void draw_info(void) {
    tft.fillScreen(TFT_BLACK);

    tft.setTextDatum(TC_DATUM); // top center
    tft.drawString(ESP_PLATFORM_NAME " " NAME_OF_FEATURE " V" ESP_ENV_VERSION, LCD_WIDTH / 2, 0, 2);
    tft.drawString("by xythobuz.de", LCD_WIDTH / 2, 16, 2);

    tft.setTextDatum(TL_DATUM); // top left
    tft.drawString("Build Date: " __DATE__, 0, 40 + 16 * 0, 1);
    tft.drawString("Build Time: " __TIME__, 0, 40 + 16 * 1, 1);
    tft.drawString("Location: " SENSOR_LOCATION, 0, 40 + 16 * 2, 1);
    tft.drawString("ID: " SENSOR_ID, 0, 40 + 16 * 3, 1);
    tft.drawString("MAC: " + String(WiFi.macAddress()), 0, 40 + 16 * 4, 1);
    tft.drawString("Free heap: " + String(ESP.getFreeHeap() / 1024.0f) + "k", 0, 40 + 16 * 5, 1);
    tft.drawString("Free sketch space: " + String(ESP.getFreeSketchSpace() / 1024.0f) + "k", 0, 40 + 16 * 6, 1);
    tft.drawString("Flash chip size: " + String(ESP.getFlashChipSize() / 1024.0f) + "k", 0, 40 + 16 * 7, 1);
    tft.drawString("Uptime: " + String(millis() / 1000) + "sec", 0, 40 + 16 * 8, 1);
    tft.drawString("IPv4: " + WiFi.localIP().toString(), 0, 40 + 16 * 9, 1);
    tft.drawString("IPv6: " + WiFi.localIPv6().toString(), 0, 40 + 16 * 10, 1);
    tft.drawString("Hostname: " + String(SENSOR_HOSTNAME_PREFIX) + String(SENSOR_ID), 0, 40 + 16 * 11, 1);
    tft.drawString("LDR: " + String(ldr_value), 0, 40 + 16 * 12, 1);
}

static void draw_standby(void) {
    // only clear whole screen when first entering standby page
    if ((curr_brightness > STANDBY_BRIGHTNESS)) {
        tft.fillScreen(TFT_BLACK);
    }

    tft.setTextDatum(TC_DATUM); // top center
    tft.drawString(ESP_PLATFORM_NAME " " NAME_OF_FEATURE " V" ESP_ENV_VERSION, LCD_WIDTH / 2, 0, 2);
    tft.drawString("by xythobuz.de", LCD_WIDTH / 2, 16, 2);

    struct tm timeinfo;
    String date, time;
    String weekday[7] = { "So.", "Mo.", "Di.", "Mi.", "Do.", "Fr.", "Sa." };
    if(getLocalTime(&timeinfo)) {
        date += weekday[timeinfo.tm_wday % 7];
        date += " ";
        if (timeinfo.tm_mday < 10) {
            date += "0";
        }
        date += String(timeinfo.tm_mday);
        date += ".";
        if ((timeinfo.tm_mon + 1) < 10) {
            date += "0";
        }
        date += String(timeinfo.tm_mon + 1);
        date += ".";
        date += String(timeinfo.tm_year + 1900);

        if (timeinfo.tm_hour < 10) {
            time += "0";
        }
        time += String(timeinfo.tm_hour);
        time += ":";
        if (timeinfo.tm_min < 10) {
            time += "0";
        }
        time += String(timeinfo.tm_min);
        time += ":";
        if (timeinfo.tm_sec < 10) {
            time += "0";
        }
        time += String(timeinfo.tm_sec);
    }

    tft.setTextDatum(MC_DATUM); // middle center
    tft.drawString(date, LCD_WIDTH / 2, LCD_HEIGHT / 2 - 8, 2);
    tft.drawString(time, LCD_WIDTH / 2, LCD_HEIGHT / 2 + 8, 2);

    tft.setTextDatum(BC_DATUM); // bottom center
    tft.drawString("Touch to begin...", LCD_WIDTH / 2, LCD_HEIGHT, 2);
}

void ui_init(void) {
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);

    tft.init();
    tft.setRotation(1);

    ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
    ledcAttachPin(TFT_BL, LEDC_CHANNEL_0);
    curr_brightness = set_max_brightness;
    ledcAnalogWrite(LEDC_CHANNEL_0, curr_brightness);

    pinMode(BTN_PIN, INPUT);

    pinMode(LDR_PIN, ANALOG);
    analogSetAttenuation(ADC_0db);
    analogReadResolution(12);
    analogSetPinAttenuation(LDR_PIN, ADC_0db);
    ldr_value = analogRead(LDR_PIN);

    ui_progress(UI_INIT);
}

static void ui_draw_menu(void) {
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

        case UI_BATHROOM:
            draw_bathroom();
            break;

        case UI_INFO:
            draw_info();
            return; // no next button

        default:
            ui_page = UI_START;
            ui_draw_menu();
            return;
    }

    draw_button("Next...", BTNS_OFF_X + BTN_W / 2 + BTN_W + BTN_GAP, BTNS_OFF_Y + BTN_H / 2 + (BTN_H + BTN_GAP) * 2, TFT_CYAN);
}

static void ui_draw_reticule(int x, int y, int l) {
    tft.drawFastHLine(x - l / 2, y, l, TFT_RED);
    tft.drawFastVLine(x, y - l / 2, l, TFT_RED);
    tft.drawCircle(x, y, l / 4, TFT_GREEN);
    tft.drawPixel(x, y, TFT_WHITE);
}

static TS_Point touchToScreen(TS_Point p) {
    p.x = map(p.x, config.touch_calibrate_left, config.touch_calibrate_right, CALIB_1_X, CALIB_2_X);
    p.y = map(p.y, config.touch_calibrate_top, config.touch_calibrate_bottom, CALIB_1_Y, CALIB_2_Y);
    if (p.x < 0) { p.x = 0; }
    if (p.x >= LCD_WIDTH) { p.x = LCD_WIDTH - 1; }
    if (p.y < 0) { p.y = 0; }
    if (p.y >= LCD_HEIGHT) { p.y = LCD_HEIGHT - 1; }
    return p;
}

static void ui_calibrate_touchscreen(void) {
    for (int step = 0; step < 3; step++) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM); // middle center
        tft.drawString("Calibrate Touchscreen", LCD_WIDTH / 2, LCD_HEIGHT / 2, 2);

        if (step == 0) {
            ui_draw_reticule(CALIB_1_X, CALIB_1_Y, 20);
        } else if (step == 1) {
            ui_draw_reticule(CALIB_2_X, CALIB_2_Y, 20);
        } else {
            tft.drawString("Press button to save", LCD_WIDTH / 2, LCD_HEIGHT / 2 + 20, 2);
            tft.drawString("Power off to re-do", LCD_WIDTH / 2, LCD_HEIGHT / 2 + 40, 2);
        }

        if (step < 2) {
            bool touched = false;
            while (!touched) {
                touched = ts.tirqTouched() && ts.touched();
                TS_Point p = ts.getPoint();

                // minimum pressure
                if (p.z < TOUCH_PRESSURE_MIN) {
                    touched = false;
                }
            }
        } else {
            while (digitalRead(BTN_PIN)) {
                bool touched = ts.tirqTouched() && ts.touched();
                if (touched) {
                    TS_Point p = touchToScreen(ts.getPoint());
                    tft.drawPixel(p.x, p.y, TFT_WHITE);
                }
            }
            mem_write(config);
            return;
        }

        TS_Point p = ts.getPoint();
        if (step == 0) {
            config.touch_calibrate_left = p.x;
            config.touch_calibrate_top = p.y;
        } else if (step == 1) {
            config.touch_calibrate_right = p.x;
            config.touch_calibrate_bottom = p.y;
        }

        // TODO ugly, wait for proper touch release?
        tft.fillScreen(TFT_BLACK);
        delay(500);
    }
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

        case UI_MEMORY_READY: {
            if ((!digitalRead(BTN_PIN)) // button held at boot
                // ... or no calibration data in memory
                || ((config.touch_calibrate_left == 0)
                    && (config.touch_calibrate_right == 0)
                    && (config.touch_calibrate_top == 0)
                    && (config.touch_calibrate_bottom == 0))
            ) {
                ui_calibrate_touchscreen();
            }
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
            // get time via NTP
            configTime(gmtOffset_sec, daylightOffset_sec, NTP_SERVER);

            ui_page = UI_START;
            ui_draw_menu();
        } break;

        case UI_UPDATE: {
            if (curr_brightness >= set_max_brightness) {
                ui_draw_menu();
            }
        } break;
    }
}

void ui_run(void) {
    unsigned long now = millis();

    // go to info page when BOOT button is pressed
    if (!digitalRead(BTN_PIN)) {
        ui_page = UI_INFO;
    }

    // read out LDR in regular intervals
    if (now >= (last_ldr + LDR_CHECK_MS)) {
        last_ldr = now;
        int ldr = analogRead(LDR_PIN);

        ldr_value = (ldr_value * (1.0f - LDR_LOWPASS_FACT)) + (ldr * LDR_LOWPASS_FACT);

        // adjust backlight according to ldr
        int tmp = MIN(LDR_DARK_VALUE, MAX(0, ldr_value));
        set_max_brightness = map(tmp, LDR_DARK_VALUE, LDR_BRIGHT_VALUE, LCD_MIN_BRIGHTNESS, LCD_MAX_BRIGHTNESS);

        // refresh info page every 1s, it shows the LDR value
        static int cnt = 0;
        if ((ui_page == UI_INFO) && (++cnt >= (1000 / LDR_CHECK_MS))) {
            cnt = 0;
            ui_draw_menu();
        }
    }

    // adjust backlight brightness
    unsigned long diff = now - last_touch_time;
    if ((diff < FULL_BRIGHT_MS) || (ui_page == UI_INFO))  {
        curr_brightness = set_max_brightness;
    } else if (diff < (FULL_BRIGHT_MS + NO_BRIGHT_MS)) {
        curr_brightness = map(diff - FULL_BRIGHT_MS, 0, NO_BRIGHT_MS, set_max_brightness, STANDBY_BRIGHTNESS);
    } else {
        if ((curr_brightness > STANDBY_BRIGHTNESS) || ((now - last_standby_draw) >= STANDBY_REDRAW_MS)) {
            // enter standby screen
            draw_standby();
            last_standby_draw = now;
        }
        curr_brightness = STANDBY_BRIGHTNESS;
    }
    ledcAnalogWrite(LEDC_CHANNEL_0, curr_brightness);

    bool touched = ts.tirqTouched() && ts.touched();
    TS_Point p;

    if (touched) {
        p = touchToScreen(ts.getPoint());

        // minimum pressure
        if (p.z < TOUCH_PRESSURE_MIN) {
            touched = false;
        }
    }

    if (touched && (!is_touched)) {
        is_touched = true;
        last_touch_time = now;

        // skip touch event and just go back to full brightness
        if (curr_brightness < set_max_brightness) {
            tft.fillScreen(TFT_BLACK); // exit standby screen
            ui_draw_menu(); // re-draw normal screen contents
            return ui_run(); // skip touch and increase brightness
        }

        if (ui_page == UI_INFO) {
            // switch to next page, skip init and info screen
            do {
                ui_page = (enum ui_pages)((ui_page + 1) % UI_NUM_PAGES);
            } while ((ui_page == UI_START) || (ui_page == UI_INFO));
            tft.fillScreen(TFT_BLACK);

            ui_draw_menu();
            return;
        }

        if ((p.x >= BTNS_OFF_X) && (p.x <= BTNS_OFF_X + BTN_W) && (p.y >= BTNS_OFF_Y) && (p.y <= BTNS_OFF_Y + BTN_H)) {
            // 1
            if (ui_page == UI_LIVINGROOM1) {
                INVERT_BOOL(ui_status.light_corner);
            } else if (ui_page == UI_LIVINGROOM2) {
                INVERT_BOOL(ui_status.light_pc);
            } else if (ui_page == UI_BATHROOM) {
                ui_status.bathroom_lights = BATH_LIGHT_NONE;
            }
            writeMQTT_UI();
        } else if ((p.x >= BTNS_OFF_X) && (p.x <= BTNS_OFF_X + BTN_W) && (p.y >= (BTNS_OFF_Y + BTN_H + BTN_GAP)) && (p.y <= (BTNS_OFF_Y + BTN_H + BTN_GAP + BTN_H))) {
            // 2
            if (ui_page == UI_LIVINGROOM1) {
                INVERT_BOOL(ui_status.light_workspace);
            } else if (ui_page == UI_LIVINGROOM2) {
                INVERT_BOOL(ui_status.light_bench);
            } else if (ui_page == UI_BATHROOM) {
                ui_status.bathroom_lights = BATH_LIGHT_BIG;
            }
            writeMQTT_UI();
        } else if ((p.x >= BTNS_OFF_X) && (p.x <= BTNS_OFF_X + BTN_W) && (p.y >= (BTNS_OFF_Y + BTN_H * 2 + BTN_GAP * 2)) && (p.y <= (BTNS_OFF_Y + BTN_H * 2 + BTN_GAP * 2 + BTN_H))) {
            // 3
            if (ui_page == UI_LIVINGROOM1) {
                INVERT_BOOL(ui_status.light_sink);
            } else if (ui_page == UI_LIVINGROOM2) {
                INVERT_BOOL(ui_status.light_kitchen);
            } else if (ui_page == UI_BATHROOM) {
                INVERT_BOOL(ui_status.pc_displays);
            }
            writeMQTT_UI();
        } else if ((p.x >= BTNS_OFF_X + BTN_W + BTN_GAP) && (p.x <= BTNS_OFF_X + BTN_W + BTN_GAP + BTN_W) && (p.y >= BTNS_OFF_Y) && (p.y <= BTNS_OFF_Y + BTN_H)) {
            // 4
            if (ui_page == UI_LIVINGROOM1) {
                INVERT_BOOL(ui_status.sound_amplifier);
            } else if (ui_page == UI_LIVINGROOM2) {
                INVERT_BOOL(ui_status.light_amp);
            } else if (ui_page == UI_BATHROOM) {
                ui_status.bathroom_lights = BATH_LIGHT_OFF;
            }
            writeMQTT_UI();
        } else if ((p.x >= BTNS_OFF_X + BTN_W + BTN_GAP) && (p.x <= BTNS_OFF_X + BTN_W + BTN_GAP + BTN_W) && (p.y >= (BTNS_OFF_Y + BTN_H + BTN_GAP)) && (p.y <= (BTNS_OFF_Y + BTN_H + BTN_GAP + BTN_H))) {
            // 5
            if (ui_page == UI_LIVINGROOM1) {
                bool on = ui_status.light_corner || ui_status.light_sink || ui_status.light_workspace
                        || ui_status.light_amp || ui_status.light_bench || ui_status.light_box
                        || ui_status.light_kitchen || ui_status.light_pc || ui_status.pc_displays;
                if (on) {
                    ui_status.light_amp = false;
                    ui_status.light_kitchen = false;
                    ui_status.light_bench= false;
                    ui_status.light_workspace = false;
                    ui_status.light_pc = false;
                    ui_status.light_corner = false;
                    ui_status.light_box = false;
                    ui_status.light_sink = false;
                    ui_status.pc_displays = false;
                } else {
                    ui_status.light_corner = true;
                    ui_status.light_sink = true;
                    ui_status.pc_displays = true;
                }
            } else if (ui_page == UI_LIVINGROOM2) {
                INVERT_BOOL(ui_status.light_box);
            } else if (ui_page == UI_BATHROOM) {
                ui_status.bathroom_lights = BATH_LIGHT_SMALL;
            }
            writeMQTT_UI();
        } else if ((p.x >= BTNS_OFF_X + BTN_W + BTN_GAP) && (p.x <= BTNS_OFF_X + BTN_W + BTN_GAP + BTN_W) && (p.y >= (BTNS_OFF_Y + BTN_H * 2 + BTN_GAP * 2)) && (p.y <= (BTNS_OFF_Y + BTN_H * 2 + BTN_GAP * 2 + BTN_H))) {
            // switch to next page, skip init and info screen
            do {
                ui_page = (enum ui_pages)((ui_page + 1) % UI_NUM_PAGES);
            } while ((ui_page == UI_START) || (ui_page == UI_INFO));
            tft.fillScreen(TFT_BLACK);
        }

        ui_draw_menu();
    } else if ((!touched) && is_touched && ((now - last_touch_time) >= MIN_TOUCH_DELAY_MS)) {
        is_touched = false;
    }
}

#endif // FEATURE_UI
