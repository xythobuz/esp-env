#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <InfluxDb.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_BME280.h>
#include <SHT2x.h>

const char* sensor_location = "SENSOR_NAME_GOES_HERE";

const char* ssid = "YOUR_SSID_HERE";
const char* password = "YOUR_PASS_HERE";

#define INFLUXDB_HOST "INFLUX_DB_SERVER_IP_HERE"
#define INFLUXDB_PORT 8086
#define INFLUXDB_DATABASE "INFLUX_DB_DATABASE_NAME_HERE"

#define SENSOR_HOSTNAME_PREFIX "ESP-"

Influxdb influx(INFLUXDB_HOST, INFLUXDB_PORT);
ESP8266WebServer server(80);
TwoWire Wire2;
SHT2x sht(HTDU21D_ADDRESS, &Wire2);
Adafruit_BME280 bme;
bool found_bme = false;
bool found_sht = false;

void handleRoot() {
    String message = "ESP8266 BME280 Sensor";
    message += "\n\nLocation: ";
    message += sensor_location;
    message += "\nMAC: ";
    message += WiFi.macAddress();

    if (found_bme) {
        message += "\n\nBME280:";
        message += "\nTemperature: ";
        message += String(bme.readTemperature());
        message += "\nHumidity: ";
        message += String(bme.readHumidity());
        message += "\nPressure: ";
        message += String(bme.readPressure());
    }

    if (found_sht) {
        message += "\n\nSHT21:";
        message += "\nTemperature: ";
        message += String(sht.GetTemperature());
        message += "\nHumidity: ";
        message += String(sht.GetHumidity());
    }

    if ((!found_bme) && (!found_sht)) {
        message += "\n\nNo Sensor available!";
    }

    message += "\n";
    server.send(200, "text/plain", message);
}

void setup() {
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW); // LED on
    delay(500);
    digitalWrite(1, HIGH); // LED off
    delay(500);
    digitalWrite(1, LOW); // LED on
    delay(500);
    digitalWrite(1, HIGH); // LED off
    delay(500);

    Wire2.begin(2, 0);
    found_bme = (!bme.begin(0x76, &Wire2)) ? false : true;
    found_sht = sht.GetAlive();

    if ((!found_bme) && (!found_sht)) {
        // no sensor available
        while (1) {
            digitalWrite(1, !digitalRead(1));
            delay(100);
        }
    }

    String hostname = SENSOR_HOSTNAME_PREFIX;
    hostname += sensor_location;
    WiFi.hostname(hostname);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    MDNS.begin(hostname);

    while (WiFi.status() != WL_CONNECTED) {
        delay(250);
        digitalWrite(1, !digitalRead(1));
    }

    influx.setDb(INFLUXDB_DATABASE);

    server.on("/", handleRoot);
    server.begin();
}

#define SERVER_HANDLE_INTERVAL 10 // in ms
#define DB_WRITE_INTERVAL (30 * 1000) // in ms
#define LED_BLINK_INTERVAL (2 * 1000) // in ms

unsigned long last_server_handle_time = 0;
unsigned long last_db_write_time = 0;
unsigned long last_led_blink_time = 0;

void loop() {
    unsigned long time = millis();

    if ((time - last_server_handle_time) >= SERVER_HANDLE_INTERVAL) {
        last_server_handle_time = time;

        server.handleClient();
        MDNS.update();
    }

    if ((time - last_db_write_time) >= DB_WRITE_INTERVAL) {
        last_db_write_time = time;

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
                digitalWrite(1, !digitalRead(1));
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
                digitalWrite(1, !digitalRead(1));
            }
        }
    }

    if ((time - last_led_blink_time) >= LED_BLINK_INTERVAL) {
        last_led_blink_time = time;

        digitalWrite(1, !digitalRead(1));
    }
}

