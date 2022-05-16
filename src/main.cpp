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
#include <Adafruit_BME280.h>
#include <SHT2x.h>

#ifdef ENABLE_CCS811
#include <Adafruit_CCS811.h>
#endif // ENABLE_CCS811

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

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)

#include <Wire.h>
#include "SimpleUpdater.h"

#define BUILTIN_LED_PIN 1

UPDATE_WEB_SERVER server(80);
SimpleUpdater updater;

#ifdef ENABLE_MQTT
#include <PubSubClient.h>
WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);
unsigned long last_mqtt_reconnect_time = 0;
#endif // ENABLE_MQTT

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
#define CCS811_ADDRESS_1 0x5A
#define CCS811_ADDRESS_2 0x5B

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

Adafruit_BME280 bme1, bme2;

bool found_bme1 = false;
bool found_bme2 = false;
bool found_sht = false;

#ifdef ENABLE_CCS811
Adafruit_CCS811 ccs1, ccs2;
bool found_ccs1 = false;
bool found_ccs2 = false;
bool ccs1_data_valid = false;
bool ccs2_data_valid = false;
int ccs1_error_code = 0;
int ccs2_error_code = 0;
#endif // ENABLE_CCS811

unsigned long last_server_handle_time = 0;
unsigned long last_db_write_time = 0;
unsigned long last_led_blink_time = 0;

void writeDatabase();

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

#ifdef ENABLE_CCS811

static float ccs1_eco2(void) {
    return ccs1.geteCO2();
}

static float ccs1_tvoc(void) {
    return ccs1.getTVOC();
}

static float ccs2_eco2(void) {
    return ccs2.geteCO2();
}

static float ccs2_tvoc(void) {
    return ccs2.getTVOC();
}

#endif // ENABLE_CCS811

#if defined(ARDUINO_ARCH_AVR)
#define ARDUINO_SEND_PARTIAL_PAGE() do { \
        size_t len = message.length(), off = 0; \
        while (off < len) { \
            if ((len - off) >= 50) { \
                client.write(message.c_str() + off, 50); \
                off += 50; \
            } else { \
                client.write(message.c_str() + off, len - off); \
                off = len; \
            } \
        } \
        message = ""; \
    } while (false);
