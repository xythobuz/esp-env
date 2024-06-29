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
#include "lora.h"
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

#ifdef FEATURE_LORA
    lora_oled_print(s);
#endif // FEATURE_LORA

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

size_t DebugLog::printf(const char *format, va_list args) {
    char line_buff[128];
    int l = vsnprintf((char *)line_buff, sizeof(line_buff), format, args);

    if (l < 0) {
        // encoding error
        l = snprintf((char *)line_buff, sizeof(line_buff), "%s: encoding error\r\n", __func__);
    } else if (l >= (ssize_t)sizeof(line_buff)) {
        // not enough space for string
        l = snprintf((char *)line_buff, sizeof(line_buff), "%s: message too long (%d)\r\n", __func__, l);
    }
    if ((l > 0) && (l <= (int)sizeof(line_buff))) {
        print(String(line_buff));
    }

    return (l < 0) ? 0 : l;
}

size_t DebugLog::printf(const char * format, ...) {
    va_list args;
    va_start(args, format);
    size_t r = printf(format, args);
    va_end(args);
    return r;
}
