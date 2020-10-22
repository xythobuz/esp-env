/*
 * main.cpp
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
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SHT2x.h>

#if defined(ARDUINO_ARCH_ESP8266)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define ESP_PLATFORM_NAME "ESP8266"

#elif defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#define ESP_PLATFORM_NAME "ESP32"

#endif

#include "config.h"
#include "moisture.h"
#include "relais.h"
#include "SimpleUpdater.h"

#define BUILTIN_LED_PIN 1

UPDATE_WEB_SERVER server(80);
SimpleUpdater updater;

#ifdef ENABLE_INFLUXDB_LOGGING
#include <InfluxDb.h>

Influxdb influx(INFLUXDB_HOST, INFLUXDB_PORT);

#define INFLUX_MAX_ERRORS_RESET 10
int error_count = 0;
#endif // ENABLE_INFLUXDB_LOGGING

#define SHT_I2C_ADDRESS HTDU21D_ADDRESS
#define BME_I2C_ADDRESS_1 0x76
#define BME_I2C_ADDRESS_2 0x77

#if defined(ARDUINO_ARCH_ESP8266)

#define I2C_SDA_PIN 2
#define I2C_SCL_PIN 0

TwoWire Wire2;
SHT2x sht(SHT_I2C_ADDRESS, &Wire2);

#elif defined(ARDUINO_ARCH_ESP32)

SHT2x sht(SHT_I2C_ADDRESS, &Wire);

#endif

Adafruit_BME280 bme1, bme2;

bool found_bme1 = false;
bool found_bme2 = false;
bool found_sht = false;

unsigned long last_server_handle_time = 0;
unsigned long last_db_write_time = 0;
unsigned long last_led_blink_time = 0;

static void relaisTest() {
    for (int i = 0; i < 10; i++) {
        relais_enable(i, 400 + (i * 1000));
        delay(100);
    }
}

void handleRelaisTest() {
    String message = F("<html><head>\n");
    message += F("<title>" ESP_PLATFORM_NAME " Environment Sensor</title>\n");
    message += F("</head><body>\n");
    message += F("<p>Relais Test started!</p>\n");
    message += F("<p><a href=\"/\">Return to Home</a></p>\n");
    message += F("</body></html>\n");
    
    server.send(200, "text/html", message);
    
    relaisTest();
}

void handleRoot() {
    String message = F("<html><head>\n");
    message += F("<title>" ESP_PLATFORM_NAME " Environment Sensor</title>\n");
    message += F("</head><body>\n");
    message += F("<h1>" ESP_PLATFORM_NAME " Environment Sensor</h1>\n");
    message += F("\n<p>\n");
    message += F("Version: ");
    message += esp_env_version;
    message += F("\n<br>\n");
    message += F("Location: ");
    message += sensor_location;
    message += F("\n<br>\n");
    message += F("MAC: ");
    message += WiFi.macAddress();
    message += F("\n</p>\n");

#if defined(ARDUINO_ARCH_ESP8266)
    
    message += F("\n<p>\n");
    message += F("Reset reason: ");
    message += ESP.getResetReason();
    message += F("\n<br>\n");
    message += F("Free heap: ");
    message += String(ESP.getFreeHeap());
    message += F(" (");
    message += String(ESP.getHeapFragmentation());
    message += F("% fragmentation)");
    message += F("\n<br>\n");
    message += F("Free sketch space: ");
    message += String(ESP.getFreeSketchSpace());
    message += F("\n<br>\n");
    message += F("Flash chip real size: ");
    message += String(ESP.getFlashChipRealSize());

    if (ESP.getFlashChipSize() != ESP.getFlashChipRealSize()) {
        message += F("\n<br>\n");
        message += F("WARNING: sdk chip size (");
        message += (ESP.getFlashChipSize());
        message += F(") does not match!");
    }
    
    message += F("\n</p>\n");
    
#elif defined(ARDUINO_ARCH_ESP32)

    message += F("\n<p>\n");
    message += F("Free heap: ");
    message += String(ESP.getFreeHeap() / 1024.0);
    message += F("k\n<br>\n");
    message += F("Free sketch space: ");
    message += String(ESP.getFreeSketchSpace() / 1024.0);
    message += F("k\n<br>\n");
    message += F("Flash chip size: ");
    message += String(ESP.getFlashChipSize() / 1024.0);
    message += F("k\n</p>\n");
    
#endif

    message += F("\n<p>\n");
    if (found_bme1) {
        message += F("BME280 Low:");
        message += F("\n<br>\n");
        message += F("Temperature: ");
        message += String(bme1.readTemperature());
        message += F("\n<br>\n");
        message += F("Humidity: ");
        message += String(bme1.readHumidity());
        message += F("\n<br>\n");
        message += F("Pressure: ");
        message += String(bme1.readPressure());
    } else {
        message += F("BME280 (low) not connected!");
    }
    message += F("\n</p>\n");

    message += F("\n<p>\n");
    if (found_bme2) {
        message += F("BME280 High:");
        message += F("\n<br>\n");
        message += F("Temperature: ");
        message += String(bme2.readTemperature());
        message += F("\n<br>\n");
        message += F("Humidity: ");
        message += String(bme2.readHumidity());
        message += F("\n<br>\n");
        message += F("Pressure: ");
        message += String(bme2.readPressure());
    } else {
        message += F("BME280 (high) not connected!");
    }
    message += F("\n</p>\n");

    message += F("\n<p>\n");
    if (found_sht) {
        message += F("SHT21:");
        message += F("\n<br>\n");
        message += F("Temperature: ");
        message += String(sht.GetTemperature());
        message += F("\n<br>\n");
        message += F("Humidity: ");
        message += String(sht.GetHumidity());
    } else {
        message += F("SHT21 not connected!");
    }
    message += F("\n</p>\n");

    for (int i = 0; i < moisture_count(); i++) {
        int moisture = moisture_read(i);
        if (moisture < moisture_max()) {
            message += F("\n<p>\n");
            message += F("Sensor ");
            message += String(i + 1);
            message += F(":\n<br>\n");
            message += F("Moisture: ");
            message += String(moisture);
            message += F(" / ");
            message += String(moisture_max());
            message += F("\n</p>\n");
        }
    }
    
    if (moisture_count() <= 0) {
        message += F("\n<p>\n");
        message += F("No moisture sensors configured!");
        message += F("\n</p>\n");
    }

    message += F("<p>\n");
    message += F("Try <a href=\"/update\">/update</a> for OTA firmware updates!\n");
    message += F("</p>\n");
    
    message += F("<p>\n");
#ifdef ENABLE_INFLUXDB_LOGGING
    message += F("InfluxDB: ");
    message += INFLUXDB_DATABASE;
    message += F(" @ ");
    message += INFLUXDB_HOST;
    message += F(":");
    message += String(INFLUXDB_PORT);
    message += F("\n");
#else
    message += F("InfluxDB logging not enabled!\n");
#endif
    message += F("</p>\n");
    
    message += F("<p><a href=\"/relaistest\">Relais Test</a></p>\n");
    message += F("</body></html>\n");

    server.send(200, "text/html", message);
}

void setup() {
    pinMode(BUILTIN_LED_PIN, OUTPUT);
    
    relais_init();

    // Blink LED for init
    for (int i = 0; i < 2; i++) {
        digitalWrite(BUILTIN_LED_PIN, LOW); // LED on
        delay(LED_INIT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, HIGH); // LED off
        delay(LED_INIT_BLINK_INTERVAL);
    }
    
    moisture_init();

    // Init I2C and try to connect to sensors
#if defined(ARDUINO_ARCH_ESP8266)

    Wire2.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    found_bme1 = (!bme1.begin(BME_I2C_ADDRESS_1, &Wire2)) ? false : true;
    found_bme2 = (!bme2.begin(BME_I2C_ADDRESS_2, &Wire2)) ? false : true;

#elif defined(ARDUINO_ARCH_ESP32)

    Wire.begin();
    found_bme1 = (!bme1.begin(BME_I2C_ADDRESS_1, &Wire)) ? false : true;
    found_bme2 = (!bme2.begin(BME_I2C_ADDRESS_2, &Wire)) ? false : true;

#endif

    found_sht = sht.GetAlive();

    // Build hostname string
    String hostname = SENSOR_HOSTNAME_PREFIX;
    hostname += sensor_location;

#if defined(ARDUINO_ARCH_ESP8266)

    // Connect to WiFi AP
    WiFi.hostname(hostname);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
    }
    
#elif defined(ARDUINO_ARCH_ESP32)

    // Set hostname workaround
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.setHostname(hostname.c_str());
    
    // Workaround for WiFi connecting only every 2nd reset
    // https://github.com/espressif/arduino-esp32/issues/2501#issuecomment-513602522
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (info.disconnected.reason == 202) {
            esp_sleep_enable_timer_wakeup(10);
            esp_deep_sleep_start();
            delay(100);
        }
    }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

    // Connect to WiFi AP
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
    }
    
    // Set hostname workaround
    WiFi.setHostname(hostname.c_str());

#endif

#ifdef ENABLE_INFLUXDB_LOGGING
    // Setup InfluxDB Client
    influx.setDb(INFLUXDB_DATABASE);
#endif // ENABLE_INFLUXDB_LOGGING

    // Setup HTTP Server
    MDNS.begin(hostname.c_str());
    updater.setup(&server);
    server.on("/", handleRoot);
    server.on("/relaistest", handleRelaisTest);
    server.begin();
    MDNS.addService("http", "tcp", 80);
}

void handleServers() {
    server.handleClient();
    
#if defined(ARDUINO_ARCH_ESP8266)
    MDNS.update();
#endif
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

#ifdef ENABLE_INFLUXDB_LOGGING
void writeDatabase() {
    if (found_bme1) {
        InfluxData measurement("environment");
        measurement.addTag("location", sensor_location);
        measurement.addTag("placement", "1");
        measurement.addTag("device", WiFi.macAddress());
        measurement.addTag("sensor", "bme280");

        measurement.addValue("temperature", bme1.readTemperature());
        measurement.addValue("pressure", bme1.readPressure());
        measurement.addValue("humidity", bme1.readHumidity());

        writeMeasurement(measurement);
    }
    if (found_bme2) {
        InfluxData measurement("environment");
        measurement.addTag("location", sensor_location);
        measurement.addTag("placement", "2");
        measurement.addTag("device", WiFi.macAddress());
        measurement.addTag("sensor", "bme280");

        measurement.addValue("temperature", bme2.readTemperature());
        measurement.addValue("pressure", bme2.readPressure());
        measurement.addValue("humidity", bme2.readHumidity());

        writeMeasurement(measurement);
    }

    if (found_sht) {
        InfluxData measurement("environment");
        measurement.addTag("location", sensor_location);
        measurement.addTag("device", WiFi.macAddress());
        measurement.addTag("sensor", "sht21");

        measurement.addValue("temperature", sht.GetTemperature());
        measurement.addValue("humidity", sht.GetHumidity());

        writeMeasurement(measurement);
    }
    
    for (int i = 0; i < moisture_count(); i++) {
        int moisture = moisture_read(i);
        if (moisture < moisture_max()) {
            InfluxData measurement("moisture");
            measurement.addTag("location", sensor_location);
            measurement.addTag("device", WiFi.macAddress());
            measurement.addTag("sensor", String(i + 1));

            measurement.addValue("value", moisture);
            measurement.addValue("maximum", moisture_max());

            writeMeasurement(measurement);
        }
    }
}
#endif // ENABLE_INFLUXDB_LOGGING

void loop() {
    unsigned long time = millis();
    
    relais_run();

    if ((time - last_server_handle_time) >= SERVER_HANDLE_INTERVAL) {
        last_server_handle_time = time;
        handleServers();
    }

#ifdef ENABLE_INFLUXDB_LOGGING
    if ((time - last_db_write_time) >= DB_WRITE_INTERVAL) {
        last_db_write_time = time;
        writeDatabase();
    }
    
#ifdef INFLUX_MAX_ERRORS_RESET
    if (error_count >= INFLUX_MAX_ERRORS_RESET) {
        ESP.restart();
    }
#endif // INFLUX_MAX_ERRORS_RESET
#endif // ENABLE_INFLUXDB_LOGGING

    if ((time - last_led_blink_time) >= LED_BLINK_INTERVAL) {
        last_led_blink_time = time;
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
    }
}

