/*
 * html.cpp
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
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#define ESP_PLATFORM_NAME "ESP8266"

#elif defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#define ESP_PLATFORM_NAME "ESP32"

#elif defined(ARDUINO_ARCH_AVR)

#include <UnoWiFiDevEdSerial1.h>
#include <WiFiLink.h>
#define ESP_PLATFORM_NAME "Uno WiFi"

#endif

#include "config.h"
#include "DebugLog.h"
#include "sensors.h"
#include "servers.h"
#include "memory.h"
#include "relais.h"
#include "moisture.h"
#include "html.h"

#if defined(ARDUINO_ARCH_AVR)
#define ARDUINO_SEND_PARTIAL_PAGE() do { \
        size_t len = message.length(), off = 0; \
        while (off < len) { \
            if ((len - off) >= 50) { \
                client.write(message.c_str() + off, 50); \
                off += 50; \
            } else { \
                client.write(message.c_str() + off, len - off); \
                off = len; \
            } \
        } \
        message = ""; \
    } while (false);
#else
#define ARDUINO_SEND_PARTIAL_PAGE() while (false) { }
#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void handlePage(int mode, int id) {
#else
void handlePage(WiFiClient &client, int mode, int id) {
#endif
    String message;

    message += F("<!DOCTYPE html>");
    message += F("<html><head>");
    message += F("<meta charset='utf-8'/>");
    message += F("<meta name='viewport' content='width=device-width, initial-scale=1'/>");
    message += F("<title>" ESP_PLATFORM_NAME " Environment Sensor</title>");
    message += F("<style>");
    message += F(".log {\n");
    message += F(    "max-height: 300px;\n");
    message += F(    "padding: 0 1.0em;\n");
    message += F(    "max-width: 1200px;\n");
    message += F(    "margin: auto;\n");
    message += F(    "margin-top: 1.5em;\n");
    message += F(    "border: 1px dashed black;\n");
    message += F(    "font-family: monospace;\n");
    message += F(    "overflow-y: scroll;\n");
    message += F(    "word-break: break-all;\n");
    message += F("}\n");
    message += F("#logbuf {\n");
    message += F(    "white-space: break-spaces;\n");
    message += F("}\n");
    message += F("@media (prefers-color-scheme: dark) {");
    message += F(    "body {");
    message += F(        "background-color: black;");
    message += F(        "color: white;");
    message += F(    "}");
    message += F(    ".log {\n");
    message += F(        "border-color: white;");
    message += F(    "}");
    message += F("}");
    message += F("</style>");
    message += F("</head><body>");
    message += F("<h1>" ESP_PLATFORM_NAME " Environment Sensor</h1>");
    message += F("\n<p>\n");
    message += F("Version: ");
    message += ESP_ENV_VERSION;
    message += F("\n<br>\n");
    message += F("Build Date: ");
    message += __DATE__;
    message += F("\n<br>\n");
    message += F("Build Time: ");
    message += __TIME__;
    message += F("\n<br>\n");
    message += F("Location: ");
    message += SENSOR_LOCATION;
    message += F("\n<br>\n");
    message += F("ID: ");
    message += SENSOR_ID;

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    message += F("\n<br>\n");
    message += F("MAC: ");
    message += WiFi.macAddress();
#endif

    message += F("\n</p>\n");

    ARDUINO_SEND_PARTIAL_PAGE();

#if defined(ARDUINO_ARCH_ESP8266)
    
    message += F("<p>");
    message += F("Reset reason: ");
    message += ESP.getResetReason();
    message += F("<br>");
    message += F("Free heap: ");
    message += String(ESP.getFreeHeap());
    message += F(" (");
    message += String(ESP.getHeapFragmentation());
    message += F("% fragmentation)");
    message += F("<br>");
    message += F("Free sketch space: ");
    message += String(ESP.getFreeSketchSpace());
    message += F("<br>");
    message += F("Flash chip real size: ");
    message += String(ESP.getFlashChipRealSize());

    if (ESP.getFlashChipSize() != ESP.getFlashChipRealSize()) {
        message += F("<br>");
        message += F("WARNING: sdk chip size (");
        message += (ESP.getFlashChipSize());
        message += F(") does not match!");
    }
    
    message += F("</p>");
    
#elif defined(ARDUINO_ARCH_ESP32)

    message += F("<p>");
    message += F("Free heap: ");
    message += String(ESP.getFreeHeap() / 1024.0);
    message += F("k<br>");
    message += F("Free sketch space: ");
    message += String(ESP.getFreeSketchSpace() / 1024.0);
    message += F("k<br>");
    message += F("Flash chip size: ");
    message += String(ESP.getFlashChipSize() / 1024.0);
    message += F("k</p><hr>");
    
#endif

#ifdef ENABLE_BME280

    message += F("\n<p>\n");
    if (found_bme1) {
        message += F("BME280 Low:");
        message += F("\n<br>\n");
        message += F("Temperature: ");
        message += String(bme1_temp());
        message += F("\n<br>\n");
        message += F("Humidity: ");
        message += String(bme1_humid());
        message += F("\n<br>\n");
        message += F("Pressure: ");
        message += String(bme1_pressure());
        message += F("\n<br>\n");
        message += F("Offset: ");
        message += String(config.bme1_temp_off);
        message += F("\n<br>\n");
        message += F("<form method=\"GET\" action=\"/calibrate\">");
        message += F("<input type=\"text\" name=\"bme1\" placeholder=\"Real Temp.\">");
        message += F("<input type=\"submit\" value=\"Calibrate\">");
        message += F("</form>");
    } else {
        message += F("BME280 (low) not connected!");
    }
    message += F("\n</p><hr>\n");

    message += F("\n<p>\n");
    if (found_bme2) {
        message += F("BME280 High:");
        message += F("\n<br>\n");
        message += F("Temperature: ");
        message += String(bme2_temp());
        message += F("\n<br>\n");
        message += F("Humidity: ");
        message += String(bme2_humid());
        message += F("\n<br>\n");
        message += F("Pressure: ");
        message += String(bme2_pressure());
        message += F("\n<br>\n");
        message += F("Offset: ");
        message += String(config.bme2_temp_off);
        message += F("\n<br>\n");
        message += F("<form method=\"GET\" action=\"/calibrate\">");
        message += F("<input type=\"text\" name=\"bme2\" placeholder=\"Real Temp.\">");
        message += F("<input type=\"submit\" value=\"Calibrate\">");
        message += F("</form>");
    } else {
        message += F("BME280 (high) not connected!");
    }
    message += F("\n</p><hr>\n");

#endif // ENABLE_BME280

    ARDUINO_SEND_PARTIAL_PAGE();

    message += F("\n<p>\n");
    if (found_sht) {
        message += F("SHT21:");
        message += F("\n<br>\n");
        message += F("Temperature: ");
        message += String(sht_temp());
        message += F("\n<br>\n");
        message += F("Humidity: ");
        message += String(sht_humid());
        message += F("\n<br>\n");
        message += F("Offset: ");
        message += String(config.sht_temp_off);
        message += F("\n<br>\n");
        message += F("<form method=\"GET\" action=\"/calibrate\">");
        message += F("<input type=\"text\" name=\"sht\" placeholder=\"Real Temp.\">");
        message += F("<input type=\"submit\" value=\"Calibrate\">");
        message += F("</form>");
    } else {
        message += F("SHT21 not connected!");
    }
    message += F("\n</p><hr>\n");

#ifdef ENABLE_CCS811

    message += F("\n<p>\n");
    if (found_ccs1) {
        message += F("CCS811 Low:");
        message += F("\n<br>\n");
        message += F("eCO2: ");
        message += String(ccs1_eco2());
        message += F("ppm");
        message += F("\n<br>\n");
        message += F("TVOC: ");
        message += String(ccs1_tvoc());
        message += F("ppb");

        if (!ccs1_data_valid) {
            message += F("\n<br>\n");
            message += F("Data invalid (");
            message += String(ccs1_error_code);
            message += F(")!");
        }
    } else {
        message += F("CCS811 (Low) not connected!");
    }
    message += F("\n</p><hr>\n");

    message += F("\n<p>\n");
    if (found_ccs2) {
        message += F("CCS811 High:");
        message += F("\n<br>\n");
        message += F("eCO2: ");
        message += String(ccs2_eco2());
        message += F("ppm");
        message += F("\n<br>\n");
        message += F("TVOC: ");
        message += String(ccs2_tvoc());
        message += F("ppb");

        if (!ccs2_data_valid) {
            message += F("\n<br>\n");
            message += F("Data invalid (");
            message += String(ccs2_error_code);
            message += F(")!");
        }
    } else {
        message += F("CCS811 (High) not connected!");
    }
    message += F("\n</p><hr>\n");

#endif // ENABLE_CCS811

    ARDUINO_SEND_PARTIAL_PAGE();

#ifdef FEATURE_MOISTURE
    for (int i = 0; i < moisture_count(); i++) {
        int moisture = moisture_read(i);
        if (moisture < moisture_max()) {
            message += F("\n<p>\n");
            message += F("Sensor ");
            message += String(i + 1);
            message += F(":\n<br>\n");
            message += F("Moisture: ");
            message += String(moisture);
            message += F(" / ");
            message += String(moisture_max());
            message += F("\n</p>\n");
        }
    }
    
    if (moisture_count() <= 0) {
        message += F("\n<p>\n");
        message += F("No moisture sensors configured!");
        message += F("\n</p>\n");
    }

    ARDUINO_SEND_PARTIAL_PAGE();
#endif // FEATURE_MOISTURE

#ifdef FEATURE_RELAIS
    message += F("\n<p>\n");
    for (int i = 0; i < relais_count(); i++) {
        message += String(F("<a href=\"/on?id=")) + String(i) + String(F("\">Relais ")) + String(i) + String(F(" On (")) + relais_name(i) + String(F(")</a><br>\n"));
        message += String(F("<a href=\"/off?id=")) + String(i) + String(F("\">Relais ")) + String(i) + String(F(" Off (")) + relais_name(i) + String(F(")</a><br>\n"));
    }
    message += String(F("<a href=\"/on?id=")) + String(relais_count()) + String(F("\">All Relais On</a><br>\n"));
    message += String(F("<a href=\"/off?id=")) + String(relais_count()) + String(F("\">All Relais Off</a><br>\n"));
    message += F("</p>\n");

    if ((mode >= 0) && (mode <= 1)) {
        message += F("<p>");
        message += F("Turned Relais ");
        message += (id < relais_count()) ? String(id) : String(F("1-4"));
        message += (mode ? String(F(" On")) : String(F(" Off")));
        message += F("</p>\n");
    }

    message += F("\n<p>\n");
    for (int i = 0; i < relais_count(); i++) {
        message += String(F("Relais ")) + String(i) + String(F(" (")) + relais_name(i) + String(F(") = ")) + (relais_get(i) ? String(F("On")) : String(F("Off"))) + String(F("<br>\n"));
    }
    message += F("</p>\n");

    ARDUINO_SEND_PARTIAL_PAGE();
#endif // FEATURE_RELAIS

    if (mode == 42) {
        message += F("<p>New calibration value saved!</p>\n");
    }

#if ! defined(ARDUINO_ARCH_AVR)
    message += F("<p>");
    message += F("Try <a href=\"/update\">/update</a> for OTA firmware updates!");
    message += F("</p><p>");
    message += F("Try <a href=\"/reset\">/reset</a> to reset ESP!");
    message += F("</p>");
#endif
    
    message += F("<p>");
#ifdef ENABLE_INFLUXDB_LOGGING
    message += F("InfluxDB: ");
    message += INFLUXDB_DATABASE;
    message += F(" @ ");
    message += INFLUXDB_HOST;
    message += F(":");
    message += String(INFLUXDB_PORT);
#else
    message += F("InfluxDB logging not enabled!");
#endif
    message += F("</p>");

    message += F("<p>Uptime: ");
    message += String(millis() / 1000);
    message += F(" sec.</p>");

#ifdef ENABLE_DEBUGLOG
    message += F("<hr><p>Debug Log:</p>");
    message += F("<div class='log'><pre id='logbuf'>");
    message += debug.getBuffer();
    message += F("</pre></div>");
#endif // ENABLE_DEBUGLOG

    message += F("</body>");

#ifdef ENABLE_WEBSOCKETS
    message += F("<script type='text/javascript'>\n");
    message += F("var socket = new WebSocket('ws://' + window.location.hostname + ':81');\n");
    message += F("socket.onmessage = function(e) {");
    message += F(    "var log = document.getElementById('logbuf');");
    message += F(    "var div = document.getElementsByClassName('log')[0];");
    message += F(    "log.innerHTML += e.data.substring(4);");
    message += F(    "if (log.innerHTML.length > (1024 * 1024)) {");
    message += F(        "log.innerHTML = log.innerHTML.substring(1024 * 1024);");
    message += F(    "}");
    message += F(    "div.scrollTop = div.scrollHeight;");
    message += F("};\n");
    message += F("var hist = document.getElementsByClassName('log')[0];\n");
    message += F("hist.scrollTop = hist.scrollHeight;\n");
    message += F("</script>\n");
#endif // ENABLE_WEBSOCKETS

    message += F("</html>");

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    server.send(200, "text/html", message);
#else
    ARDUINO_SEND_PARTIAL_PAGE();
#endif
}

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void handleReset() {
    String message;
    message += F("<!DOCTYPE html>");
    message += F("<html><head>");
    message += F("<meta charset='utf-8'/>");
    message += F("<meta name='viewport' content='width=device-width, initial-scale=1'/>");
    message += F("<meta http-equiv='refresh' content='10; URL=/'/>");
    message += F("<title>" ESP_PLATFORM_NAME " Environment Sensor</title>");
    message += F("<style>");
    message += F("@media (prefers-color-scheme: dark) {");
    message += F(    "body {");
    message += F(        "background-color: black;");
    message += F(        "color: white;");
    message += F(    "}");
    message += F("}");
    message += F("</style>");
    message += F("</head><body>");
    message += F("<p>Resetting in 2s...</p>");
    message += F("<p>Auto redirect in 10s. Please retry manually on error.</p>");
    message += F("<p>Go <a href=\"/\">back</a> to start.</p>");
    message += F("</body></html>");
    server.send(200, "text/html", message);

    delay(2000);
    ESP.restart();
}
#endif
