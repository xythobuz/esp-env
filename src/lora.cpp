/*
 * lora.cpp
 *
 * ESP8266 / ESP32 Environmental Sensor
 *
 * https://github.com/beegee-tokyo/SX126x-Arduino/blob/master/examples/PingPongPio/src/main.cpp
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
#include <SX126x-Arduino.h>

#include "config.h"
#include "DebugLog.h"
#include "lora.h"

// Define LoRa parameters
#define RF_FREQUENCY               868000000 // Hz
#define TX_OUTPUT_POWER            22 // dBm
#define LORA_BANDWIDTH             0 // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR      7 // [SF7..SF12]
#define LORA_CODINGRATE            1 // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH       8 // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT        0 // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON       false
#define RX_TIMEOUT_VALUE           3000
#define TX_TIMEOUT_VALUE           3000

#define BUFFER_SIZE 64 // Define the payload size here

static hw_config hwConfig;
static RadioEvents_t RadioEvents;

void lora_init(void) {
    // Define the HW configuration between MCU and SX126x
    hwConfig.CHIP_TYPE = SX1262_CHIP;

    // TODO pin config!
    hwConfig.PIN_LORA_RESET = 4;
    hwConfig.PIN_LORA_NSS = 5;
    hwConfig.PIN_LORA_SCLK = 18;
    hwConfig.PIN_LORA_MISO = 19;
    hwConfig.PIN_LORA_DIO_1 = 21;
    hwConfig.PIN_LORA_BUSY = 22;
    hwConfig.PIN_LORA_MOSI = 23;
    hwConfig.RADIO_TXEN = 26;
    hwConfig.RADIO_RXEN = 27;

    hwConfig.USE_DIO2_ANT_SWITCH = true;
    hwConfig.USE_DIO3_TCXO = true;
    hwConfig.USE_DIO3_ANT_SWITCH = false;

    // Initialize the LoRa chip
    debug.println("Starting lora_hardware_init");
    lora_hardware_init(hwConfig);

    // Initialize the Radio callbacks
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;
    RadioEvents.CadDone = OnCadDone;

    // Initialize the Radio
    Radio.Init(&RadioEvents);

    // Set Radio channel
    Radio.SetChannel(RF_FREQUENCY);

    // Set Radio TX configuration
    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                      LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                      LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                      true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

    // Set Radio RX configuration
    Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                      LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                      LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                      0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

    // Start LoRa
    debug.println("Starting Radio.Rx");
    Radio.Rx(RX_TIMEOUT_VALUE);
}

void lora_run(void) {
#ifdef ARDUINO_ARCH_ESP8266
    // Handle Radio events
    Radio.IrqProcess();
#endif // ARDUINO_ARCH_ESP8266
}

#endif // FEATURE_LORA
