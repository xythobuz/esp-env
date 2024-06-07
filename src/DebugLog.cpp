/*
 * DebugLog.cpp
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
#include "servers.h"
#include "DebugLog.h"

DebugLog debug;

#ifdef ENABLE_DEBUGLOG

String DebugLog::getBuffer(void) {
    String r;
    for (unsigned int i = 0; i < buffer.size(); i++) {
        r += buffer[i];
    }
    return r;
}

void DebugLog::addToBuffer(String s) {
    for (unsigned int i = 0; i < s.length(); i++) {
        buffer.push(s[i]);
    }
}

#endif

void DebugLog::sendToTargets(String s) {
    Serial.print(s);

    s = "log:" + s;
    wifi_send_websocket(s);
}

void DebugLog::write(char c) {
    print(String(c));
}

void DebugLog::print(String s) {
#ifdef ENABLE_DEBUGLOG
    addToBuffer(s);
#endif // ENABLE_DEBUGLOG

    sendToTargets(s);
}

void DebugLog::print(int n) {
    print(String(n));
}

void DebugLog::println(void) {
    print(String(F("\r\n")));
}

void DebugLog::println(String s) {
    s += String(F("\r\n"));
    print(s);
}

void DebugLog::println(int n) {
    println(String(n));
}
