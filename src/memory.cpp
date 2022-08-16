/*
 * memory.cpp
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

#include <Arduino.h>
#include <EEPROM.h>

#include "DebugLog.h"
#include "memory.h"

ConfigMemory mem_read() {
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    EEPROM.begin(sizeof(ConfigMemory) + 1);
#endif

    ConfigMemory cm;
    uint8_t cs = 0x42;
    uint8_t *us = (uint8_t *)((void *)&cm);
    for (unsigned int i = 0; i < sizeof(ConfigMemory); i++) {
        uint8_t v = EEPROM.read(i);
        cs ^= v;
        us[i] = v;
    }

    uint8_t checksum = EEPROM.read(sizeof(ConfigMemory));

    if (cs == checksum) {
        return cm;
    } else {
        debug.print(F("Config checksum mismatch: "));
        debug.print(cs);
        debug.print(F(" != "));
        debug.println(checksum);

        ConfigMemory empty;
        return empty;
    }
}

void mem_write(ConfigMemory mem) {
    uint8_t cs = 0x42;
    const uint8_t *us = (const uint8_t *)((const void *)&mem);
    for (unsigned int i = 0; i < sizeof(ConfigMemory); i++) {
        uint8_t v = us[i];
        EEPROM.write(i, v);
        cs ^= v;
    }

    EEPROM.write(sizeof(ConfigMemory), cs);

#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_ESP32)
    EEPROM.commit();
#endif

    debug.print(F("Written config with checksum "));
    debug.println(cs);
}







/*
0
0
0
0
0
0
0
0
0
0
0
0
30
5
22
192
0
0
0
0
0
0
0
0
205





0
0
0
0
0
0
0
0
0
0
0
0
184
158
19
192
0
0
0
0
0
0
0
0
245
*/
