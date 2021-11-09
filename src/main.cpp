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

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)

#include "SimpleUpdater.h"

#define BUILTIN_LED_PIN 1

UPDATE_WEB_SERVER server(80);
SimpleUpdater updater;

#elif defined(ARDUINO_ARCH_AVR)

#define ESP_PLATFORM_NAME "Uno WiFi"
#define BUILTIN_LED_PIN 13

#endif

#ifdef ENABLE_INFLUXDB_LOGGING
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#include <InfluxDb.h>
#else
#include "SimpleInflux.h"
#endif

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

#elif defined(ARDUINO_ARCH_AVR)

#include <UnoWiFiDevEdSerial1.h>
#include <WiFiLink.h>

WiFiServer server(80);
SHT2x sht(SHT_I2C_ADDRESS, &Wire);

#endif

//#define ENABLE_RELAIS_TEST

Adafruit_BME280 bme1, bme2;

bool found_bme1 = false;
bool found_bme2 = false;
bool found_sht = false;

unsigned long last_server_handle_time = 0;
unsigned long last_db_write_time = 0;
unsigned long last_led_blink_time = 0;

#ifdef ENABLE_RELAIS_TEST

#include "relais.h"

static void relaisTest() {
    for (int i = 0; i < 10; i++) {
        relais_enable(i, 400 + (i * 1000));
        delay(100);
    }
}

void handleRelaisTest() {
    String message = F("<html><head>");
    message += F("<title>" ESP_PLATFORM_NAME " Environment Sensor</title>");
    message += F("</head><body>");
    message += F("<p>Relais Test started!</p>");
    message += F("<p><a href=\"/\">Return to Home</a></p>");
    message += F("</body></html>");
    
    server.send(200, "text/html", message);
    
    relaisTest();
}

#endif // ENABLE_RELAIS_TEST

static float bme1_temp(void) {
    while (1) {
        float a = bme1.readTemperature();
        float b = bme1.readTemperature();
        
        if ((a > b) && ((a - b) < 2.0)) {
            return (a + b) / 2.0;
        }
        
        if ((a < b) && ((b - a) < 2.0)) {
            return (a + b) / 2.0;
        }
    }
    return 0.0;
}

static float bme2_temp(void) {
    while (1) {
        float a = bme2.readTemperature();
        float b = bme2.readTemperature();
        
        if ((a > b) && ((a - b) < 2.0)) {
            return (a + b) / 2.0;
        }
        
        if ((a < b) && ((b - a) < 2.0)) {
            return (a + b) / 2.0;
        }
    }
    return 0.0;
}

static float bme1_humid(void) {
    while (1) {
        float a = bme1.readHumidity();
        float b = bme1.readHumidity();
        
        if ((a > b) && ((a - b) < 2.0)) {
            return (a + b) / 2.0;
        }
        
        if ((a < b) && ((b - a) < 2.0)) {
            return (a + b) / 2.0;
        }
    }
    return 0.0;
}

static float bme2_humid(void) {
    while (1) {
        float a = bme2.readHumidity();
        float b = bme2.readHumidity();
        
        if ((a > b) && ((a - b) < 2.0)) {
            return (a + b) / 2.0;
        }
        
        if ((a < b) && ((b - a) < 2.0)) {
            return (a + b) / 2.0;
        }
    }
    return 0.0;
}

static float bme1_pressure(void) {
    while (1) {
        float a = bme1.readPressure();
        float b = bme1.readPressure();
        
        if ((a > b) && ((a - b) < 2.0)) {
            return (a + b) / 2.0;
        }
        
        if ((a < b) && ((b - a) < 2.0)) {
            return (a + b) / 2.0;
        }
    }
    return 0.0;
}

static float bme2_pressure(void) {
    while (1) {
        float a = bme2.readPressure();
        float b = bme2.readPressure();
        
        if ((a > b) && ((a - b) < 2.0)) {
            return (a + b) / 2.0;
        }
        
        if ((a < b) && ((b - a) < 2.0)) {
            return (a + b) / 2.0;
        }
    }
    return 0.0;
}

