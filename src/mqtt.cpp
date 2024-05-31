/*
 * mqtt.cpp
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
#include "DebugLog.h"
#include "sensors.h"
#include "relais.h"
#include "influx.h"
#include "ui.h"
#include "mqtt.h"

#ifdef ENABLE_MQTT

#if defined(ARDUINO_ARCH_ESP8266)

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#elif defined(ARDUINO_ARCH_ESP32)

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#elif defined(ARDUINO_ARCH_AVR)

#include <UnoWiFiDevEdSerial1.h>
#include <WiFiLink.h>

#endif

#include <PubSubClient.h>

WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);
static unsigned long last_mqtt_reconnect_time = 0;
static unsigned long last_mqtt_write_time = 0;

static void writeMQTT() {
    if (!mqtt.connected()) {
        return;
    }

    bool wrote = false;

    if (found_sht) {
        mqtt.publish(SENSOR_LOCATION "/temperature", String(sht_temp()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/humidity", String(sht_humid()).c_str(), true);
        wrote = true;
#ifdef ENABLE_BME280
    } else if (found_bme1) {
        mqtt.publish(SENSOR_LOCATION "/temperature", String(bme1_temp()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/humidity", String(bme1_humid()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/pressure", String(bme1_pressure()).c_str(), true);
        wrote = true;
    } else if (found_bme2) {
        mqtt.publish(SENSOR_LOCATION "/temperature", String(bme2_temp()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/humidity", String(bme2_humid()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/pressure", String(bme2_pressure()).c_str(), true);
        wrote = true;
#endif // ENABLE_BME280
    }

#ifdef ENABLE_CCS811
    if (found_ccs1) {
        mqtt.publish(SENSOR_LOCATION "/eco2", String(ccs1_eco2()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/tvoc", String(ccs1_tvoc()).c_str(), true);
        wrote = true;
    } else if (found_ccs2) {
        mqtt.publish(SENSOR_LOCATION "/eco2", String(ccs2_eco2()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/tvoc", String(ccs2_tvoc()).c_str(), true);
        wrote = true;
    }
#endif // ENABLE_CCS811

    if (wrote) {
        debug.println(F("Updated MQTT sensor values"));
    }
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String ts(topic), ps;
    for (unsigned int i = 0; i < length; i++) {
        char c = payload[i];
        ps += c;
    }

    debug.print(F("MQTT Rx @ \""));
    debug.print(ts);
    debug.print(F("\" = \""));
    debug.print(ps);
    debug.println(F("\""));

#if defined(FEATURE_RELAIS) || defined(FEATURE_UI)
    int state = 0;
    if (ps.indexOf("on") != -1) {
        state = 1;
    } else if (ps.indexOf("off") != -1) {
        state = 0;
    } else {
        return;
    }
#endif

#ifdef FEATURE_RELAIS
    int id = -1;

    String our_topic(SENSOR_LOCATION);
    our_topic += "/";

    if (!ts.startsWith(our_topic)) {
        debug.print(F("Unknown MQTT room "));
        debug.println(ts);
        return;
    }

    String ids = ts.substring(our_topic.length());
    for (int i = 0; i < relais_count(); i++) {
        if (ids == relais_name(i)) {
            id = i;
            break;
        }
    }

    if (id < 0) {
        debug.print(F("Unknown MQTT topic "));
        debug.println(ts);
        return;
    }

    if ((id >= 0) && (id < relais_count())) {
        debug.print(F("Turning "));
        debug.print(state ? "on" : "off");
        debug.print(F(" relais "));
        debug.println(id);

        relais_set(id, state);

        writeDatabase();
    }
#endif // FEATURE_RELAIS

#ifdef FEATURE_UI
    // store new topic values for display
    if (ts == "livingroom/light_kitchen") {
        ui_status.light_kitchen = state ? true : false;
        ui_progress(UI_UPDATE);
    } else if (ts == "livingroom/light_pc") {
        ui_status.light_pc = state ? true : false;
        ui_progress(UI_UPDATE);
    } else if (ts == "livingroom/light_bench") {
        ui_status.light_bench = state ? true : false;
        ui_progress(UI_UPDATE);
    } else if (ts == "livingroom/light_amp") {
        ui_status.light_amp = state ? true : false;
        ui_progress(UI_UPDATE);
    } else if (ts == "livingroom/light_box") {
        ui_status.light_box = state ? true : false;
        ui_progress(UI_UPDATE);
    } else if (ts == "livingroom/light_corner/cmnd/POWER") {
        ui_status.light_corner = state ? true: false;
        ui_progress(UI_UPDATE);
    } else if (ts == "livingroom/workbench/cmnd/POWER") {
        ui_status.light_workspace = state ? true : false;
        ui_progress(UI_UPDATE);
    } else if (ts == "livingroom/amp/cmnd/POWER") {
        ui_status.sound_amplifier = state ? true : false;
        ui_progress(UI_UPDATE);
    }
#endif // FEATURE_UI
}

static void mqttReconnect() {
    // Create a random client ID
    String clientId = F("ESP-" SENSOR_ID "-");
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
#if defined(MQTT_USER) && defined(MQTT_PASS)
    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
#else
    if (mqtt.connect(clientId.c_str())) {
#endif

#ifdef FEATURE_RELAIS
        for (int i = 0; i < relais_count(); i++) {
            String topic(SENSOR_LOCATION);
            topic += String("/") + relais_name(i);
            mqtt.subscribe(topic.c_str());
        }
#endif // FEATURE_RELAIS

#ifdef FEATURE_UI
        // TODO need to list all topics we're interested in?
        mqtt.subscribe("livingroom/light_kitchen");
        mqtt.subscribe("livingroom/light_pc");
        mqtt.subscribe("livingroom/light_bench");
        mqtt.subscribe("livingroom/light_amp");
        mqtt.subscribe("livingroom/light_box");
        mqtt.subscribe("livingroom/light_corner/cmnd/POWER");
        mqtt.subscribe("livingroom/workbench/cmnd/POWER");
        mqtt.subscribe("livingroom/amp/cmnd/POWER");
#endif // FEATURE_UI
    }
}

void initMQTT() {
    mqtt.setServer(MQTT_HOST, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
}

void runMQTT() {
    unsigned long time = millis();

    if ((time - last_mqtt_write_time) >= MQTT_WRITE_INTERVAL) {
        last_mqtt_write_time = time;
        writeMQTT();
    }

    if (!mqtt.connected() && ((millis() - last_mqtt_reconnect_time) >= MQTT_RECONNECT_INTERVAL)) {
        last_mqtt_reconnect_time = millis();
        mqttReconnect();
    }

    mqtt.loop();
}

#ifdef FEATURE_UI
static struct ui_status prev_status = ui_status;

void writeMQTT_UI(void) {
    struct ui_status curr_status = ui_status;
    if (curr_status.light_amp != prev_status.light_amp) {
        mqtt.publish("livingroom/light_amp", curr_status.light_amp ? "on" : "off", true);
    } else if (curr_status.light_bench != prev_status.light_bench) {
        mqtt.publish("livingroom/light_bench", curr_status.light_bench ? "on" : "off", true);
    } else if (curr_status.light_box != prev_status.light_box) {
        mqtt.publish("livingroom/light_box", curr_status.light_box ? "on" : "off", true);
    } else if (curr_status.light_corner != prev_status.light_corner) {
        mqtt.publish("livingroom/light_corner/cmnd/POWER", curr_status.light_corner ? "on" : "off", true);
    } else if (curr_status.light_kitchen != prev_status.light_kitchen) {
        mqtt.publish("livingroom/light_kitchen", curr_status.light_kitchen ? "on" : "off", true);
    } else if (curr_status.light_pc != prev_status.light_pc) {
        mqtt.publish("livingroom/light_pc", curr_status.light_pc ? "on" : "off", true);
    } else if (curr_status.light_workspace != prev_status.light_workspace) {
        mqtt.publish("livingroom/workbench/cmnd/POWER", curr_status.light_workspace ? "on" : "off", true);
    } else if (curr_status.sound_amplifier != prev_status.sound_amplifier) {
        mqtt.publish("livingroom/amp/cmnd/POWER", curr_status.sound_amplifier ? "on" : "off", true);
    }

    prev_status = curr_status;
}
#endif

#else

void initMQTT() { }
void runMQTT() { }

#endif // ENABLE_MQTT
