/*
 * influx.h
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

#ifndef __INFLUX_H__
#define __INFLUX_H__

void initInflux();
void runInflux();
void writeDatabase();

void writeSensorDatum(String measurement, String sensor, String placement, String key, double value);

#endif // __INFLUX_H__
