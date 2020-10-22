/*
 * relais.h
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

#ifndef __ESP_RELAIS_ACTOR__
#define __ESP_RELAIS_ACTOR__

int relais_count(void);
int relais_enable(int relais, unsigned long time);
void relais_init(void);
void relais_run(void);

#endif // __ESP_RELAIS_ACTOR__

