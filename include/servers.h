/*
 * servers.h
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

#ifndef __SERVERS_H__
#define __SERVERS_H__

void initServers(String hostname);
void runServers();

void wifi_send_websocket(String s);

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#include "SimpleUpdater.h"
extern UPDATE_WEB_SERVER server;
#elif defined(ARDUINO_ARCH_AVR)
#include <WiFiLink.h>
extern WiFiServer server;
#endif

#endif // __SERVERS_H__
