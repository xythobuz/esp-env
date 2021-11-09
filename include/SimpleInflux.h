/*
 * SimpleInflux.h
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

#ifndef __ESP_SIMPLE_INFLUX__
#define __ESP_SIMPLE_INFLUX__

#define SIMPLE_INFLUX_MAX_ELEMENTS 3

class InfluxData {
  public:
    InfluxData(const char *name) : data_name(name), tag_count(0), value_count(0) { }
    void addTag(const char *name, const char *value);
    void addTag(const char *name, String &value) { addTag(name, value.c_str()); }
    void addValue(const char *name, double value);

    int tagCount() { return tag_count; }
    const char *tagName(int tag) { return tag_name[tag < tag_count ? tag : 0]; }
    const char *tagValue(int tag) { return tag_value[tag < tag_count ? tag : 0]; }

    int valueCount() { return value_count; }
    const char *valueName(int val) { return value_name[val < value_count ? val : 0]; }
    double valueValue(int val) { return value_value[val < value_count ? val : 0]; }

    const char *dataName() { return data_name; }
    void setName(const char *n) { data_name = n; }

    void clear() { tag_count = 0; value_count = 0;}

  private:
    const char *data_name;

    const char *tag_name[SIMPLE_INFLUX_MAX_ELEMENTS];
    const char *tag_value[SIMPLE_INFLUX_MAX_ELEMENTS];
    int tag_count;

    const char *value_name[SIMPLE_INFLUX_MAX_ELEMENTS];
    double value_value[SIMPLE_INFLUX_MAX_ELEMENTS];
    int value_count;
};

class Influxdb {
  public:
    Influxdb(const char *host, int port) : db_host(host), db_port(port) { }
    void setDb(const char *db) { db_name = db; }
    boolean write(InfluxData &data);

  private:
      const char *db_host;
      int db_port;
      const char *db_name;
};

#endif // __ESP_SIMPLE_INFLUX__