#else
#define ARDUINO_SEND_PARTIAL_PAGE() while (false) { }
#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void handlePage(int mode = -1, int id = 0) {
#else
void handlePage(WiFiClient &client, int mode = -1, int id = 0) {
#endif
    String message;

    message += F("<html><head>");
    message += F("<title>" ESP_PLATFORM_NAME " Environment Sensor</title>");
    message += F("</head><body>");
    message += F("<h1>" ESP_PLATFORM_NAME " Environment Sensor</h1>");
    message += F("\n<p>\n");
    message += F("Version: ");
    message += ESP_ENV_VERSION;
    message += F("\n<br>\n");
    message += F("Location: ");
    message += SENSOR_LOCATION;

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    message += F("\n<br>\n");
    message += F("MAC: ");
    message += WiFi.macAddress();
#endif

    message += F("\n</p>\n");

    ARDUINO_SEND_PARTIAL_PAGE();

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

    message += F("\n<p>\n");
    if (found_bme1) {
        message += F("BME280 Low:");
        message += F("\n<br>\n");
        message += F("Temperature: ");
        message += String(bme1_temp());
        message += F("\n<br>\n");
        message += F("Humidity: ");
        message += String(bme1_humid());
        message += F("\n<br>\n");
        message += F("Pressure: ");
        message += String(bme1_pressure());
    } else {
        message += F("BME280 (low) not connected!");
    }
    message += F("\n</p>\n");

    message += F("\n<p>\n");
    if (found_bme2) {
        message += F("BME280 High:");
        message += F("\n<br>\n");
        message += F("Temperature: ");
        message += String(bme2_temp());
        message += F("\n<br>\n");
        message += F("Humidity: ");
        message += String(bme2_humid());
        message += F("\n<br>\n");
        message += F("Pressure: ");
        message += String(bme2_pressure());
    } else {
        message += F("BME280 (high) not connected!");
    }
    message += F("\n</p>\n");

    ARDUINO_SEND_PARTIAL_PAGE();

    message += F("\n<p>\n");
    if (found_sht) {
        message += F("SHT21:");
        message += F("\n<br>\n");
        message += F("Temperature: ");
        message += String(sht_temp());
        message += F("\n<br>\n");
        message += F("Humidity: ");
        message += String(sht_humid());
    } else {
        message += F("SHT21 not connected!");
    }
    message += F("\n</p>\n");

#ifdef ENABLE_CCS811

    message += F("\n<p>\n");
    if (found_ccs1) {
        message += F("CCS811 Low:");
        message += F("\n<br>\n");
        message += F("eCO2: ");
        message += String(ccs1_eco2());
        message += F("ppm");
        message += F("\n<br>\n");
        message += F("TVOC: ");
        message += String(ccs1_tvoc());
        message += F("ppb");

        if (!ccs1_data_valid) {
            message += F("\n<br>\n");
            message += F("Data invalid (");
            message += String(ccs1_error_code);
            message += F(")!");
        }
    } else {
        message += F("CCS811 (Low) not connected!");
    }
    message += F("\n</p>\n");

    message += F("\n<p>\n");
    if (found_ccs2) {
        message += F("CCS811 High:");
        message += F("\n<br>\n");
        message += F("eCO2: ");
        message += String(ccs2_eco2());
        message += F("ppm");
        message += F("\n<br>\n");
        message += F("TVOC: ");
        message += String(ccs2_tvoc());
        message += F("ppb");

        if (!ccs2_data_valid) {
            message += F("\n<br>\n");
            message += F("Data invalid (");
            message += String(ccs2_error_code);
            message += F(")!");
        }
    } else {
        message += F("CCS811 (High) not connected!");
    }
    message += F("\n</p>\n");

#endif // ENABLE_CCS811

    ARDUINO_SEND_PARTIAL_PAGE();

#ifdef FEATURE_MOISTURE
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

    ARDUINO_SEND_PARTIAL_PAGE();
#endif // FEATURE_MOISTURE

#ifdef FEATURE_RELAIS
    message += F("\n<p>\n");
    for (int i = 0; i < relais_count(); i++) {
        message += String(F("<a href=\"/on?id=")) + String(i) + String(F("\">Relais ")) + String(i) + String(F(" On (")) + relais_name(i) + String(F(")</a><br>\n"));
        message += String(F("<a href=\"/off?id=")) + String(i) + String(F("\">Relais ")) + String(i) + String(F(" Off (")) + relais_name(i) + String(F(")</a><br>\n"));
    }
    message += String(F("<a href=\"/on?id=")) + String(relais_count()) + String(F("\">All Relais On</a><br>\n"));
    message += String(F("<a href=\"/off?id=")) + String(relais_count()) + String(F("\">All Relais Off</a><br>\n"));
    message += F("</p>\n");

    if (mode >= 0) {
        message += F("<p>");
        message += F("Turned Relais ");
        message += (id < relais_count()) ? String(id) : String(F("1-4"));
        message += (mode ? String(F(" On")) : String(F(" Off")));
        message += F("</p>\n");
    }

    message += F("\n<p>\n");
    for (int i = 0; i < relais_count(); i++) {
        message += String(F("Relais ")) + String(i) + String(F(" (")) + relais_name(i) + String(F(") = ")) + (relais_get(i) ? String(F("On")) : String(F("Off"))) + String(F("<br>\n"));
    }
    message += F("</p>\n");

    ARDUINO_SEND_PARTIAL_PAGE();
#endif // FEATURE_RELAIS

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

    message += F("</body></html>");

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    server.send(200, "text/html", message);
#else
    ARDUINO_SEND_PARTIAL_PAGE();
#endif
}

