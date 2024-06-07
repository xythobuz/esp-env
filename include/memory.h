/*
 * memory.h
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

#ifndef __ESP_ENV_MEMORY__
#define __ESP_ENV_MEMORY__

struct ConfigMemory {
    double sht_temp_off;
    double bme1_temp_off;
    double bme2_temp_off;

#ifdef FEATURE_UI
    int touch_calibrate_left;
    int touch_calibrate_right;
    int touch_calibrate_top;
    int touch_calibrate_bottom;
#endif // FEATURE_UI

    ConfigMemory() {
        sht_temp_off = 0.0;
        bme1_temp_off = 0.0;
        bme2_temp_off = 0.0;

#ifdef FEATURE_UI
        touch_calibrate_left = 0;
        touch_calibrate_right = 0;
        touch_calibrate_top = 0;
        touch_calibrate_bottom = 0;
#endif // FEATURE_UI
    }
};

ConfigMemory mem_read();
void mem_write(ConfigMemory mem);

extern ConfigMemory config;

#endif // __ESP_ENV_MEMORY__
