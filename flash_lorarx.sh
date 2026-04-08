#!/usr/bin/env bash

MQTT_HOST="MQTT_IP_HERE"
MQTT_USER="USERNAME"
MQTT_PASS="PASSWORD"

echo "Resetting ESPs"
mosquitto_pub -h $MQTT_HOST -u $MQTT_USER -P $MQTT_PASS -t "esp_env/cmd" -m "reset"

echo "Waiting for them to come back online..."
sleep 5

echo "Flashing Lora-Rx"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_TESTING" pio run -e lorarx -t upload --upload-port=lora-testing

echo "Restore Lora-Rx compile_commands.json"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_TESTING" pio run -e lorarx -t compiledb
