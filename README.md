# ESP-Env

Small firmware supporting different environmental sensors, as well as different microcontroller platforms.

Supports SHT21, BME280, CCS811, analog moisture sensors, serial or gpio relais, [SML](https://wiki.volkszaehler.org/software/sml) smart meter readouts.

Runs on ESP32, ESP8266, [Arduino Uno Wifi Developer Edition](https://github.com/JAndrassy/UnoWiFiDevEdSerial1), [Heltec Lora32 V3](https://heltec.org/project/wifi-lora-32-v3/), [Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display).

[![ESP-Env with BME280](https://www.xythobuz.de/img/espenv_10_small.jpg)](https://www.xythobuz.de/img/espenv_10.jpg)
[![SML LoRa transmitter](https://www.xythobuz.de/img/lora_sml_3_small.jpg)](https://www.xythobuz.de/img/lora_sml_3.jpg)

This is all very much specific to my IoT devices at home.
But it should be easy enough to customize and use as a starting point for your own experiments.

See [this blog post](https://www.xythobuz.de/espenv.html) for my initial temperature measurement experiments.
And [this blog post](https://www.xythobuz.de/lora_sml.html) for my LoRa SML smart meter bridge.
Some more stuff about my setup can be found [here](https://www.xythobuz.de/smarthome.html).

Mostly this is my simple playground with an Arduino platform compatible build system set up, to quickly try out things.

OTA updating on ESP32 requires a manual reset afterwards. (?)

Also remember to mod the LDR of CYDs.
Remove R19 and replace R15 with 100k.
