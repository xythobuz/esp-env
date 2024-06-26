/*
 * sensors.cpp
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

#include <SHT2x.h>

#ifdef ENABLE_BME280
#include <Adafruit_BME280.h>
#endif // ENABLE_BME280

#ifdef ENABLE_CCS811
#include <Adafruit_CCS811.h>
#endif // ENABLE_CCS811

#include "config.h"
#include "DebugLog.h"
#include "memory.h"
#include "servers.h"
#include "html.h"
#include "sensors.h"

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#include <Wire.h>
#endif

#define SHT_I2C_ADDRESS HTDU21D_ADDRESS
#define BME_I2C_ADDRESS_1 0x76
#define BME_I2C_ADDRESS_2 0x77
#define CCS811_ADDRESS_1 0x5A
#define CCS811_ADDRESS_2 0x5B

#if defined(ARDUINO_ARCH_ESP8266)

#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 0

static TwoWire Wire2;
static SHT2x sht(SHT_I2C_ADDRESS, &Wire2);

#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_AVR)

static SHT2x sht(SHT_I2C_ADDRESS, &Wire);

#endif

#ifdef ENABLE_BME280
static Adafruit_BME280 bme1, bme2;
bool found_bme1 = false;
bool found_bme2 = false;
#endif // ENABLE_BME280

bool found_sht = false;

#ifdef ENABLE_CCS811
static Adafruit_CCS811 ccs1, ccs2;
bool found_ccs1 = false;
bool found_ccs2 = false;
bool ccs1_data_valid = false;
bool ccs2_data_valid = false;
int ccs1_error_code = 0;
int ccs2_error_code = 0;
#endif // ENABLE_CCS811

static unsigned long last_sensor_handle_time = 0;

#define DEF_SENSOR_READ_FUNC(n, v)        \
float n(void) {                           \
    while (1) {                           \
        float a = v;                      \
        float b = v;                      \
                                          \
        if ((a > b) && ((a - b) < 2.0)) { \
            return (a + b) / 2.0;         \
        }                                 \
                                          \
        if ((a < b) && ((b - a) < 2.0)) { \
            return (a + b) / 2.0;         \
        }                                 \
                                          \
        /* to keep wdt happy */           \
        delay(1);                         \
    }                                     \
    return 0.0;                           \
}

#ifdef ENABLE_BME280
DEF_SENSOR_READ_FUNC(bme1_temp, bme1.readTemperature())
DEF_SENSOR_READ_FUNC(bme2_temp, bme2.readTemperature())
DEF_SENSOR_READ_FUNC(bme1_humid, bme1.readHumidity())
DEF_SENSOR_READ_FUNC(bme2_humid, bme2.readHumidity())
DEF_SENSOR_READ_FUNC(bme1_pressure, bme1.readPressure())
DEF_SENSOR_READ_FUNC(bme2_pressure, bme2.readPressure())
#endif // ENABLE_BME280

DEF_SENSOR_READ_FUNC(sht_temp, sht.GetTemperature() + config.sht_temp_off)
DEF_SENSOR_READ_FUNC(sht_humid, sht.GetHumidity())

#ifdef ENABLE_CCS811

float ccs1_eco2(void) {
    return ccs1.geteCO2();
}

float ccs1_tvoc(void) {
    return ccs1.getTVOC();
}

float ccs2_eco2(void) {
    return ccs2.geteCO2();
}

float ccs2_tvoc(void) {
    return ccs2.getTVOC();
}

static void ccs_update() {
    if (found_ccs1) {
        if (ccs1.available()) {
            if (found_sht) {
                ccs1.setEnvironmentalData(sht_humid(), sht_temp());
#ifdef ENABLE_BME280
            } else if (found_bme1) {
                ccs1.setEnvironmentalData(bme1_humid(), bme1_temp());
            } else if (found_bme2) {
                ccs1.setEnvironmentalData(bme2_humid(), bme2_temp());
#endif // ENABLE_BME280
            }

            ccs1_error_code = ccs1.readData();
            ccs1_data_valid = (ccs1_error_code == 0);
        }
    }

    if (found_ccs2) {
        if (ccs2.available()) {
            if (found_sht) {
                ccs2.setEnvironmentalData(sht_humid(), sht_temp());
#ifdef ENABLE_BME280
            } else if (found_bme1) {
                ccs2.setEnvironmentalData(bme1_humid(), bme1_temp());
            } else if (found_bme2) {
                ccs2.setEnvironmentalData(bme2_humid(), bme2_temp());
#endif // ENABLE_BME280
            }

            ccs2_error_code = ccs2.readData();
            ccs2_data_valid = (ccs2_error_code == 0);
        }
    }
}

