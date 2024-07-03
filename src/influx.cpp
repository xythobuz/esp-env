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
#include "ui.h"
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

#ifdef USE_INFLUXDB_LIB
#include <InfluxDb.h>
#else
#include "SimpleInflux.h"
#endif

static Influxdb influx(INFLUXDB_HOST, INFLUXDB_PORT);
static int error_count = 0;
static unsigned long last_db_write_time = 0;

void initInflux() {
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
        debug.println(F("Resetting due to too many Influx errors"));
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

static void addTagsGeneric(InfluxData &measurement) {
    measurement.addTag("location", SENSOR_LOCATION);
    measurement.addTag("location-id", SENSOR_ID);

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    measurement.addTag("device", WiFi.macAddress().c_str());
#endif
}

static void addTagsSensor(InfluxData &measurement, String sensor, String placement) {
    addTagsGeneric(measurement);
    measurement.addTag("sensor", sensor);
    measurement.addTag("placement", placement);
}

#ifdef FEATURE_RELAIS
static void addTagsRelais(InfluxData &measurement, String id, String name) {
    addTagsGeneric(measurement);
    measurement.addTag("id", id);
    measurement.addTag("name", name);
}
#endif

void writeSensorDatum(String measurement, String sensor, String placement, String key, double value) {
    InfluxData ms(measurement);
    addTagsSensor(ms, sensor, placement);

    ms.addValue(key, value);

    debug.print("Writing ");
    debug.print(measurement);
    debug.print(" ");
    debug.print(sensor);
    debug.print(" ");
    debug.print(placement);
    debug.print(" ");
    debug.print(key);
    debug.print(" ");
    debug.printf("%.2lf\n", value);
    writeMeasurement(ms);

#ifdef FEATURE_NTP
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        debug.print(time_to_time_str(timeinfo) + " ");
    }
#endif // FEATURE_NTP

    debug.println(F("Done!"));
}

void writeDatabase() {
#ifdef ENABLE_BME280

    if (found_bme1) {
        InfluxData measurement("environment");
        addTagsSensor(measurement, F("bme280"), F("1"));

        measurement.addValue("temperature", bme1_temp());
        measurement.addValue("pressure", bme1_pressure());
        measurement.addValue("humidity", bme1_humid());

        debug.println(F("Writing bme1"));
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }

    if (found_bme2) {
        InfluxData measurement("environment");
        addTagsSensor(measurement, F("bme280"), F("2"));

        measurement.addValue("temperature", bme2_temp());
        measurement.addValue("pressure", bme2_pressure());
        measurement.addValue("humidity", bme2_humid());

        debug.println(F("Writing bme2"));
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }

#endif // ENABLE_BME280

    if (found_sht) {
        InfluxData measurement("environment");
        addTagsSensor(measurement, F("sht21"), F("1"));

        measurement.addValue("temperature", sht_temp());
        measurement.addValue("humidity", sht_humid());

        debug.println(F("Writing sht"));
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }

#ifdef ENABLE_CCS811

    if (found_ccs1) {
        InfluxData measurement("environment");
        addTagsSensor(measurement, F("ccs811"), F("1"));

        String err(ccs1_error_code);
        measurement.addTag("error", err);

        measurement.addValue("eco2", ccs1_eco2());
        measurement.addValue("tvoc", ccs1_tvoc());

        debug.println(F("Writing ccs1"));
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }

    if (found_ccs2) {
        InfluxData measurement("environment");
        addTagsSensor(measurement, F("ccs811"), F("2"));

        String err(ccs2_error_code);
        measurement.addTag("error", err);

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
            String sensor(i + 1, DEC);
            InfluxData measurement("moisture");
            addTagsSensor(measurement, sensor, sensor);

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
        addTagsRelais(measurement, String(i), relais_name(i));

        measurement.addValue("state", relais_get(i));

        debug.print(F("Writing relais "));
        debug.println(i);
        writeMeasurement(measurement);
        debug.println(F("Done!"));
    }
#endif // FEATURE_RELAIS
}

#else

void initInflux() { }
void runInflux() { }
void writeDatabase() { }

#endif // ENABLE_INFLUXDB_LOGGING
