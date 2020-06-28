#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Adafruit_BME280.h>
#include <SHT2x.h>
#include "config.h"

#ifdef ENABLE_DATABASE_WRITES
#include <InfluxDb.h>
Influxdb influx(INFLUXDB_HOST, INFLUXDB_PORT);
#endif // ENABLE_DATABASE_WRITES

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updater;

TwoWire Wire2;
SHT2x sht(HTDU21D_ADDRESS, &Wire2);
Adafruit_BME280 bme;

bool found_bme = false;
bool found_sht = false;

unsigned long last_server_handle_time = 0;
unsigned long last_db_write_time = 0;
unsigned long last_led_blink_time = 0;

void handleRoot() {
    String message = F("ESP8266 Environment Sensor");
    message += F("\n\n");
    message += F("Version: ");
    message += esp_env_version;
    message += F("\n");
    message += F("Location: ");
    message += sensor_location;
    message += F("\n");
    message += F("MAC: ");
    message += WiFi.macAddress();

#ifdef HTTP_SHOW_ESP_STATS
    message += F("\n");
    message += F("Reset reason: ");
    message += ESP.getResetReason();
    message += F("\n");
    message += F("Free heap: ");
    message += String(ESP.getFreeHeap());
    message += F(" (");
    message += String(ESP.getHeapFragmentation());
    message += F("% fragmentation)");
    message += F("\n");
    message += F("Free sketch space: ");
    message += String(ESP.getFreeSketchSpace());
    message += F("\n");
    message += F("Flash chip real size: ");
    message += String(ESP.getFlashChipRealSize());

    if (ESP.getFlashChipSize() != ESP.getFlashChipRealSize()) {
        message += F("\n");
        message += F("WARNING: sdk chip size (");
        message += (ESP.getFlashChipSize());
        message += F(") does not match!");
    }
#endif // HTTP_SHOW_ESP_STATS

    if (found_bme) {
        message += F("\n\n");
        message += F("BME280:");
        message += F("\n");
        message += F("Temperature: ");
        message += String(bme.readTemperature());
        message += F("\n");
        message += F("Humidity: ");
        message += String(bme.readHumidity());
        message += F("\n");
        message += F("Pressure: ");
        message += String(bme.readPressure());
    }

    if (found_sht) {
        message += F("\n\n");
        message += F("SHT21:");
        message += F("\n");
        message += F("Temperature: ");
        message += String(sht.GetTemperature());
        message += F("\n");
        message += F("Humidity: ");
        message += String(sht.GetHumidity());
    }

    if ((!found_bme) && (!found_sht)) {
        message += F("\n\n");
        message += F("No Sensor available!");
    }

    message += F("\n\n");
    message += F("Try /update for OTA firmware updates!");
    message += F("\n");

    server.send(200, "text/plain", message);
}

void setup() {
    pinMode(1, OUTPUT);

    // Blink LED for init
    for (int i = 0; i < 2; i++) {
        digitalWrite(1, LOW); // LED on
        delay(LED_INIT_BLINK_INTERVAL);
        digitalWrite(1, HIGH); // LED off
        delay(LED_INIT_BLINK_INTERVAL);
    }

    // Init I2C and try to connect to sensors
    Wire2.begin(2, 0);
    found_bme = (!bme.begin(0x76, &Wire2)) ? false : true;
    found_sht = sht.GetAlive();

#ifdef DONT_RUN_WITHOUT_SENSORS
    if ((!found_bme) && (!found_sht)) {
        // no sensor available
        while (1) {
            digitalWrite(1, !digitalRead(1));
            delay(LED_ERROR_BLINK_INTERVAL);
        }
    }
#endif // DONT_RUN_WITHOUT_SENSORS

    // Set hostname
    String hostname = SENSOR_HOSTNAME_PREFIX;
    hostname += sensor_location;
    WiFi.hostname(hostname);

    // Connect to WiFi AP
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(LED_CONNECT_BLINK_INTERVAL);
        digitalWrite(1, !digitalRead(1));
    }

#ifdef ENABLE_DATABASE_WRITES
    // Setup InfluxDB Client
    influx.setDb(INFLUXDB_DATABASE);
#endif // ENABLE_DATABASE_WRITES

    // Setup HTTP Server
    MDNS.begin(hostname);
    updater.setup(&server);
    server.on("/", handleRoot);
    server.begin();
    MDNS.addService("http", "tcp", 80);
}

void handleServers() {
    server.handleClient();
    MDNS.update();
}

#ifdef ENABLE_DATABASE_WRITES
void writeDatabase() {
    if (found_bme) {
        InfluxData measurement("environment");
        measurement.addTag("location", sensor_location);
        measurement.addTag("device", WiFi.macAddress());
        measurement.addTag("sensor", "bme280");

        measurement.addValue("temperature", bme.readTemperature());
        measurement.addValue("pressure", bme.readPressure());
        measurement.addValue("humidity", bme.readHumidity());

        boolean success = influx.write(measurement);
        if (!success) {
            for (int i = 0; i < 10; i++) {
                digitalWrite(1, LOW); // LED on
                delay(LED_ERROR_BLINK_INTERVAL);
                digitalWrite(1, HIGH); // LED off
                delay(LED_ERROR_BLINK_INTERVAL);
            }
        }
    }

    if (found_sht) {
        InfluxData measurement("environment");
        measurement.addTag("location", sensor_location);
        measurement.addTag("device", WiFi.macAddress());
        measurement.addTag("sensor", "sht21");

        measurement.addValue("temperature", sht.GetTemperature());
        measurement.addValue("humidity", sht.GetHumidity());

        boolean success = influx.write(measurement);
        if (!success) {
            for (int i = 0; i < 10; i++) {
                digitalWrite(1, LOW); // LED on
                delay(LED_ERROR_BLINK_INTERVAL);
                digitalWrite(1, HIGH); // LED off
                delay(LED_ERROR_BLINK_INTERVAL);
            }
        }
    }
}
#endif // ENABLE_DATABASE_WRITES

void loop() {
    unsigned long time = millis();

    if ((time - last_server_handle_time) >= SERVER_HANDLE_INTERVAL) {
        last_server_handle_time = time;
        handleServers();
    }

#ifdef ENABLE_DATABASE_WRITES
    if ((time - last_db_write_time) >= DB_WRITE_INTERVAL) {
        last_db_write_time = time;
        writeDatabase();
    }
#endif // ENABLE_DATABASE_WRITES

#ifdef ENABLE_LED_HEARTBEAT_BLINK
    if ((time - last_led_blink_time) >= LED_BLINK_INTERVAL) {
        last_led_blink_time = time;
        digitalWrite(1, !digitalRead(1));
    }
#endif // ENABLE_LED_HEARTBEAT_BLINK
}

