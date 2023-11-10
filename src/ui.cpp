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

void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255) {
    uint32_t duty = (4095 / valueMax) * min(value, valueMax);
    ledcWrite(channel, duty);
}

SPIClass mySpi = SPIClass(HSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

TFT_eSPI tft = TFT_eSPI();

void ui_init(void) {
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);

    tft.init();
    tft.setRotation(1);

    ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
    ledcAttachPin(TFT_BL, LEDC_CHANNEL_0);
    ledcAnalogWrite(LEDC_CHANNEL_0, 255);

    tft.fillScreen(TFT_BLACK);

    int x = LCD_WIDTH / 2;
    int y = LCD_HEIGHT / 2;
    int fontSize = 2;
    tft.drawCentreString("Initializing ESP-ENV", x, y - 16, fontSize);
    tft.drawCentreString("xythobuz.de", x, y + 16, fontSize);
}

void printTouchToDisplay(TS_Point p) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    int x = LCD_WIDTH / 2;
    int y = 100;
    int fontSize = 2;

    String temp = "Pressure = " + String(p.z);
    tft.drawCentreString(temp, x, y, fontSize);

    y += 16;
    temp = "X = " + String(p.x);
    tft.drawCentreString(temp, x, y, fontSize);

    y += 16;
    temp = "Y = " + String(p.y);
    tft.drawCentreString(temp, x, y, fontSize);
}

void ui_run(void) {
    if (ts.tirqTouched() && ts.touched()) {
        TS_Point p = ts.getPoint();
        printTouchToDisplay(p);

        if (p.x < 1000) {
            writeMQTTtopic("livingroom/light_kitchen", "off");
        } else if (p.x > 3000) {
            writeMQTTtopic("livingroom/light_kitchen", "on");
        }

        delay(1000);
    }
}

#endif // FEATURE_UI