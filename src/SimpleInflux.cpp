/*
 * SimpleInflux.cpp
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
#include "DebugLog.h"
#include "SimpleInflux.h"

#if defined(ARDUINO_ARCH_AVR)

#include <WiFiLink.h>

WiFiClient client;

#endif

void InfluxData::addTag(const char *name, const char *value) {
    if (tag_count < SIMPLE_INFLUX_MAX_ELEMENTS) {
        tag_name[tag_count] = name;
        tag_value[tag_count] = value;
        tag_count++;
    }
}

void InfluxData::addValue(const char *name, double value) {
    if (value_count < SIMPLE_INFLUX_MAX_ELEMENTS) {
        value_name[value_count] = name;
        value_value[value_count] = value;
        value_count++;
    }
}

// https://docs.influxdata.com/influxdb/v1.8/guides/write_data/

boolean Influxdb::write(InfluxData &data) {
#if defined(ARDUINO_ARCH_AVR)

    debug.print(F("Writing "));
    debug.println(data.dataName());

    client.stop();

    if (client.connect(db_host, db_port)) {
        client.print(F("POST /write?db="));
        client.print(db_name);
        client.println(F(" HTTP/1.1"));

        client.print(F("Host: "));
        client.print(db_host);
        client.print(F(":"));
        client.println(String(db_port));

        client.println(F("Connection: close"));

        int len = strlen(data.dataName()) + 1;
        for (int i = 0; i < data.tagCount(); i++) {
            len += strlen(data.tagName(i)) + strlen(data.tagValue(i)) + 2;
        }
        for (int i = 0; i < data.valueCount(); i++) {
            len += strlen(data.valueName(i)) + String(data.valueValue(i)).length() + 1;
            if (i > 0) {
                len += 1;
            }
        }

        client.print(F("Content-Length: "));
        client.println(String(len));

        client.println();

        client.print(data.dataName());
        for (int i = 0; i < data.tagCount(); i++) {
            client.print(F(","));
            client.print(data.tagName(i));
            client.print(F("="));
            //client.print(F("\""));
            client.print(data.tagValue(i));
            //client.print(F("\""));
        }
        client.print(F(" "));
        for (int i = 0; i < data.valueCount(); i++) {
            if (i > 0) {
                client.print(F(","));
            }
            client.print(data.valueName(i));
            client.print(F("="));
            client.print(data.valueValue(i));
        }
        // we're leaving out the timestamp, it's optional

        boolean currentLineIsBlank = true, contains_error = false;
        int compare_off = 0;
        String compare_to(F("X-Influxdb-Error"));
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c != '\r') {
                    debug.write(c);
                }

                if (compare_off == compare_to.length()) {
                    contains_error = true;
                } else if ((compare_off < compare_to.length()) && (c == compare_to[compare_off])) {
                    if ((compare_off > 0) || currentLineIsBlank) {
                        compare_off++;
                    }
                } else {
                    compare_off = 0;
                }

                if ((c == '\n') && currentLineIsBlank) {
                    // http headers ended
                    break;
                }

                if (c == '\n') {
                    // you're starting a new line
                    currentLineIsBlank = true;
                } else if (c != '\r') {
                    // you've gotten a character on the current line
                    currentLineIsBlank = false;
                }
            }
        }

        client.stop();
        debug.println(contains_error ? F("Request failed") : F("Request Done"));
        return !contains_error;
    } else {
        debug.println(F("Error connecting"));
        return false; // failed
    }
#else

    return true; // success

#endif
}
