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

#ifdef ENABLE_DEBUGLOG
#include <CircularBuffer.h>
#define DEBUG_LOG_HISTORY_SIZE 1024
#endif // ENABLE_DEBUGLOG

class DebugLog {
public:
#ifdef ENABLE_DEBUGLOG
    String getBuffer(void);
#endif // ENABLE_DEBUGLOG
    
    void write(char c);
    void print(String s);
    void print(int n);
    
    void println(void);
    void println(String s);
    void println(int n);
    
private:
    void sendToTargets(String s);
    
#ifdef ENABLE_DEBUGLOG
    void addToBuffer(String s);
    
    CircularBuffer<char, DEBUG_LOG_HISTORY_SIZE> buffer;
#endif // ENABLE_DEBUGLOG
};

extern DebugLog debug;

#endif // _DEBUG_LOG_H_
