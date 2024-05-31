/*
 * mqtt.h
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

#ifndef __MQTT_H__
#define __MQTT_H__

void initMQTT();
void runMQTT();

#ifdef FEATURE_UI
void writeMQTT_UI(void);
#endif // FEATURE_UI

#endif // __MQTT_H__
