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

    if (found_sht) {
        mqtt.publish(SENSOR_LOCATION "/temperature", String(sht_temp()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/humidity", String(sht_humid()).c_str(), true);
#ifdef ENABLE_BME280
    } else if (found_bme1) {
        mqtt.publish(SENSOR_LOCATION "/temperature", String(bme1_temp()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/humidity", String(bme1_humid()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/pressure", String(bme1_pressure()).c_str(), true);
    } else if (found_bme2) {
        mqtt.publish(SENSOR_LOCATION "/temperature", String(bme2_temp()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/humidity", String(bme2_humid()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/pressure", String(bme2_pressure()).c_str(), true);
#endif // ENABLE_BME280
    }

#ifdef ENABLE_CCS811
    if (found_ccs1) {
        mqtt.publish(SENSOR_LOCATION "/eco2", String(ccs1_eco2()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/tvoc", String(ccs1_tvoc()).c_str(), true);
    } else if (found_ccs2) {
        mqtt.publish(SENSOR_LOCATION "/eco2", String(ccs2_eco2()).c_str(), true);
        mqtt.publish(SENSOR_LOCATION "/tvoc", String(ccs2_tvoc()).c_str(), true);
    }
#endif // ENABLE_CCS811
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

#ifdef FEATURE_RELAIS
    int state = 0;
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

    if (ps.indexOf("on") != -1) {
        state = 1;
    } else if (ps.indexOf("off") != -1) {
        state = 0;
    } else {
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
    }
}

void initMQTT() {
    debug.println(F("MQTT"));
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

#else

void initMQTT() { }
void runMQTT() { }

#endif // ENABLE_MQTT
