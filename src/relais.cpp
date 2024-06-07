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

#include "config.h"
#include "relais.h"

#if defined(RELAIS_SERIAL)

#define SERIAL_RELAIS_COUNT 4

/*
Turn OFF the first relay  : A0 01 00 A1
Turn ON the first relay   : A0 01 01 A2
Turn OFF the second relay : A0 02 00 A2
Turn ON the second relay  : A0 02 01 A3
Turn OFF the third relay  : A0 03 00 A3
Turn ON the third relay   : A0 03 01 A4
Turn OFF the fourth relay : A0 04 00 A4
Turn ON the fourth relay  : A0 04 01 A5
*/

static int states[SERIAL_RELAIS_COUNT];

static String names[SERIAL_RELAIS_COUNT] = {
#if defined(SENSOR_LOCATION_BATHROOM)
    String("light_small"),
    String("light_big"),
    String("relais_2"),
    String("fan")
#elif defined(SENSOR_LOCATION_LIVINGROOM_WORKSPACE)
    String("light_pc"),
    String("light_bench"),
    String("relais_w2"),
    String("relais_w3")
#elif defined(SENSOR_LOCATION_LIVINGROOM_TV)
    String("light_amp"),
    String("light_box"),
    String("light_kitchen"),
    String("relais_t3")
#else
    String("relais_0"),
    String("relais_1"),
    String("relais_2"),
    String("relais_3")
#endif
};

static int initial_values[SERIAL_RELAIS_COUNT] = {
#if defined(SENSOR_LOCATION_BATHROOM)
    1, 0, 0, 1
#else
    0, 0, 0, 0
#endif
};

void relais_init(void) {
    Serial.begin(115200);

    for (int i = 0; i < SERIAL_RELAIS_COUNT; i++) {
        relais_set(i, initial_values[i]);
    }
}

int relais_count(void) {
    return SERIAL_RELAIS_COUNT;
}

void relais_set(int relais, int state) {
    if ((relais < 0) || (relais >= SERIAL_RELAIS_COUNT)) {
        return;
    }

    states[relais] = state;

    int cmd[4];
    cmd[0] = 0xA0; // command

    cmd[1] = relais + 1; // relais id, 1-4
    cmd[2] = state ? 1 : 0; // relais state

    cmd[3] = 0; // checksum
    for (int i = 0; i < 3; i++) {
        cmd[3] += cmd[i];
    }

    for (int i = 0; i < 4; i++) {
        Serial.write(cmd[i]);
    }

    delay(100);
}

int relais_get(int relais) {
    if ((relais < 0) || (relais >= SERIAL_RELAIS_COUNT)) {
        return 0;
    }

    return states[relais];
}

String relais_name(int relais) {
    if ((relais < 0) || (relais >= SERIAL_RELAIS_COUNT)) {
        return String(F("Unknown"));
    }

    return names[relais];
}

#elif defined(RELAIS_GPIO)

#define GPIO_RELAIS_COUNT 10

static int gpios[GPIO_RELAIS_COUNT] = {
    0, 15, 2, 4,
    16, 17, 5, 18,
    19, 23
};

static int states[GPIO_RELAIS_COUNT];

static String names[GPIO_RELAIS_COUNT] = {
    String(F("relais_0")),
    String(F("relais_1")),
    String(F("relais_2")),
    String(F("relais_3")),
    String(F("relais_4")),
    String(F("relais_5")),
    String(F("relais_6")),
    String(F("relais_7")),
    String(F("relais_8")),
    String(F("relais_9"))
};

void relais_init(void) {
    for (int i = 0; i < GPIO_RELAIS_COUNT; i++) {
        pinMode(gpios[i], OUTPUT);
        relais_set(i, 0);
    }
}

int relais_count(void) {
    return GPIO_RELAIS_COUNT;
}

void relais_set(int relais, int state) {
    if ((relais < 0) || (relais >= GPIO_RELAIS_COUNT)) {
        return;
    }

    states[relais] = state;
    digitalWrite(gpios[relais], state ? LOW : HIGH);
}

int relais_get(int relais) {
    if ((relais < 0) || (relais >= GPIO_RELAIS_COUNT)) {
        return 0;
    }

    return states[relais];
}

String relais_name(int relais) {
    if ((relais < 0) || (relais >= GPIO_RELAIS_COUNT)) {
        return String(F("Unknown"));
    }

    return names[relais];
}

#else

void relais_init(void) { }
int relais_count(void) { return 0; }
void relais_set(int relais, int state) { }
int relais_get(int relais) { return 0; }

#endif
