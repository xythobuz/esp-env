/*
 * config.h
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

#ifndef __ESP_ENV_CONFIG__
#define __ESP_ENV_CONFIG__

// Sketch version
const char* esp_env_version = "0.4.0";

// location of sensor, used in DB and hostname
//const char* sensor_location = "livingroom";
//const char* sensor_location = "bedroom";
//const char* sensor_location = "bathroom";
//const char* sensor_location = "kitchen";
//const char* sensor_location = "hallway";
//const char* sensor_location = "tent";
//const char* sensor_location = "storage";
//const char* sensor_location = "greenhouse";
const char* sensor_location = "testing";

#define SENSOR_HOSTNAME_PREFIX "ESP-"

// WiFi AP settings
const char* ssid = "WIFI_SSID_HERE";
const char* password = "WIFI_PASSWORD_HERE";

// InfluxDB settings
#define INFLUXDB_HOST "INFLUX_IP_HERE"
#define INFLUXDB_PORT 8086
#define INFLUXDB_DATABASE "roomsensorsdiy"

// feature selection
#define ENABLE_INFLUXDB_LOGGING

// all given in milliseconds
#define SERVER_HANDLE_INTERVAL 10
#define DB_WRITE_INTERVAL (30 * 1000)
#define LED_BLINK_INTERVAL (2 * 1000)
#define LED_INIT_BLINK_INTERVAL 500
#define LED_CONNECT_BLINK_INTERVAL 250
#define LED_ERROR_BLINK_INTERVAL 100

#endif // __ESP_ENV_CONFIG__

