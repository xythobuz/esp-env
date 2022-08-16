/*
 * DebugLog.h
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

#ifndef _DEBUG_LOG_H_
#define _DEBUG_LOG_H_

#include <Arduino.h>

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#include <CircularBuffer.h>
#define DEBUG_LOG_HISTORY_SIZE 1024
#endif

class DebugLog {
public:
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    String getBuffer(void);
#endif
    
    void write(char c);
    void print(String s);
    void print(int n);
    
    void println(void);
    void println(String s);
    void println(int n);
    
private:
    void sendToTargets(String s);
    
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    void addToBuffer(String s);
    
    CircularBuffer<char, DEBUG_LOG_HISTORY_SIZE> buffer;
#endif
};

extern DebugLog debug;

#endif // _DEBUG_LOG_H_
