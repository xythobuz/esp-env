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
#define ESP_ENV_VERSION "0.7.3"

// location of sensor, used in DB and hostname
// should be defined on pio command line
//#define SENSOR_LOCATION_LIVINGROOM
//#define SENSOR_LOCATION_LIVINGROOM_WORKSPACE
//#define SENSOR_LOCATION_LIVINGROOM_TV
//#define SENSOR_LOCATION_BEDROOM
//#define SENSOR_LOCATION_BATHROOM
//#define SENSOR_LOCATION_GREENHOUSE
//#define SENSOR_LOCATION_TESTING

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
//#define INFLUX_MAX_ERRORS_RESET 10

// LoRa SML Bridge "Crypto"
// needs to be sizeof(struct lora_sml_msg) bytes long
#define LORA_XOR_KEY "_SUPER_SECRET_KEY_HERE_"
#define LORA_CLIENT_CHECKSUM

// all given in milliseconds
#define SERVER_HANDLE_INTERVAL 10
#define SENSOR_HANDLE_INTERVAL (5 * 1000)
#define DB_WRITE_INTERVAL (30 * 1000)
#define MQTT_WRITE_INTERVAL (30 * 1000)
#define LED_BLINK_INTERVAL (2 * 1000)
#define LED_INIT_BLINK_INTERVAL 500
#define LED_CONNECT_BLINK_INTERVAL 250
#define LED_ERROR_BLINK_INTERVAL 100
#define MQTT_RECONNECT_INTERVAL (5 * 1000)

#define NTP_SERVER "pool.ntp.org"

//  https://remotemonitoringsystems.ca/time-zone-abbreviations.php
#define NTP_TZ_LOCATION "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Berlin, Germany, Europe

#if defined(SENSOR_LOCATION_LIVINGROOM)
#define SENSOR_LOCATION "livingroom"
#define SENSOR_ID SENSOR_LOCATION
#elif defined(SENSOR_LOCATION_LIVINGROOM_WORKSPACE)
#define SENSOR_LOCATION "livingroom"
#define SENSOR_ID "livingroom-ws"
#elif defined(SENSOR_LOCATION_LIVINGROOM_TV)
#define SENSOR_LOCATION "livingroom"
#define SENSOR_ID "livingroom-tv"
#elif defined(SENSOR_LOCATION_BEDROOM)
#define SENSOR_LOCATION "bedroom"
#define SENSOR_ID SENSOR_LOCATION
#elif defined(SENSOR_LOCATION_BATHROOM)
#define SENSOR_LOCATION "bathroom"
#define SENSOR_ID SENSOR_LOCATION
#elif defined(SENSOR_LOCATION_GREENHOUSE)
#define SENSOR_LOCATION "greenhouse"
#define SENSOR_ID SENSOR_LOCATION
#elif defined(SENSOR_LOCATION_TESTING)
#define SENSOR_LOCATION "testing"
#define SENSOR_ID SENSOR_LOCATION
#else
#error "Please define a SENSOR_LOCATION_x"
#endif

#if defined(RELAIS_SERIAL) || defined(RELAIS_GPIO)
#define FEATURE_RELAIS
#endif

#if defined(MOISTURE_ADC_ESP32) || defined(MOISTURE_ADC_ARDUINO)
#define FEATURE_MOISTURE
#endif

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
#define BUILTIN_LED_PIN 1
#elif defined(ARDUINO_ARCH_AVR)
#define BUILTIN_LED_PIN 13
#endif

#if defined(ARDUINO_ARCH_ESP8266)
#define ESP_PLATFORM_NAME "ESP8266"
#elif defined(ARDUINO_ARCH_ESP32)
#define ESP_PLATFORM_NAME "ESP32"
#elif defined(ARDUINO_ARCH_AVR)
#define ESP_PLATFORM_NAME "Uno WiFi"
#endif

#endif // __ESP_ENV_CONFIG__