static float sht_temp(void) {
    while (1) {
        float a = sht.GetTemperature();
        float b = sht.GetTemperature();
        
        if ((a > b) && ((a - b) < 2.0)) {
            return (a + b) / 2.0;
        }
        
        if ((a < b) && ((b - a) < 2.0)) {
            return (a + b) / 2.0;
        }
    }
    return 0.0;
}

static float sht_humid(void) {
    while (1) {
        float a = sht.GetHumidity();
        float b = sht.GetHumidity();
        
        if ((a > b) && ((a - b) < 2.0)) {
            return (a + b) / 2.0;
        }
        
        if ((a < b) && ((b - a) < 2.0)) {
            return (a + b) / 2.0;
        }
    }
    return 0.0;
}

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void handleRoot() {
#else
void handleRoot(WiFiClient &client) {
#endif
    String message;

    message += F("<html><head>");
    message += F("<title>" ESP_PLATFORM_NAME " Environment Sensor</title>");
    message += F("</head><body>");
    message += F("<h1>" ESP_PLATFORM_NAME " Environment Sensor</h1>");
    message += F("<p>");
    message += F("Version: ");
    message += esp_env_version;
    message += F("<br>");
    message += F("Location: ");
    message += sensor_location;

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    message += F("<br>");
    message += F("MAC: ");
    message += WiFi.macAddress();
#endif

    message += F("</p>");

#if defined(ARDUINO_ARCH_AVR)
    do {
        size_t len = message.length(), off = 0;
        while (off < len) {
            if ((len - off) >= 50) {
                client.write(message.c_str() + off, 50);
                off += 50;
            } else {
                client.write(message.c_str() + off, len - off);
                off = len;
            }
        }
        message = "";
    } while (false);
#endif

#if defined(ARDUINO_ARCH_ESP8266)
    
    message += F("<p>");
    message += F("Reset reason: ");
    message += ESP.getResetReason();
    message += F("<br>");
    message += F("Free heap: ");
    message += String(ESP.getFreeHeap());
    message += F(" (");
    message += String(ESP.getHeapFragmentation());
    message += F("% fragmentation)");
    message += F("<br>");
    message += F("Free sketch space: ");
    message += String(ESP.getFreeSketchSpace());
    message += F("<br>");
    message += F("Flash chip real size: ");
    message += String(ESP.getFlashChipRealSize());

    if (ESP.getFlashChipSize() != ESP.getFlashChipRealSize()) {
        message += F("<br>");
        message += F("WARNING: sdk chip size (");
        message += (ESP.getFlashChipSize());
        message += F(") does not match!");
    }
    
    message += F("</p>");
    
#elif defined(ARDUINO_ARCH_ESP32)

    message += F("<p>");
    message += F("Free heap: ");
    message += String(ESP.getFreeHeap() / 1024.0);
    message += F("k<br>");
    message += F("Free sketch space: ");
    message += String(ESP.getFreeSketchSpace() / 1024.0);
    message += F("k<br>");
    message += F("Flash chip size: ");
    message += String(ESP.getFlashChipSize() / 1024.0);
    message += F("k</p>");
    
#endif

    message += F("<p>");
    if (found_bme1) {
        message += F("BME280 Low:");
        message += F("<br>");
        message += F("Temperature: ");
        message += String(bme1_temp());
        message += F("<br>");
        message += F("Humidity: ");
        message += String(bme1_humid());
        message += F("<br>");
        message += F("Pressure: ");
        message += String(bme1_pressure());
    } else {
        message += F("BME280 (low) not connected!");
    }
    message += F("</p>");

    message += F("<p>");
    if (found_bme2) {
        message += F("BME280 High:");
        message += F("<br>");
        message += F("Temperature: ");
        message += String(bme2_temp());
        message += F("<br>");
        message += F("Humidity: ");
        message += String(bme2_humid());
        message += F("<br>");
        message += F("Pressure: ");
        message += String(bme2_pressure());
    } else {
        message += F("BME280 (high) not connected!");
    }
    message += F("</p>");

    message += F("<p>");
    if (found_sht) {
        message += F("SHT21:");
        message += F("<br>");
        message += F("Temperature: ");
        message += String(sht_temp());
        message += F("<br>");
        message += F("Humidity: ");
        message += String(sht_humid());
    } else {
        //message += F("SHT21 not connected!");
    }
    message += F("</p>");

#if defined(ARDUINO_ARCH_AVR)
    do {
        size_t len = message.length(), off = 0;
        while (off < len) {
            if ((len - off) >= 50) {
                client.write(message.c_str() + off, 50);
                off += 50;
            } else {
                client.write(message.c_str() + off, len - off);
                off = len;
            }
        }
        message = "";
    } while (false);
#endif

    for (int i = 0; i < moisture_count(); i++) {
        int moisture = moisture_read(i);
        if (moisture < moisture_max()) {
            message += F("<p>");
            message += F("Sensor ");
            message += String(i + 1);
            message += F(":<br>");
            message += F("Moisture: ");
            message += String(moisture);
            message += F(" / ");
            message += String(moisture_max());
            message += F("</p>");
        }
    }
    
    if (moisture_count() <= 0) {
        message += F("<p>");
        message += F("No moisture sensors configured!");
        message += F("</p>");
    }

#if ! defined(ARDUINO_ARCH_AVR)
    message += F("<p>");
    message += F("Try <a href=\"/update\">/update</a> for OTA firmware updates!");
    message += F("</p>");
#endif
    
    message += F("<p>");
#ifdef ENABLE_INFLUXDB_LOGGING
    message += F("InfluxDB: ");
    message += INFLUXDB_DATABASE;
    message += F(" @ ");
    message += INFLUXDB_HOST;
    message += F(":");
    message += String(INFLUXDB_PORT);
#else
    message += F("InfluxDB logging not enabled!");
#endif
    message += F("</p>");
    
#ifdef ENABLE_RELAIS_TEST
    message += F("<p><a href=\"/relaistest\">Relais Test</a></p>");
#endif // ENABLE_RELAIS_TEST

    message += F("</body></html>");

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    server.send(200, "text/html", message);
#else
    do {
        size_t len = message.length(), off = 0;
        while (off < len) {
            if ((len - off) >= 50) {
                client.write(message.c_str() + off, 50);
                off += 50;
            } else {
                client.write(message.c_str() + off, len - off);
                off = len;
            }
        }
    } while (false);
#endif
}

void setup() {
    pinMode(BUILTIN_LED_PIN, OUTPUT);
    
#ifdef ENABLE_RELAIS_TEST
    relais_init();
#endif // ENABLE_RELAIS_TEST

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

#elif defined(ARDUINO_ARCH_AVR)

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

#elif defined(ARDUINO_ARCH_AVR)

    Serial.begin(115200);
    Serial1.begin(115200);

    WiFi.init(&Serial1);

    Serial.print(F("Connecting WiFi"));
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        Serial.print(F("."));
    }
    Serial.println(F("\nWiFi connected!"));

#endif

#ifdef ENABLE_INFLUXDB_LOGGING
    // Setup InfluxDB Client
    influx.setDb(INFLUXDB_DATABASE);
#endif // ENABLE_INFLUXDB_LOGGING

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    // Setup HTTP Server
    MDNS.begin(hostname.c_str());
    updater.setup(&server);
    server.on("/", handleRoot);

#ifdef ENABLE_RELAIS_TEST
    server.on("/relaistest", handleRelaisTest);
#endif

    MDNS.addService("http", "tcp", 80);
#endif

    server.begin();
}

