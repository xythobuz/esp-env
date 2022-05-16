/*
 * moisture.h
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

#ifndef __ESP_ADC_MOISTURE_SENSOR__
#define __ESP_ADC_MOISTURE_SENSOR__

void moisture_init(void);
int moisture_count(void);
int moisture_read(int sensor);
int moisture_max(void);

#endif // __ESP_ADC_MOISTURE_SENSOR__
