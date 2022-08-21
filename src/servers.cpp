/*
 * servers.cpp
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

#elif defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)

#include "SimpleUpdater.h"

UPDATE_WEB_SERVER server(80);
SimpleUpdater updater;

#ifdef ENABLE_WEBSOCKETS
#include <WebSocketsServer.h>
WebSocketsServer socket(81);
#endif // ENABLE_WEBSOCKETS

#elif defined(ARDUINO_ARCH_AVR)

#include <UnoWiFiDevEdSerial1.h>
#include <WiFiLink.h>

WiFiServer server(80);

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

static unsigned long last_server_handle_time = 0;

void wifi_send_websocket(String s) {
#ifdef ENABLE_WEBSOCKETS
    socket.broadcastTXT(s);
#endif // ENABLE_WEBSOCKETS
}

#ifdef FEATURE_RELAIS

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
static void handleOn() {
#else
static void handleOn(WiFiClient &client) {
#endif
    String id_string = server.arg("id");
    int id = id_string.toInt();

    if ((id >= 0) && (id < relais_count())) {
        relais_set(id, 1);
    } else {
        for (int i = 0; i < relais_count(); i++) {
            relais_set(i, 1);
        }
    }

    writeDatabase();

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    handlePage(1, id);
#else
    handlePage(client, 1, id);
#endif
}

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
static void handleOff() {
#else
static void handleOff(WiFiClient &client) {
#endif
    String id_string = server.arg("id");
    int id = id_string.toInt();

    if ((id >= 0) && (id < relais_count())) {
        relais_set(id, 0);
    } else {
        for (int i = 0; i < relais_count(); i++) {
            relais_set(i, 0);
        }
    }

    writeDatabase();

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    handlePage(0, id);
#else
    handlePage(client, 0, id);
#endif
}

#endif // FEATURE_RELAIS

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
static void handleRoot() {
    handlePage();
#else
static void handleRoot(WiFiClient &client) {
    handlePage(client);
#endif
}

void initServers(String hostname) {
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    // Setup HTTP Server
    MDNS.begin(hostname.c_str());
    updater.setup(&server);
    server.on("/", handleRoot);
    server.on("/reset", handleReset);
    server.on("/calibrate", handleCalibrate);

#ifdef FEATURE_RELAIS
    server.on("/on", handleOn);
    server.on("/off", handleOff);
#endif // FEATURE_RELAIS

    MDNS.addService("http", "tcp", 80);

#ifdef ENABLE_WEBSOCKETS
    socket.begin();
#endif // ENABLE_WEBSOCKETS
#endif

    server.begin();
}

#if defined(ARDUINO_ARCH_AVR)
static void http_server() {
    // listen for incoming clients
    WiFiClient client = server.available();
    if (client) {
        debug.println(F("new http client"));

        // an http request ends with a blank line
        boolean currentLineIsBlank = true;

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                debug.write(c);

                // if you've gotten to the end of the line (received a newline
                // character) and the line is blank, the http request has ended,
                // so you can send a reply
                if ((c == '\n') && currentLineIsBlank) {
                    // send a standard http response header
                    client.println(F("HTTP/1.1 200 OK"));
                    client.println(F("Content-Type: text/html"));
                    client.println(F("Connection: close"));
                    client.println();

                    // TODO parse path and handle different pages
                    handleRoot(client);

                    break;
                }

                if (c == '\n') {
                    // you're starting a new line
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    // you've gotten a character on the current line
                    currentLineIsBlank = false;
                }
            }
        }

        // give the web browser time to receive the data
        delay(10);

        // close the connection
        client.stop();
        debug.println(F("http client disconnected"));
    }
}
#endif

static void handleServers() {
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    server.handleClient();
#else
    http_server();
#endif
    
#if defined(ARDUINO_ARCH_ESP8266)
    MDNS.update();
#endif
}

void runServers() {
    unsigned long time = millis();

    if ((time - last_server_handle_time) >= SERVER_HANDLE_INTERVAL) {
        last_server_handle_time = time;
        handleServers();

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#ifdef ENABLE_WEBSOCKETS
        socket.loop();
#endif // ENABLE_WEBSOCKETS
#endif
    }
}
