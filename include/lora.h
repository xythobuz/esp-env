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

#ifdef FEATURE_SML

enum lora_sml_type {
    LORA_SML_HELLO = 0,
    LORA_SML_SUM_WH,
    LORA_SML_T1_WH,
    LORA_SML_T2_WH,
    LORA_SML_SUM_W,
    LORA_SML_L1_W,
    LORA_SML_L2_W,
    LORA_SML_L3_W,
    LORA_SML_BAT_V,

    LORA_SML_NUM_MESSAGES
};

void lora_sml_send(enum lora_sml_type msg, double value, unsigned long counter);

#endif // FEATURE_SML

#endif // FEATURE_LORA

#endif // __ESP_ENV_LORA__