#endif // ENABLE_CCS811

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void handleCalibrate() {
    bool diff = false;

#ifdef ENABLE_BME280

    if (server.hasArg("bme1")) {
        diff = true;
        String off_string = server.arg("bme1");
        double real_temp = off_string.toDouble();
        double meas_temp = bme1_temp() - config.bme1_temp_off;
        config.bme1_temp_off = real_temp - meas_temp;
    }

    if (server.hasArg("bme2")) {
        diff = true;
        String off_string = server.arg("bme2");
        double real_temp = off_string.toDouble();
        double meas_temp = bme2_temp() - config.bme2_temp_off;
        config.bme2_temp_off = real_temp - meas_temp;
    }

#endif // ENABLE_BME280

    if (server.hasArg("sht")) {
        diff = true;
        String off_string = server.arg("sht");
        double real_temp = off_string.toDouble();
        double meas_temp = sht_temp() - config.sht_temp_off;
        config.sht_temp_off = real_temp - meas_temp;
    }

    if (diff) {
#ifdef ENABLE_BME280
        if (found_bme1) {
            bme1.setTemperatureCompensation(config.bme1_temp_off);
        }

        if (found_bme2) {
            bme2.setTemperatureCompensation(config.bme2_temp_off);
        }
#endif // ENABLE_BME280

        mem_write(config);
        handlePage(42);
    } else {
        handlePage();
    }
}
#endif

void initSensors() {
    // Init I2C and try to connect to sensors
#if defined(ARDUINO_ARCH_ESP8266)

#if defined(ENABLE_BME280) || defined(ENABLE_CCS811)
    debug.println(F("Wire2"));
    Wire2.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#endif

#ifdef ENABLE_BME280
    debug.println(F("BME"));
    found_bme1 = (!bme1.begin(BME_I2C_ADDRESS_1, &Wire2)) ? false : true;
    found_bme2 = (!bme2.begin(BME_I2C_ADDRESS_2, &Wire2)) ? false : true;
#endif // ENABLE_BME280

#ifdef ENABLE_CCS811
    debug.println(F("CCS"));
    found_ccs1 = ccs1.begin(CCS811_ADDRESS_1, &Wire2);
    found_ccs2 = ccs2.begin(CCS811_ADDRESS_2, &Wire2);
#endif // ENABLE_CCS811

#elif defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_AVR)

#if defined(ARDUINO_ARCH_ESP32) && (defined(ENABLE_BME280) || defined(ENABLE_CCS811))
    debug.println(F("Wire"));
    Wire.begin();
#endif

#ifdef ENABLE_BME280
    debug.println(F("BME"));
    found_bme1 = (!bme1.begin(BME_I2C_ADDRESS_1, &Wire)) ? false : true;
    found_bme2 = (!bme2.begin(BME_I2C_ADDRESS_2, &Wire)) ? false : true;
#endif // ENABLE_BME280

#ifdef ENABLE_CCS811
    debug.println(F("CCS"));
    found_ccs1 = ccs1.begin(CCS811_ADDRESS_1, &Wire);
    found_ccs2 = ccs2.begin(CCS811_ADDRESS_2, &Wire);
#endif // ENABLE_CCS811

#endif

    debug.println(F("SHT"));
    found_sht = sht.GetAlive();

#ifdef ENABLE_BME280
    // initialize temperature offsets
    if (found_bme1) {
        bme1.setTemperatureCompensation(config.bme1_temp_off);
    }
    if (found_bme2) {
        bme2.setTemperatureCompensation(config.bme2_temp_off);
    }
#endif // ENABLE_BME280
}

void runSensors() {
    unsigned long time = millis();

    if ((time - last_sensor_handle_time) >= SENSOR_HANDLE_INTERVAL) {
        last_sensor_handle_time = time;

#ifdef ENABLE_CCS811
        if (found_ccs1 || found_ccs2) {
            ccs_update();
        }
#endif // ENABLE_CCS811
    }
}
