/*
 * smart_meter.cpp
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

#ifdef FEATURE_SML

#include <Arduino.h>
#include <sml.h>
#include <SoftwareSerial.h>

#include "config.h"
#include "DebugLog.h"
#include "lora.h"
#include "smart_meter.h"

#define SML_TX 47
#define SML_RX 48

static EspSoftwareSerial::UART port1, port2;

static double SumWh = NAN, T1Wh = NAN, T2Wh = NAN;
static double SumW = NAN, L1W = NAN, L2W = NAN, L3W = NAN;

static void EnergySum(void) {
    smlOBISWh(SumWh);
}

static void EnergyT1(void) {
    smlOBISWh(T1Wh);
}

static void EnergyT2(void) {
    smlOBISWh(T2Wh);
}

static void PowerSum(void) {
    smlOBISW(SumW);
}

static void PowerL1(void) {
    smlOBISW(L1W);
}

static void PowerL2(void) {
    smlOBISW(L2W);
}

static void PowerL3(void) {
    smlOBISW(L3W);
}

static void init_vars(void) {
    SumWh = NAN;
    T1Wh = NAN;
    T2Wh = NAN;
    SumW = NAN;
    L1W = NAN;
    L2W = NAN;
    L3W = NAN;
}

typedef struct {
    const unsigned char OBIS[6];
    void (*Handler)();
} OBISHandler;

static OBISHandler handlers[] = {
    { { 1, 0,  1, 8, 0, 255 }, EnergySum }, // 1-0: 1.8.0*255 (T1 + T2) Wh
    { { 1, 0,  1, 8, 1, 255 }, EnergyT1 },  // 1-0: 1.8.1*255 (T1) Wh
    { { 1, 0,  1, 8, 2, 255 }, EnergyT2 },  // 1-0: 1.8.2*255 (T2) Wh
    { { 1, 0, 16, 7, 0, 255 }, PowerSum },  // 1-0:16.7.0*255 (L1 + L2 + L3) W
    { { 1, 0, 21, 7, 0, 255 }, PowerL1 },   // 1-0:21.7.0*255 (L1) W
    { { 1, 0, 41, 7, 0, 255 }, PowerL2 },   // 1-0:41.7.0*255 (L2) W
    { { 1, 0, 61, 7, 0, 255 }, PowerL3 },   // 1-0:61.7.0*255 (L3) W
};

void sml_init(void) {
    init_vars();

    port1.begin(9600, SWSERIAL_8N1, SML_RX, -1, false);
    port2.begin(9600, SWSERIAL_8N1, SML_TX, -1, false);
}

void sml_run(void) {
    if ((!port1.available()) && (!port2.available())) {
        return;
    }

    unsigned char c = port1.available() ? port1.read() : port2.read();
    sml_states_t s = smlState(c);

    if (s == SML_START) {
        init_vars();
    } else if (s == SML_LISTEND) {
        for (unsigned int i = 0; i < (sizeof(handlers) / sizeof(handlers[0])); i++) {
            if (smlOBISCheck(handlers[i].OBIS)) {
                handlers[i].Handler();
            }
        }
    } else if (s == SML_FINAL) {
        debug.println("SML Reading:");

        if (!isnan(SumWh))
            debug.printf("Sum: %14.3lf Wh\n", SumWh);

        if (!isnan(T1Wh))
            debug.printf(" T1: %14.3lf Wh\n", T1Wh);

        if (!isnan(T2Wh))
            debug.printf(" T2: %14.3lf Wh\n", T2Wh);

        if (!isnan(SumW))
            debug.printf("Sum: %14.3lf W\n", SumW);

        if (!isnan(L1W))
            debug.printf(" L1: %14.3lf W\n", L1W);

        if (!isnan(L2W))
            debug.printf(" L2: %14.3lf W\n", L2W);

        if (!isnan(L3W))
            debug.printf(" L3: %14.3lf W\n", L3W);

        debug.println();

#ifdef FEATURE_LORA
        lora_sml_send(SumWh, T1Wh, T2Wh, SumW, L1W, L2W, L3W);
#endif // FEATURE_LORA
    }
}

#endif // FEATURE_SML
