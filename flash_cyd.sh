#!/usr/bin/env bash

echo "Flashing Livingroom"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_LIVINGROOM" pio run -e cyd -t upload --upload-port=cyd-livingroom

echo "Flashing Livingroom WS"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_LIVINGROOM_WORKSPACE" pio run -e cyd -t upload --upload-port=cyd-livingroom-ws

echo "Flashing Bathroom"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_BATHROOM" pio run -e cyd -t upload --upload-port=cyd-bathroom

echo "Flashing Bedroom"
PLATFORMIO_BUILD_FLAGS="-DSENSOR_LOCATION_BEDROOM" pio run -e cyd -t upload --upload-port=cyd-bedroom
