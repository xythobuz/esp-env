#ifndef __ESP_ENV_CONFIG__
#define __ESP_ENV_CONFIG__

// Sketch version
const char* esp_env_version = "0.2.0";

// location of sensor, used in DB and hostname
//const char* sensor_location = "livingroom";
//const char* sensor_location = "bedroom";
//const char* sensor_location = "bathroom";
//const char* sensor_location = "kitchen";
//const char* sensor_location = "hallway";
//const char* sensor_location = "tent";
const char* sensor_location = "storage";

#define SENSOR_HOSTNAME_PREFIX "ESP-"

// WiFi AP settings
const char* ssid = "WIFI_SSID_HERE";
const char* password = "WIFI_PASSWORD_HERE";

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

// feature selection
#define ENABLE_DATABASE_WRITES
#define ENABLE_LED_HEARTBEAT_BLINK
#define DONT_RUN_WITHOUT_SENSORS
#define HTTP_SHOW_ESP_STATS

#endif // __ESP_ENV_CONFIG__

