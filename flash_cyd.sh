#!/usr/bin/env bash

MQTT_HOST="MQTT_IP_HERE"
MQTT_USER="USERNAME"
MQTT_PASS="PASSWORD"

echo "Resetting ESPs"
mosquitto_pub -h $MQTT_HOST -u $MQTT_USER -P $MQTT_PASS -t "esp_env/cmd" -m "reset"

echo "Waiting for them to come back online..."
sleep 5

echo "Flashing Livingroom"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_LIVINGROOM" pio run -e cyd -t upload --upload-port=cyd-livingroom

echo "Flashing Livingroom WS"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_LIVINGROOM_WORKSPACE" pio run -e cyd -t upload --upload-port=cyd-livingroom-ws

echo "Flashing Bathroom"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_BATHROOM" pio run -e cyd -t upload --upload-port=cyd-bathroom

echo "Flashing Bedroom"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_BEDROOM" pio run -e cyd -t upload --upload-port=cyd-bedroom

echo "Restore CYD compile_commands.json"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_LIVINGROOM" pio run -e cyd -t compiledb
