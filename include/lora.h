/*
 * lora.h
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

#ifndef __ESP_ENV_LORA__
#define __ESP_ENV_LORA__

#ifdef FEATURE_LORA

void lora_oled_init(void);
void lora_oled_print(String s);

void lora_init(void);
void lora_run(void);

#endif // FEATURE_LORA

#endif // __ESP_ENV_LORA__
