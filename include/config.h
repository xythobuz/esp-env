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
#define ESP_ENV_VERSION "0.5.0"

// location of sensor, used in DB and hostname
//#define SENSOR_LOCATION_LIVINGROOM
//#define SENSOR_LOCATION_BEDROOM
//#define SENSOR_LOCATION_BATHROOM
//#define SENSOR_LOCATION_GREENHOUSE
#define SENSOR_LOCATION_TESTING

// WiFi AP settings
#define WIFI_SSID "WIFI_SSID_HERE"
#define WIFI_PASS "WIFI_PASSWORD_HERE"

// MQTT settings
#define MQTT_HOST "MQTT_IP_HERE"
#define MQTT_PORT 1883
#define MQTT_USER "USERNAME" // undef to disable auth
#define MQTT_PASS "PASSWORD" // undef to disable auth

// InfluxDB settings
#define INFLUXDB_HOST "INFLUX_IP_HERE"
#define INFLUXDB_PORT 8086
#define INFLUXDB_DATABASE "roomsensorsdiy"

// all given in milliseconds
#define SERVER_HANDLE_INTERVAL 10
#define DB_WRITE_INTERVAL (30 * 1000)
#define LED_BLINK_INTERVAL (2 * 1000)
#define LED_INIT_BLINK_INTERVAL 500
#define LED_CONNECT_BLINK_INTERVAL 250
#define LED_ERROR_BLINK_INTERVAL 100
#define MQTT_RECONNECT_INTERVAL (5 * 1000)

#if defined(SENSOR_LOCATION_LIVINGROOM)
#define SENSOR_LOCATION "livingroom"
#elif defined(SENSOR_LOCATION_BEDROOM)
#define SENSOR_LOCATION "bedroom"
#elif defined(SENSOR_LOCATION_BATHROOM)
#define SENSOR_LOCATION "bathroom"
#elif defined(SENSOR_LOCATION_GREENHOUSE)
#define SENSOR_LOCATION "greenhouse"
#elif defined(SENSOR_LOCATION_TESTING)
#define SENSOR_LOCATION "testing"
#else
#define SENSOR_LOCATION "unknown"
#endif

#if defined(RELAIS_SERIAL) || defined(RELAIS_GPIO)
#define FEATURE_RELAIS
#endif

#if defined(MOISTURE_ADC_ESP32) || defined(MOISTURE_ADC_ARDUINO)
#define FEATURE_MOISTURE
#endif

#endif // __ESP_ENV_CONFIG__
