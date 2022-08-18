/*
 * sensors.h
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

#ifndef __SENSORS_H__
#define __SENSORS_H__

float sht_temp(void);
float sht_humid(void);
extern bool found_sht;

#ifdef ENABLE_BME280
float bme1_temp(void);
float bme2_temp(void);
float bme1_humid(void);
float bme2_humid(void);
float bme1_pressure(void);
float bme2_pressure(void);
extern bool found_bme1, found_bme2;
#endif // ENABLE_BME280

#ifdef ENABLE_CCS811
float ccs1_eco2(void);
float ccs1_tvoc(void);
float ccs2_eco2(void);
float ccs2_tvoc(void);
extern bool found_ccs1, found_ccs2;
extern bool ccs1_data_valid, ccs2_data_valid;
extern int ccs1_error_code, ccs2_error_code;
#endif // ENABLE_CCS811

void handleCalibrate();
void initSensors();
void runSensors();

#endif // __SENSORS_H__
