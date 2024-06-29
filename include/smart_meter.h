/*
 * smart_meter.h
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

#ifndef __ESP_ENV_SMART_METER__
#define __ESP_ENV_SMART_METER__

#ifdef FEATURE_SML

void sml_init(void);
void sml_run(void);

#endif // FEATURE_SML

#endif // __ESP_ENV_SMART_METER__
