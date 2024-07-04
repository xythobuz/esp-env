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

#define SML_TX 33
#define SML_RX 34
#define SML_BAUD 9600
#define SML_PARAM SWSERIAL_8N1

static EspSoftwareSerial::UART port1, port2;
RTC_DATA_ATTR static unsigned long counter = 0;

static double SumWh = NAN, T1Wh = NAN, T2Wh = NAN;
static double SumW = NAN, L1W = NAN, L2W = NAN, L3W = NAN;

bool sml_data_received(void) {
    return counter > 0;
}

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

    port1.begin(SML_BAUD, SML_PARAM, SML_RX, -1, false);
    port2.begin(SML_BAUD, SML_PARAM, SML_TX, -1, false);
}

void sml_run(void) {
    if ((!port1.available()) && (!port2.available())) {
        return;
    }

    unsigned char c = port1.available() ? port1.read() : port2.read();
    sml_states_t s = smlState(c);

    if (s == SML_START) {
        init_vars();
        counter++;
    } else if (s == SML_LISTEND) {
        for (unsigned int i = 0; i < (sizeof(handlers) / sizeof(handlers[0])); i++) {
            if (smlOBISCheck(handlers[i].OBIS)) {
                handlers[i].Handler();
            }
        }
    } else if (s == SML_FINAL) {
        debug.println("SML Reading:");

        if (!isnan(SumWh)) {
            debug.printf("Sum: %14.3lf Wh\n", SumWh);
        }

        if (!isnan(T1Wh)) {
            debug.printf(" T1: %14.3lf Wh\n", T1Wh);
        }

        if (!isnan(T2Wh)) {
            debug.printf(" T2: %14.3lf Wh\n", T2Wh);
        }

        if (!isnan(SumW)) {
            debug.printf("Sum: %14.3lf W\n", SumW);

#ifdef FEATURE_LORA
            lora_sml_send(LORA_SML_SUM_W, SumW, counter);
#endif // FEATURE_LORA
        }

        if (!isnan(L1W)) {
            debug.printf(" L1: %14.3lf W\n", L1W);
#ifdef FEATURE_LORA
            lora_sml_send(LORA_SML_L1_W, L1W, counter);
#endif // FEATURE_LORA
        }

        if (!isnan(L2W)) {
            debug.printf(" L2: %14.3lf W\n", L2W);

#ifdef FEATURE_LORA
            lora_sml_send(LORA_SML_L2_W, L2W, counter);
#endif // FEATURE_LORA
        }

        if (!isnan(L3W)) {
            debug.printf(" L3: %14.3lf W\n", L3W);

#ifdef FEATURE_LORA
            lora_sml_send(LORA_SML_L3_W, L3W, counter);
#endif // FEATURE_LORA
        }

        debug.println();

#ifdef FEATURE_LORA
        // the power readings (Watt) are just sent as is, if available.
        // the energy readings are consolidated if possible, to avoid
        // unneccessary data transmission
        if ((!isnan(SumWh)) && (!isnan(T1Wh))
                && (fabs(SumWh - T1Wh) < 0.1)
                && ((isnan(T2Wh)) || (fabs(T2Wh) < 0.1))) {
            // (SumWh == T1Wh) && ((T2Wh == 0) || (T2Wh == NAN))
            // just send T1Wh
            lora_sml_send(LORA_SML_T1_WH, T1Wh, counter);
        } else if ((!isnan(SumWh)) && (!isnan(T2Wh))
                && (fabs(SumWh - T2Wh) < 0.1)
                && ((isnan(T1Wh)) || (fabs(T1Wh) < 0.1))) {
            // (SumWh == T2Wh) && ((T1Wh == 0) || (T1Wh == NAN))
            // just send T2Wh
            lora_sml_send(LORA_SML_T2_WH, T2Wh, counter);
        } else {
            // just do normal sending if available
            if (!isnan(SumWh)) {
                lora_sml_send(LORA_SML_SUM_WH, SumWh, counter);
            }
            if (!isnan(T1Wh)) {
                lora_sml_send(LORA_SML_T1_WH, T1Wh, counter);
            }
            if (!isnan(T2Wh)) {
                lora_sml_send(LORA_SML_T2_WH, T2Wh, counter);
            }
        }

        // update own battery state with each sml readout
        lora_sml_send(LORA_SML_BAT_V, lora_get_mangled_bat(), counter);

        lora_sml_done();
#endif // FEATURE_LORA
    }
}

#endif // FEATURE_SML
