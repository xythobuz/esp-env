/*
 * relais.cpp
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
#include "relais.h"

#if defined(ARDUINO_ARCH_ESP8266)

int relais_count(void) {
    return 0;
}

int relais_enable(int relais, unsigned long time) {
    return 0;
}

void relais_init(void) { }
void relais_run(void) { }

#elif defined(ARDUINO_ARCH_ESP32)

#define RELAIS_COUNT 10

struct relais_state {
    unsigned long turn_off;
};

static struct relais_state state[RELAIS_COUNT];
static int gpios[RELAIS_COUNT] = {
    0, 15, 2, 4,
    16, 17, 5, 18,
    19, 23
};

static void relais_set(int relais, int state) {
    digitalWrite(gpios[relais], state ? LOW : HIGH);
}

int relais_count(void) {
    return RELAIS_COUNT;
}

int relais_enable(int relais, unsigned long time) {
    if ((relais < 0) || (relais >= RELAIS_COUNT)) {
        return -1;
    }
    
    relais_set(relais, 1);
    state[relais].turn_off = millis() + time;
    return 0;
}

void relais_init(void) {
    for (int i = 0; i < RELAIS_COUNT; i++) {
        pinMode(gpios[i], OUTPUT);
        relais_set(i, 0);
        
        state[i].turn_off = 0;
    }
}

void relais_run(void) {
    for (int i = 0; i < RELAIS_COUNT; i++) {
        if (state[i].turn_off > 0) {
            if (millis() >= state[i].turn_off) {
                relais_set(i, 0);
            }
        }
    }
}

#endif