#if defined(ARDUINO_ARCH_AVR)
void http_server() {
    // listen for incoming clients
    WiFiClient client = server.available();
    if (client) {
        Serial.println(F("new http client"));

        // an http request ends with a blank line
        boolean currentLineIsBlank = true;

        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                Serial.write(c);

                // if you've gotten to the end of the line (received a newline
                // character) and the line is blank, the http request has ended,
                // so you can send a reply
                if ((c == '\n') && currentLineIsBlank) {
                    // send a standard http response header
                    client.println(F("HTTP/1.1 200 OK"));
                    client.println(F("Content-Type: text/html"));
                    client.println(F("Connection: close"));
                    client.println();
                    handleRoot(client);
                    break;
                }

                if (c == '\n') {
                    // you're starting a new line
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    // you've gotten a character on the current line
                    currentLineIsBlank = false;
                }
            }
        }

        // give the web browser time to receive the data
        delay(10);

        // close the connection
        client.stop();
        Serial.println(F("http client disconnected"));
    }
}
#endif

void handleServers() {
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    server.handleClient();
#else
    http_server();
#endif
    
#if defined(ARDUINO_ARCH_ESP8266)
    MDNS.update();
#endif
}

#ifdef ENABLE_INFLUXDB_LOGGING
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
    Serial.println(F("Writing to InfluxDB"));

    InfluxData measurement("");