#ifdef FEATURE_RELAIS

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void handleOn() {
#else
void handleOn(WiFiClient &client) {
#endif
    String id_string = server.arg("id");
    int id = id_string.toInt();

    if ((id >= 0) && (id < relais_count())) {
        relais_set(id, 1);
    } else {
        for (int i = 0; i < relais_count(); i++) {
            relais_set(i, 1);
        }
    }

#ifdef ENABLE_INFLUXDB_LOGGING
    writeDatabase();
#endif // ENABLE_INFLUXDB_LOGGING

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    handlePage(1, id);
#else
    handlePage(client, 1, id);
#endif
}

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void handleOff() {
#else
void handleOff(WiFiClient &client) {
#endif
    String id_string = server.arg("id");
    int id = id_string.toInt();

    if ((id >= 0) && (id < relais_count())) {
        relais_set(id, 0);
    } else {
        for (int i = 0; i < relais_count(); i++) {
            relais_set(i, 0);
        }
    }

#ifdef ENABLE_INFLUXDB_LOGGING
    writeDatabase();
#endif // ENABLE_INFLUXDB_LOGGING

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    handlePage(0, id);
#else
    handlePage(client, 0, id);
#endif
}

#endif // FEATURE_RELAIS

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
void handleRoot() {
    handlePage();
#else
void handleRoot(WiFiClient &client) {
    handlePage(client);
#endif
}

#ifdef ENABLE_MQTT
void writeMQTT() {
    if (!mqtt.connected()) {
        return;
    }

    if (found_bme1) {
        mqtt.publish(SENSOR_LOCATION "/temperature", String(bme1_temp()).c_str());
        mqtt.publish(SENSOR_LOCATION "/humidity", String(bme1_humid()).c_str());
        mqtt.publish(SENSOR_LOCATION "/pressure", String(bme1_pressure()).c_str());
    } else if (found_bme2) {
        mqtt.publish(SENSOR_LOCATION "/temperature", String(bme2_temp()).c_str());
        mqtt.publish(SENSOR_LOCATION "/humidity", String(bme2_humid()).c_str());
        mqtt.publish(SENSOR_LOCATION "/pressure", String(bme2_pressure()).c_str());
    } else if (found_sht) {
        mqtt.publish(SENSOR_LOCATION "/temperature", String(sht_temp()).c_str());
        mqtt.publish(SENSOR_LOCATION "/humidity", String(sht_humid()).c_str());
    }

#ifdef ENABLE_CCS811
    if (found_ccs1) {
        mqtt.publish(SENSOR_LOCATION "/eco2", String(ccs1_eco2()).c_str());
        mqtt.publish(SENSOR_LOCATION "/tvoc", String(ccs1_tvoc()).c_str());
    } else if (found_ccs2) {
        mqtt.publish(SENSOR_LOCATION "/eco2", String(ccs2_eco2()).c_str());
        mqtt.publish(SENSOR_LOCATION "/tvoc", String(ccs2_tvoc()).c_str());
    }
#endif // ENABLE_CCS811
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
#ifdef FEATURE_RELAIS
    int state = 0;
    int id = -1;

    String ts(topic), ps((char *)payload);

    String our_topic(SENSOR_LOCATION);
    our_topic += "/";

    if (!ts.startsWith(our_topic)) {
        Serial.print(F("Unknown MQTT room "));
        Serial.println(ts);
        return;
    }

    String ids = ts.substring(our_topic.length());
    for (int i = 0; i < relais_count(); i++) {
        if (ids == relais_name(i)) {
            id = i;
            break;
        }
    }

    if (id < 0) {
        Serial.print(F("Unknown MQTT topic "));
        Serial.println(ts);
        return;
    }

    if (ps.indexOf("on") != -1) {
        state = 1;
    } else if (ps.indexOf("off") != -1) {
        state = 0;
    } else {
        return;
    }

    if ((id >= 0) && (id < relais_count())) {
        relais_set(id, state);

#ifdef ENABLE_INFLUXDB_LOGGING
        writeDatabase();
#endif // ENABLE_INFLUXDB_LOGGING
    }
#endif // FEATURE_RELAIS
}

void mqttReconnect() {
    // Create a random client ID
    String clientId = F("ESP-" SENSOR_LOCATION "-");
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
#if defined(MQTT_USER) && defined(MQTT_PASS)
    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
#else
    if (mqtt.connect(clientId.c_str())) {
#endif
        // Once connected, publish an announcement...
        mqtt.publish(SENSOR_LOCATION, "sensor online");

        // ... and resubscribe
#ifdef FEATURE_RELAIS
        mqtt.subscribe(SENSOR_LOCATION);
        for (int i = 0; i < relais_count(); i++) {
            String topic(SENSOR_LOCATION);
            topic += String("/") + relais_name(i);
            mqtt.subscribe(topic.c_str());
        }
#endif // FEATURE_RELAIS
    }
}
#endif // ENABLE_MQTT

void setup() {
    pinMode(BUILTIN_LED_PIN, OUTPUT);
    
    Serial.begin(115200);

    // Blink LED for init
    for (int i = 0; i < 2; i++) {
        digitalWrite(BUILTIN_LED_PIN, LOW); // LED on
        delay(LED_INIT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, HIGH); // LED off
        delay(LED_INIT_BLINK_INTERVAL);
    }

    Serial.print(F("Relais"));
    relais_init();
    
    Serial.print(F("Moisture"));
    moisture_init();

    // Init I2C and try to connect to sensors
#if defined(ARDUINO_ARCH_ESP8266)

    Serial.print(F("Wire2"));
    Wire2.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    Serial.print(F("BME"));
    found_bme1 = (!bme1.begin(BME_I2C_ADDRESS_1, &Wire2)) ? false : true;
    found_bme2 = (!bme2.begin(BME_I2C_ADDRESS_2, &Wire2)) ? false : true;

#ifdef ENABLE_CCS811
    Serial.print(F("CCS"));
    found_ccs1 = ccs1.begin(CCS811_ADDRESS_1, &Wire2);
    found_ccs2 = ccs2.begin(CCS811_ADDRESS_2, &Wire2);
#endif // ENABLE_CCS811

#elif defined(ARDUINO_ARCH_ESP32)

    Serial.print(F("Wire"));
    Wire.begin();

    Serial.print(F("BME"));
    found_bme1 = (!bme1.begin(BME_I2C_ADDRESS_1, &Wire)) ? false : true;
    found_bme2 = (!bme2.begin(BME_I2C_ADDRESS_2, &Wire)) ? false : true;

#ifdef ENABLE_CCS811
    Serial.print(F("CCS"));
    found_ccs1 = ccs1.begin(CCS811_ADDRESS_1, &Wire);
    found_ccs2 = ccs2.begin(CCS811_ADDRESS_2, &Wire);
#endif // ENABLE_CCS811

#elif defined(ARDUINO_ARCH_AVR)

    Serial.print(F("BME"));
    found_bme1 = (!bme1.begin(BME_I2C_ADDRESS_1, &Wire)) ? false : true;
    found_bme2 = (!bme2.begin(BME_I2C_ADDRESS_2, &Wire)) ? false : true;

#ifdef ENABLE_CCS811
    Serial.print(F("CCS"));
    found_ccs1 = ccs1.begin(CCS811_ADDRESS_1, &Wire);
    found_ccs2 = ccs2.begin(CCS811_ADDRESS_2, &Wire);
#endif // ENABLE_CCS811

#endif

    Serial.print(F("SHT"));
    found_sht = sht.GetAlive();

    // Build hostname string
    String hostname = SENSOR_HOSTNAME_PREFIX;
    hostname += SENSOR_LOCATION;

#if defined(ARDUINO_ARCH_ESP8266)

    // Connect to WiFi AP
    Serial.print(F("Connecting WiFi"));
    WiFi.hostname(hostname);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        Serial.print(F("."));
    }
    Serial.println(F("\nWiFi connected!"));
    
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
    Serial.print(F("Connecting WiFi"));
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        Serial.print(F("."));
    }
    Serial.println(F("\nWiFi connected!"));
    
    // Set hostname workaround
    WiFi.setHostname(hostname.c_str());

#elif defined(ARDUINO_ARCH_AVR)

    Serial1.begin(115200);

    WiFi.init(&Serial1);

    Serial.print(F("Connecting WiFi"));
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
        Serial.print(F("."));
    }
    Serial.println(F("\nWiFi connected!"));

#endif

    Serial.println(F("Seeding"));
    randomSeed(micros());

#ifdef ENABLE_MQTT
    Serial.println(F("MQTT"));
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
#endif // ENABLE_MQTT

#ifdef ENABLE_INFLUXDB_LOGGING
    Serial.println(F("Influx"));
    influx.setDb(INFLUXDB_DATABASE);
#endif // ENABLE_INFLUXDB_LOGGING

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    // Setup HTTP Server
    Serial.println(F("HTTP"));
    MDNS.begin(hostname.c_str());
    updater.setup(&server);
    server.on("/", handleRoot);

#ifdef FEATURE_RELAIS
    server.on("/on", handleOn);
    server.on("/off", handleOff);
#endif // FEATURE_RELAIS

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

                    // TODO parse path and handle different pages
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

        measurement.addTag("location", SENSOR_LOCATION);
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

        measurement.addTag("location", SENSOR_LOCATION);
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

        measurement.addTag("location", SENSOR_LOCATION);
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

#ifdef ENABLE_CCS811

    if (found_ccs1) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", SENSOR_LOCATION);
        measurement.addTag("placement", "1");
        measurement.addTag("sensor", "ccs811");

        String err(ccs1_error_code);
        measurement.addTag("error", err);

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("eco2", ccs1_eco2());
        measurement.addValue("tvoc", ccs1_tvoc());

        Serial.println(F("Writing ccs1"));
        writeMeasurement(measurement);
        Serial.println(F("Done!"));
    }

    if (found_ccs2) {
#if defined(ARDUINO_ARCH_AVR)
        measurement.clear();
        measurement.setName("environment");
#else
        InfluxData measurement("environment");
#endif

        measurement.addTag("location", SENSOR_LOCATION);
        measurement.addTag("placement", "2");
        measurement.addTag("sensor", "ccs811");

        String err(ccs2_error_code);
        measurement.addTag("error", err);

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("eco2", ccs2_eco2());
        measurement.addValue("tvoc", ccs2_tvoc());

        Serial.println(F("Writing ccs2"));
        writeMeasurement(measurement);
        Serial.println(F("Done!"));
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
#endif // FEATURE_MOISTURE

#ifdef FEATURE_RELAIS
    for (int i = 0; i < relais_count(); i++) {
        InfluxData measurement("relais");
        measurement.addTag("location", SENSOR_LOCATION);
        measurement.addTag("id", String(i));
        measurement.addTag("name", relais_name(i));

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        measurement.addTag("device", WiFi.macAddress());
#endif

        measurement.addValue("state", relais_get(i));
        writeMeasurement(measurement);
    }
#endif // FEATURE_RELAIS

    Serial.println(F("All Done!"));
}
#endif // ENABLE_INFLUXDB_LOGGING

