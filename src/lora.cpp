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
#include "lora.h"

#define OLED_BAT_INTERVAL (10UL * 1000UL) // in ms
#define LORA_LED_BRIGHTNESS 25 // in percent, 50% brightness is plenty for this LED

// Pause between transmited packets in seconds.
// Set to zero to only transmit a packet when pressing the user button
// Will not exceed 1% duty cycle, even if you set a lower value.
#define PAUSE               10

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
static unsigned long last_tx = 0, counter = 0, tx_time = 0, minimum_pause = 0;
static volatile bool rx_flag = false;

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

static void lora_rx(void) {
    rx_flag = true;
}

void lora_init(void) {
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

    // Start receiving
    RADIOLIB_CHECK(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
    if (!success) {
        use_lora = false;
        return;
    }
}

void lora_run(void) {
    heltec_loop();

#ifdef OLED_BAT_INTERVAL
    unsigned long time = millis();
    if ((time - last_bat_time) >= OLED_BAT_INTERVAL) {
        last_bat_time = time;
        print_bat();
    }
#endif

    if (!use_lora) {
        return;
    }

    // If a packet was received, display it and the RSSI and SNR
    if (rx_flag) {
        rx_flag = false;

        bool success = true;
        String data;
        RADIOLIB_CHECK(radio.readData(data));
        if (success) {
            debug.printf("RX [%s]\n", data.c_str());
            debug.printf("  RSSI: %.2f dBm\n", radio.getRSSI());
            debug.printf("  SNR: %.2f dB\n", radio.getSNR());
        }

        success = true;
        RADIOLIB_CHECK(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
        if (!success) {
            use_lora = false;
            return;
        }
    }

    bool tx_legal = millis() > last_tx + minimum_pause;

    // Transmit a packet every PAUSE seconds or when the button is pressed
    if ((PAUSE && tx_legal && millis() - last_tx > (PAUSE * 1000)) || button.isSingleClick()) {
        // In case of button click, tell user to wait
        if (!tx_legal) {
            debug.printf("Legal limit, wait %i sec.\n", (int)((minimum_pause - (millis() - last_tx)) / 1000) + 1);
            return;
        }

        debug.printf("TX [%s] ", String(counter).c_str());
        radio.clearDio1Action();

        heltec_led(LORA_LED_BRIGHTNESS);

        bool success = true;
        tx_time = millis();
        RADIOLIB_CHECK(radio.transmit(String(counter++).c_str()));
        tx_time = millis() - tx_time;

        heltec_led(0);

        if (success) {
            debug.printf("OK (%i ms)\n", (int)tx_time);
        } else {
            debug.printf("fail (%i)\n", _radiolib_status);
        }

        // Maximum 1% duty cycle
        minimum_pause = tx_time * 100;
        last_tx = millis();

        radio.setDio1Action(lora_rx);

        success = true;
        RADIOLIB_CHECK(radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF));
        if (!success) {
            use_lora = false;
            return;
        }
    }
}

#endif // FEATURE_LORA