#endif

    if (found_bme1) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", sensor_location);
        measurement.addTag("placement", "1");
        measurement.addTag("sensor", "bme280");

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("temperature", bme1_temp());
        measurement.addValue("pressure", bme1_pressure());
        measurement.addValue("humidity", bme1_humid());

        Serial.println(F("Writing bme1"));
        writeMeasurement(measurement);
        Serial.println(F("Done!"));
    }

    if (found_bme2) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", sensor_location);
        measurement.addTag("placement", "2");
        measurement.addTag("sensor", "bme280");

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("temperature", bme2_temp());
        measurement.addValue("pressure", bme2_pressure());
        measurement.addValue("humidity", bme2_humid());

        Serial.println(F("Writing bme2"));
        writeMeasurement(measurement);
        Serial.println(F("Done!"));
    }

    if (found_sht) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", sensor_location);
        measurement.addTag("sensor", "sht21");

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("temperature", sht_temp());
        measurement.addValue("humidity", sht_humid());

        Serial.println(F("Writing sht"));
        writeMeasurement(measurement);
        Serial.println(F("Done!"));
    }
    
    for (int i = 0; i < moisture_count(); i++) {
        int moisture = moisture_read(i);
        if (moisture < moisture_max()) {
#if defined(ARDUINO_ARCH_AVR)
            measurement.clear();
            measurement.setName("moisture");
#else
            InfluxData measurement("moisture");
#endif

            measurement.addTag("location", sensor_location);
            String sensor(i + 1, DEC);
            measurement.addTag("sensor", sensor);

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
            measurement.addTag("device", WiFi.macAddress());
#endif

            measurement.addValue("value", moisture);
            measurement.addValue("maximum", moisture_max());

            Serial.print(F("Writing moisture "));
            Serial.println(i);
            writeMeasurement(measurement);
            Serial.println(F("Done!"));
        }
    }

    Serial.println(F("All Done!"));
}
#endif // ENABLE_INFLUXDB_LOGGING

void loop() {
    unsigned long time = millis();
    
#ifdef ENABLE_RELAIS_TEST
    relais_run();
#endif // ENABLE_RELAIS_TEST

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
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        ESP.restart();
#endif
    }
#endif // INFLUX_MAX_ERRORS_RESET
#endif // ENABLE_INFLUXDB_LOGGING

    // blink heartbeat LED
    if ((time - last_led_blink_time) >= LED_BLINK_INTERVAL) {
        last_led_blink_time = time;
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
    }
    
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    // reset ESP every 6h to be safe
    if (time >= (6 * 60 * 60 * 1000)) {
        ESP.restart();
    }
#endif
}

