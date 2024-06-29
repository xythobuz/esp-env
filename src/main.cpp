/*
 * main.cpp
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

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#elif defined(ARDUINO_ARCH_AVR)
#include <UnoWiFiDevEdSerial1.h>
#include <WiFiLink.h>
#endif

#include "config.h"
#include "DebugLog.h"
#include "moisture.h"
#include "sensors.h"
#include "relais.h"
#include "memory.h"
#include "influx.h"
#include "mqtt.h"
#include "html.h"
#include "servers.h"
#include "ui.h"
#include "lora.h"

unsigned long last_led_blink_time = 0;

ConfigMemory config;

#if defined(ARDUINO_ARCH_ESP8266)

WiFiEventHandler disconnectHandler;

void onDisconnected(const WiFiEventStationModeDisconnected& event) {
    /*
     * simply restart in case we lose wifi connection
     * we can't do anything useful without wifi anyway!
     */
    ESP.restart();
}

#endif // ARDUINO_ARCH_ESP8266

void setup() {
    pinMode(BUILTIN_LED_PIN, OUTPUT);

    Serial.begin(115200);

    debug.println(F("Initializing..."));

    // Blink LED for init
    for (int i = 0; i < 2; i++) {
        digitalWrite(BUILTIN_LED_PIN, LOW); // LED on
        delay(LED_INIT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, HIGH); // LED off
        delay(LED_INIT_BLINK_INTERVAL);
    }

#ifdef FEATURE_UI
    debug.println(F("UI"));
    ui_init();
#endif // FEATURE_UI

    config = mem_read();

#ifdef FEATURE_UI
    ui_progress(UI_MEMORY_READY);
#endif // FEATURE_UI

#ifdef FEATURE_RELAIS
    debug.println(F("Relais"));
    relais_init();
#endif // FEATURE_RELAIS

#ifdef FEATURE_MOISTURE
    debug.println(F("Moisture"));
    moisture_init();
#endif // FEATURE_MOISTURE

    debug.println(F("Sensors"));
    initSensors();

#ifdef FEATURE_LORA
    debug.println(F("Lora"));
    lora_init();
#endif // FEATURE_LORA

#ifndef FEATURE_DISABLE_WIFI

    // Build hostname string
    String hostname = SENSOR_HOSTNAME_PREFIX;
    hostname += SENSOR_ID;

#if defined(ARDUINO_ARCH_ESP8266)

    // Connect to WiFi AP
    debug.print(F("Connecting WiFi"));
#ifdef FEATURE_UI
    ui_progress(UI_WIFI_CONNECT);
#endif // FEATURE_UI
    WiFi.hostname(hostname);
    WiFi.mode(WIFI_STA);
    WiFi.hostname(hostname);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        debug.print(F("."));
#ifdef FEATURE_UI
        ui_progress(UI_WIFI_CONNECTING);
#endif // FEATURE_UI
    }
    debug.println(F("\nWiFi connected!"));
#ifdef FEATURE_UI
    ui_progress(UI_WIFI_CONNECTED);
#endif // FEATURE_UI

    disconnectHandler = WiFi.onStationModeDisconnected(onDisconnected);

    // Set hostname workaround
    WiFi.hostname(hostname);

#elif defined(ARDUINO_ARCH_ESP32)

    // Set hostname workaround
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(hostname.c_str());

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        /*
         * was initially: workaround for WiFi connecting only every 2nd reset
         * https://github.com/espressif/arduino-esp32/issues/2501#issuecomment-513602522
         *
         * now simply reset on every disconnect reason - we can't do anything
         * useful without wifi anyway!
         */
        esp_sleep_enable_timer_wakeup(10);
        esp_deep_sleep_start();
        delay(100);
        ESP.restart();
#ifdef NEW_ESP32_LIB
    }, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
#else
    }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);
#endif

    // Connect to WiFi AP
    debug.print(F("Connecting WiFi"));
#ifdef FEATURE_UI
    ui_progress(UI_WIFI_CONNECT);
#endif // FEATURE_UI
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(hostname.c_str());
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        debug.print(F("."));
#ifdef FEATURE_UI
        ui_progress(UI_WIFI_CONNECTING);
#endif // FEATURE_UI
    }
    debug.println(F("\nWiFi connected!"));
#ifdef FEATURE_UI
    ui_progress(UI_WIFI_CONNECTED);
#endif // FEATURE_UI

    // Set hostname workaround
    WiFi.setHostname(hostname.c_str());

#elif defined(ARDUINO_ARCH_AVR)

    Serial1.begin(115200);

    WiFi.init(&Serial1);

    debug.print(F("Connecting WiFi"));
#ifdef FEATURE_UI
    ui_progress(UI_WIFI_CONNECT);
#endif // FEATURE_UI
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        debug.print(F("."));
#ifdef FEATURE_UI
        ui_progress(UI_WIFI_CONNECTING);
#endif // FEATURE_UI
    }
    debug.println(F("\nWiFi connected!"));
#ifdef FEATURE_UI
    ui_progress(UI_WIFI_CONNECTED);
#endif // FEATURE_UI

#endif // ARCH

    debug.println(F("Seeding"));
    randomSeed(micros());

    debug.println(F("MQTT"));
    initMQTT();

    debug.println(F("Influx"));
    initInflux();

    debug.println(F("Servers"));
    initServers(hostname);

#endif // FEATURE_DISABLE_WIFI

    debug.println(F("Ready! Starting..."));

#ifdef FEATURE_UI
    debug.println(F("UI Go"));
    ui_progress(UI_READY);
#endif // FEATURE_UI
}

void loop() {
    runSensors();

#ifndef FEATURE_DISABLE_WIFI
    runServers();
    runMQTT();
    runInflux();
#endif // FEATURE_DISABLE_WIFI

#ifdef FEATURE_UI
    ui_run();
#endif

#ifdef FEATURE_LORA
    lora_run();
#endif // FEATURE_LORA

    // blink heartbeat LED
    unsigned long time = millis();
    if ((time - last_led_blink_time) >= LED_BLINK_INTERVAL) {
        last_led_blink_time = time;
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
    }
}
