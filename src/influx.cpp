/*
 * influx.cpp
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
#include "DebugLog.h"
#include "sensors.h"
#include "relais.h"
#include "moisture.h"
#include "influx.h"

#ifdef ENABLE_INFLUXDB_LOGGING

#if defined(ARDUINO_ARCH_ESP8266)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#elif defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#elif defined(ARDUINO_ARCH_AVR)

#include <UnoWiFiDevEdSerial1.h>
#include <WiFiLink.h>

#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#include <InfluxDb.h>
#else
#include "SimpleInflux.h"
#endif

static Influxdb influx(INFLUXDB_HOST, INFLUXDB_PORT);
static int error_count = 0;
static unsigned long last_db_write_time = 0;

void initInflux() {
    debug.println(F("Influx"));
    influx.setDb(INFLUXDB_DATABASE);
}

void runInflux() {
    unsigned long time = millis();

    if ((time - last_db_write_time) >= DB_WRITE_INTERVAL) {
        last_db_write_time = time;
        writeDatabase();
    }

#ifdef INFLUX_MAX_ERRORS_RESET
    if (error_count >= INFLUX_MAX_ERRORS_RESET) {
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        ESP.restart();
#else
        // TODO implement watchdog reset for AVR
#endif
    }
#endif // INFLUX_MAX_ERRORS_RESET
}

static boolean writeMeasurement(InfluxData &measurement) {
    boolean success = influx.write(measurement);
    if (!success) {
        error_count++;
        for (int i = 0; i < 10; i++) {
            digitalWrite(BUILTIN_LED_PIN, LOW); // LED on
            delay(LED_ERROR_BLINK_INTERVAL);
            digitalWrite(BUILTIN_LED_PIN, HIGH); // LED off
            delay(LED_ERROR_BLINK_INTERVAL);
        }
    }
    return success;
}

void writeDatabase() {
#if defined(ARDUINO_ARCH_AVR)
    debug.println(F("Writing to InfluxDB"));

    InfluxData measurement("");
#endif

    if (found_bme1) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", SENSOR_LOCATION);
        measurement.addTag("location-id", SENSOR_ID);
        measurement.addTag("placement", "1");
        measurement.addTag("sensor", "bme280");

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("temperature", bme1_temp());
        measurement.addValue("pressure", bme1_pressure());
        measurement.addValue("humidity", bme1_humid());

        debug.println(F("Writing bme1"));
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }

    if (found_bme2) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", SENSOR_LOCATION);
        measurement.addTag("location-id", SENSOR_ID);
        measurement.addTag("placement", "2");
        measurement.addTag("sensor", "bme280");

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("temperature", bme2_temp());
        measurement.addValue("pressure", bme2_pressure());
        measurement.addValue("humidity", bme2_humid());

        debug.println(F("Writing bme2"));
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }

    if (found_sht) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", SENSOR_LOCATION);
        measurement.addTag("location-id", SENSOR_ID);
        measurement.addTag("sensor", "sht21");

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("temperature", sht_temp());
        measurement.addValue("humidity", sht_humid());

        debug.println(F("Writing sht"));
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }

#ifdef ENABLE_CCS811

    if (found_ccs1) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", SENSOR_LOCATION);
        measurement.addTag("location-id", SENSOR_ID);
        measurement.addTag("placement", "1");
        measurement.addTag("sensor", "ccs811");

        String err(ccs1_error_code);
        measurement.addTag("error", err);

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("eco2", ccs1_eco2());
        measurement.addValue("tvoc", ccs1_tvoc());

        debug.println(F("Writing ccs1"));
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }

    if (found_ccs2) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", SENSOR_LOCATION);
        measurement.addTag("location-id", SENSOR_ID);
        measurement.addTag("placement", "2");
        measurement.addTag("sensor", "ccs811");

        String err(ccs2_error_code);
        measurement.addTag("error", err);

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("eco2", ccs2_eco2());
        measurement.addValue("tvoc", ccs2_tvoc());

        debug.println(F("Writing ccs2"));
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }

#endif // ENABLE_CCS811

#ifdef FEATURE_MOISTURE
    for (int i = 0; i < moisture_count(); i++) {
        int moisture = moisture_read(i);
        if (moisture < moisture_max()) {
#if defined(ARDUINO_ARCH_AVR)
            measurement.clear();
            measurement.setName("moisture");
#else
            InfluxData measurement("moisture");
#endif

            measurement.addTag("location", SENSOR_LOCATION);
            measurement.addTag("location-id", SENSOR_ID);
            String sensor(i + 1, DEC);
            measurement.addTag("sensor", sensor);

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
            measurement.addTag("device", WiFi.macAddress());
#endif

            measurement.addValue("value", moisture);
            measurement.addValue("maximum", moisture_max());

            debug.print(F("Writing moisture "));
            debug.println(i);
            writeMeasurement(measurement);
            debug.println(F("Done!"));
        }
    }
#endif // FEATURE_MOISTURE

#ifdef FEATURE_RELAIS
    for (int i = 0; i < relais_count(); i++) {
        InfluxData measurement("relais");
        measurement.addTag("location", SENSOR_LOCATION);
        measurement.addTag("location-id", SENSOR_ID);
        measurement.addTag("id", String(i));
        measurement.addTag("name", relais_name(i));

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("state", relais_get(i));
        writeMeasurement(measurement);
    }
#endif // FEATURE_RELAIS
}

#else

void initInflux() { }
void runInflux() { }
void writeDatabase() { }

#endif // ENABLE_INFLUXDB_LOGGING
