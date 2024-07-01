/*
 * lora.cpp
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

#ifdef FEATURE_LORA

#include <Arduino.h>

// Turns the 'PRG' button into the power button, long press is off
#define HELTEC_POWER_BUTTON

#include <heltec_unofficial.h>

#include "config.h"
#include "DebugLog.h"
#include "influx.h"
#include "lora.h"

//#define DEBUG_LORA_RX_HEXDUMP

#ifdef FEATURE_SML
#define LORA_LED_BRIGHTNESS 0 // in percent, 50% brightness is plenty for this LED
#define OLED_BAT_INTERVAL (2UL * 60UL * 1000UL) // in ms
#define FORCE_BAT_SEND_AT_OLED_INTERVAL
#else // FEATURE_SML
#define LORA_LED_BRIGHTNESS 25 // in percent, 50% brightness is plenty for this LED
#endif // FEATURE_SML

// Frequency in MHz. Keep the decimal point to designate float.
// Check your own rules and regulations to see what is legal where you are.
#define FREQUENCY           866.3       // for Europe
// #define FREQUENCY           905.2       // for US

// LoRa bandwidth. Keep the decimal point to designate float.
// Allowed values are 7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125.0, 250.0 and 500.0 kHz.
#define BANDWIDTH           250.0

// Number from 5 to 12. Higher means slower but higher "processor gain",
// meaning (in nutshell) longer range and more robust against interference.
#define SPREADING_FACTOR    9

// Transmit power in dBm. 0 dBm = 1 mW, enough for tabletop-testing. This value can be
// set anywhere between -9 dBm (0.125 mW) to 22 dBm (158 mW). Note that the maximum ERP
// (which is what your antenna maximally radiates) on the EU ISM band is 25 mW, and that
// transmissting without an antenna can damage your hardware.
// 25mW = 14dBm
#define MAX_TX_POWER        14
#define ANTENNA_GAIN        5
#define TRANSMIT_POWER      (MAX_TX_POWER - ANTENNA_GAIN)

#define RADIOLIB_xy(action) \
    debug.print(#action); \
    debug.print(" = "); \
    debug.print(state); \
    debug.print(" ("); \
    debug.print(radiolib_result_string(state)); \
    debug.println(")");

#define RADIOLIB_CHECK(action) do { \
    int state = action; \
    if (state != RADIOLIB_ERR_NONE) { \
        RADIOLIB_xy(action); \
        success = false; \
    } \
} while (false);

static unsigned long last_bat_time = 0;
static bool use_lora = true;
static unsigned long last_tx = 0, tx_time = 0, minimum_pause = 0;
static volatile bool rx_flag = false;

#ifdef FEATURE_SML

struct sml_cache {
    double value, next_value;
    bool ready, has_next;
    unsigned long counter, next_counter;
};

static struct sml_cache cache[LORA_SML_NUM_MESSAGES];

#endif // FEATURE_SML

void lora_oled_init(void) {
    heltec_setup();
}

void lora_oled_print(String s) {
    display.print(s);
}

static void print_bat(void) {
    float vbat = heltec_vbat();
    debug.printf("Vbat: %.2fV (%d%%)\n", vbat, heltec_battery_percent(vbat));
}

double lora_get_mangled_bat(void) {
    uint8_t data[sizeof(double)];
    float vbat = heltec_vbat();
    int percent = heltec_battery_percent(vbat);
    memcpy(data, &vbat, sizeof(float));
    memcpy(data + sizeof(float), &percent, sizeof(int));
    return *((double *)data);
}

// adapted from "Hacker's Delight"
static uint32_t calc_checksum(const uint8_t *data, size_t len) {
    uint32_t c = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        c ^= data[i];
        for (size_t j = 0; j < 8; j++) {
            uint32_t mask = -(c & 1);
            c = (c >> 1) ^ (0xEDB88320 & mask);
        }
    }

    return ~c;
}

static void lora_rx(void) {
    rx_flag = true;
}

static bool lora_tx(enum lora_sml_type type, double value) {
    bool tx_legal = millis() > (last_tx + minimum_pause);
    if (!tx_legal) {
        return false;
    }

    struct lora_sml_msg msg;
    msg.type = type;
    msg.value = value;
    msg.checksum = calc_checksum((uint8_t *)&msg, offsetof(struct lora_sml_msg, checksum));

    uint8_t *data = (uint8_t *)&msg;
    const size_t len = sizeof(struct lora_sml_msg);

    debug.printf("TX [%d] (%lu) ", data[0], len);

#ifdef LORA_XOR_KEY
    for (size_t i = 0; i < len; i++) {
        data[i] ^= LORA_XOR_KEY[i];
    }
#endif

    radio.clearDio1Action();

    heltec_led(LORA_LED_BRIGHTNESS);

    bool success = true;
    tx_time = millis();
    RADIOLIB_CHECK(radio.transmit(data, len));
    tx_time = millis() - tx_time;

    heltec_led(0);

    bool r = true;
    if (success) {
        debug.printf("OK (%i ms)\n", (int)tx_time);
    } else {
        debug.println("fail");
        r = false;
    }

    // Maximum 1% duty cycle
    minimum_pause = tx_time * 100;
    last_tx = millis();

    radio.setDio1Action(lora_rx);

#ifndef FEATURE_SML
    success = true;
    RADIOLIB_CHECK(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
    if (!success) {
        use_lora = false;
    }
#endif // ! FEATURE_SML

    return r;
}

#ifdef FEATURE_SML
static bool lora_sml_cache_send(enum lora_sml_type msg) {
    return lora_tx(msg, cache[msg].value);
}

static void lora_sml_handle_cache(void) {
    // find smallest message counter that is ready
    unsigned long min_counter = ULONG_MAX;
    for (int i = 0; i < LORA_SML_NUM_MESSAGES; i++) {
        if (cache[i].ready && (cache[i].counter < min_counter)) {
            min_counter = cache[i].counter;
        }
    }

    // try to transmit next value with lowest counter
    for (int i = 0; i < LORA_SML_NUM_MESSAGES; i++) {
        if (cache[i].ready && (cache[i].counter == min_counter)) {
            if (lora_sml_cache_send((enum lora_sml_type)i)) {
                if (cache[i].has_next) {
                    cache[i].has_next = false;
                    cache[i].value = cache[i].next_value;
                    cache[i].counter = cache[i].next_counter;
                } else {
                    cache[i].ready = false;
                }
            }
        }
    }
}

void lora_sml_send(enum lora_sml_type msg, double value, unsigned long counter) {
    if (cache[msg].ready) {
        // still waiting to be transmitted, so cache for next cycle
        cache[msg].has_next = true;
        cache[msg].next_value = value;
        cache[msg].next_counter = counter;
    } else {
        // cache as current value, for transmission in this cycle
        cache[msg].ready = true;
        cache[msg].value = value;
        cache[msg].counter = counter;
    }
}
#endif // FEATURE_SML

void lora_init(void) {
#ifdef FEATURE_SML
    for (int i = 0; i < LORA_SML_NUM_MESSAGES; i++) {
        cache[i].value = NAN;
        cache[i].next_value = NAN;
        cache[i].ready = false;
        cache[i].has_next = false;
        cache[i].counter = 0;
        cache[i].next_counter = 0;
    }
#endif // FEATURE_SML

    print_bat();

    bool success = true;

    RADIOLIB_CHECK(radio.begin());
    if (!success) {
        use_lora = false;
        return;
    }

    radio.setDio1Action(lora_rx);

    debug.printf("Frequency: %.2f MHz\n", FREQUENCY);
    RADIOLIB_CHECK(radio.setFrequency(FREQUENCY));
    if (!success) {
        use_lora = false;
        return;
    }

    debug.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
    RADIOLIB_CHECK(radio.setBandwidth(BANDWIDTH));
    if (!success) {
        use_lora = false;
        return;
    }

    debug.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
    RADIOLIB_CHECK(radio.setSpreadingFactor(SPREADING_FACTOR));
    if (!success) {
        use_lora = false;
        return;
    }

    debug.printf("TX power: %i dBm\n", TRANSMIT_POWER);
    RADIOLIB_CHECK(radio.setOutputPower(TRANSMIT_POWER));
    if (!success) {
        use_lora = false;
        return;
    }

#ifndef FEATURE_SML
    // Start receiving
    RADIOLIB_CHECK(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
    if (!success) {
        use_lora = false;
        return;
    }
#endif // ! FEATURE_SML

#ifdef FEATURE_SML
    // turn on Ve external 3.3V to power Smart Meter reader
    heltec_ve(true);

    // send hello msg after boot
    lora_sml_send(LORA_SML_HELLO, heltec_temperature(), 0);
#endif // FEATURE_SML
}

void lora_run(void) {
    heltec_loop();

#ifdef OLED_BAT_INTERVAL
    unsigned long time = millis();
    if (((time - last_bat_time) >= OLED_BAT_INTERVAL) || (last_bat_time == 0)) {
        last_bat_time = time;
        print_bat();

#ifdef FORCE_BAT_SEND_AT_OLED_INTERVAL
        lora_sml_send(LORA_SML_BAT_V, lora_get_mangled_bat(), 0);
#endif // FORCE_BAT_SEND_AT_OLED_INTERVAL
    }
#endif

    if (!use_lora) {
        return;
    }

    if (rx_flag) {
        rx_flag = false;

        bool success = true;
        uint8_t data[sizeof(struct lora_sml_msg)];
        RADIOLIB_CHECK(radio.readData(data, sizeof(data)));
        if (success) {
#ifdef LORA_XOR_KEY
            for (size_t i = 0; i < sizeof(data); i++) {
                data[i] ^= LORA_XOR_KEY[i];
            }
#endif

            debug.printf("RX [%i]\n", data[0]);
            debug.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
            debug.printf("  SNR: %.2f dB\n", radio.getSNR());

#if defined(DEBUG_LORA_RX_HEXDUMP) || (!defined(ENABLE_INFLUXDB_LOGGING))
            for (int i = 0; i < sizeof(data); i++) {
                debug.printf("  %02X", data[i]);
                if (i < (sizeof(data) - 1)) {
                    debug.print(" ");
                } else {
                    debug.println();
                }
            }
#endif

            struct lora_sml_msg *msg = (struct lora_sml_msg *)data;
            uint32_t checksum = calc_checksum(data, offsetof(struct lora_sml_msg, checksum));
            if (checksum != msg->checksum) {
                debug.printf("  CRC: 0x%08X != 0x%08X\n", msg->checksum, checksum);
            } else {
                debug.printf("  CRC: OK 0x%08X\n", checksum);

#ifdef ENABLE_INFLUXDB_LOGGING
                if (data[0] == LORA_SML_BAT_V) {
                    // extract mangled float and int from double
                    float vbat = NAN;
                    int percent = -1;
                    memcpy(&vbat, data + offsetof(struct lora_sml_msg, value), sizeof(float));
                    memcpy(&percent, data + offsetof(struct lora_sml_msg, value) + sizeof(float), sizeof(int));
                    debug.printf("  Vbat: %.2f (%d%%)\n", vbat, percent);

                    writeSensorDatum("environment", "sml", SENSOR_LOCATION, "vbat", vbat);
                    writeSensorDatum("environment", "sml", SENSOR_LOCATION, "percent", percent);
                } else {
                    debug.printf("  Value: %.2f\n", msg->value);

                    String key;
                    switch (data[0]) {
                        case LORA_SML_HELLO:
                            key = "hello";
                            break;

                        case LORA_SML_SUM_WH:
                            key = "Sum_Wh";
                            break;

                        case LORA_SML_T1_WH:
                            key = "T1_Wh";
                            break;

                        case LORA_SML_T2_WH:
                            key = "T2_Wh";
                            break;

                        case LORA_SML_SUM_W:
                            key = "Sum_W";
                            break;

                        case LORA_SML_L1_W:
                            key = "L1_W";
                            break;

                        case LORA_SML_L2_W:
                            key = "L2_W";
                            break;

                        case LORA_SML_L3_W:
                            key = "L3_W";
                            break;

                        default:
                            key = "unknown";
                            break;
                    }

                    writeSensorDatum("environment", "sml", SENSOR_LOCATION, key, msg->value);
                }
#endif // ENABLE_INFLUXDB_LOGGING
            }
        }

#ifndef FEATURE_SML
        success = true;
        RADIOLIB_CHECK(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
        if (!success) {
            use_lora = false;
            return;
        }
#endif // ! FEATURE_SML
    }

#ifdef FEATURE_SML
    lora_sml_handle_cache();
#endif // FEATURE_SML

    if (button.isSingleClick()) {
        // In case of button click, tell user to wait
        bool tx_legal = millis() > last_tx + minimum_pause;
        if (!tx_legal) {
            debug.printf("Legal limit, wait %i sec.\n", (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
            return;
        }

        // send test hello message on lorarx target, or battery state on loratx target
#ifdef FEATURE_SML
        lora_sml_send(LORA_SML_BAT_V, lora_get_mangled_bat(), 0);
#else // FEATURE_SML
        lora_tx(LORA_SML_HELLO, heltec_temperature());
#endif // FEATURE_SML
    }
}

#endif // FEATURE_LORA
