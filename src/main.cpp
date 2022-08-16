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

unsigned long last_led_blink_time = 0;

ConfigMemory config;

void setup() {
    pinMode(BUILTIN_LED_PIN, OUTPUT);
    
    Serial.begin(115200);

    // Blink LED for init
    for (int i = 0; i < 2; i++) {
        digitalWrite(BUILTIN_LED_PIN, LOW); // LED on
        delay(LED_INIT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, HIGH); // LED off
        delay(LED_INIT_BLINK_INTERVAL);
    }

    config = mem_read();

    debug.println(F("Relais"));
    relais_init();
    
    debug.println(F("Moisture"));
    moisture_init();

    initSensors();

    // Build hostname string
    String hostname = SENSOR_HOSTNAME_PREFIX;
    hostname += SENSOR_ID;

#if defined(ARDUINO_ARCH_ESP8266)

    // Connect to WiFi AP
    debug.print(F("Connecting WiFi"));
    WiFi.hostname(hostname);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        debug.print(F("."));
    }
    debug.println(F("\nWiFi connected!"));
    
#elif defined(ARDUINO_ARCH_ESP32)

    // Set hostname workaround
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(hostname.c_str());
    
    // Workaround for WiFi connecting only every 2nd reset
    // https://github.com/espressif/arduino-esp32/issues/2501#issuecomment-513602522
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (info.disconnected.reason == 202) {
            esp_sleep_enable_timer_wakeup(10);
            esp_deep_sleep_start();
            delay(100);
        }
    }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

    // Connect to WiFi AP
    debug.print(F("Connecting WiFi"));
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        debug.print(F("."));
    }
    debug.println(F("\nWiFi connected!"));
    
    // Set hostname workaround
    WiFi.setHostname(hostname.c_str());

#elif defined(ARDUINO_ARCH_AVR)

    Serial1.begin(115200);

    WiFi.init(&Serial1);

    debug.print(F("Connecting WiFi"));
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        debug.print(F("."));
    }
    debug.println(F("\nWiFi connected!"));

#endif

    debug.println(F("Seeding"));
    randomSeed(micros());

    initMQTT();
    initInflux();
    initServers(hostname);
}

void loop() {
    unsigned long time = millis();

    runServers();
    runSensors();
    runMQTT();
    runInflux();

    // blink heartbeat LED
    if ((time - last_led_blink_time) >= LED_BLINK_INTERVAL) {
        last_led_blink_time = time;
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
    }
    
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    // reset ESP every 3d to be safe
    if (time >= (3UL * 24UL * 60UL * 60UL * 1000UL)) {
        ESP.restart();
    }
#endif
}