#ifdef ENABLE_CCS811
void ccs_update() {
    if (found_ccs1) {
        if (ccs1.available()) {
            ccs1_error_code = ccs1.readData();
            ccs1_data_valid = (ccs1_error_code == 0);

            if (found_bme1) {
                ccs1.setEnvironmentalData(bme1_humid(), bme1_temp());
            } else if (found_bme2) {
                ccs1.setEnvironmentalData(bme2_humid(), bme2_temp());
            } else if (found_sht) {
                ccs1.setEnvironmentalData(sht_humid(), sht_temp());
            }
        }
    }

    if (found_ccs2) {
        if (ccs2.available()) {
            ccs2_error_code = ccs2.readData();
            ccs2_data_valid = (ccs2_error_code == 0);

            if (found_bme1) {
                ccs2.setEnvironmentalData(bme1_humid(), bme1_temp());
            } else if (found_bme2) {
                ccs2.setEnvironmentalData(bme2_humid(), bme2_temp());
            } else if (found_sht) {
                ccs2.setEnvironmentalData(sht_humid(), sht_temp());
            }
        }
    }
}
#endif // ENABLE_CCS811

void loop() {
    unsigned long time = millis();
    
#ifdef ENABLE_CCS811
    if (found_ccs1 || found_ccs2) {
        ccs_update();
    }
#endif // ENABLE_CCS811

    if ((time - last_server_handle_time) >= SERVER_HANDLE_INTERVAL) {
        last_server_handle_time = time;
        handleServers();
    }

    if ((time - last_db_write_time) >= DB_WRITE_INTERVAL) {
        last_db_write_time = time;

#ifdef ENABLE_INFLUXDB_LOGGING
        writeDatabase();
#endif // ENABLE_INFLUXDB_LOGGING

#ifdef ENABLE_MQTT
        writeMQTT();
#endif // ENABLE_MQTT
    }
    
#ifdef ENABLE_INFLUXDB_LOGGING
#ifdef INFLUX_MAX_ERRORS_RESET
    if (error_count >= INFLUX_MAX_ERRORS_RESET) {
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
        ESP.restart();
#endif
    }
#endif // INFLUX_MAX_ERRORS_RESET
#endif // ENABLE_INFLUXDB_LOGGING

#ifdef ENABLE_MQTT
    if (!mqtt.connected() && ((millis() - last_mqtt_reconnect_time) >= MQTT_RECONNECT_INTERVAL)) {
        last_mqtt_reconnect_time = millis();
        mqttReconnect();
    }

    mqtt.loop();
#endif // ENABLE_MQTT

    // blink heartbeat LED
    if ((time - last_led_blink_time) >= LED_BLINK_INTERVAL) {
        last_led_blink_time = time;
        digitalWrite(BUILTIN_LED_PIN, !digitalRead(BUILTIN_LED_PIN));
    }
    
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    // reset ESP every 3d to be safe
    if (time >= (3UL * 24UL * 60UL * 60UL * 1000UL)) {
        ESP.restart();
    }
#endif
}
