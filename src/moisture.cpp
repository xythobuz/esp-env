/*
 * moisture.cpp
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

#include <Arduino.h>

#include "config.h"
#include "moisture.h"

#if defined(MOISTURE_ADC_ESP32)

#include <driver/adc.h>

#define ADC_OVERSAMPLE 20
#define ADC_BITWIDTH 12
#define ADC_BITS ADC_WIDTH_BIT_12
#define ADC_ATTENUATION ADC_ATTEN_DB_11

#define SENSOR_COUNT 6
adc1_channel_t sensor_pin[SENSOR_COUNT] = {
    ADC1_CHANNEL_0, ADC1_CHANNEL_3,
    ADC1_CHANNEL_6, ADC1_CHANNEL_7,
    ADC1_CHANNEL_4, ADC1_CHANNEL_5
};

static int adc_read_oversampled(adc1_channel_t pin) {
    uint32_t sample_sum = 0;

    for (int i = 0; i < ADC_OVERSAMPLE; i++) {
        sample_sum += adc1_get_raw(pin);
    }

    return sample_sum / ADC_OVERSAMPLE;
}

void moisture_init(void) {
    adc1_config_width(ADC_BITS);
    for (int i = 0; i < SENSOR_COUNT; i++) {
        adc1_config_channel_atten(sensor_pin[i], ADC_ATTENUATION);
    }
}

int moisture_count(void) {
    return SENSOR_COUNT;
}

int moisture_read(int sensor) {
    if ((sensor < 0) || (sensor > SENSOR_COUNT)) {
        return -1;
    }

    return adc_read_oversampled(sensor_pin[sensor]);
}

int moisture_max(void) {
    return (1 << ADC_BITWIDTH) - 1;
}

#elif defined(MOISTURE_ADC_ARDUINO)

// Hardware I2C pins A4 and A5 not usable on Arduino Uno Wifi Dev. Ed.
#define SENSOR_COUNT 6
uint8_t sensor_pins[SENSOR_COUNT] = { A0, A1, A1, A2, A3, A3 };

#define OVERSAMPLE_COUNT 3

void moisture_init(void) {

}

int moisture_count(void) {
    return SENSOR_COUNT;
}

int moisture_read(int sensor) {
    unsigned long val = 0;
    for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
        val += analogRead(sensor_pins[sensor]);
    }
    val /= OVERSAMPLE_COUNT;
    if (val >= 1010) val = 1023; // avoid noisy data in db
    return (int)((val << 2) | 3);
}

int moisture_max(void) {
    return 4095;
}

#else

void moisture_init(void) { }

int moisture_count(void) {
    return 0;
}

int moisture_read(int sensor) {
    return 0;
}

int moisture_max(void) {
    return 0;
}

#endif
