/*
 SHT2x - Humidity Library for Arduino.
 Get humidity and temperature from the HTU21D/SHT2x sensors.

 Hardware Setup:
 Attach the SDA pin to A4, SCL to A5.

 Software:
 Call SHT2x.Begin() in setup.
 SHT2x.ReadHumidity() will return a float containing the humidity. Ex: 54.7
 SHT2x.ReadTemperature() will return a float containing the temperature in Celsius. Ex: 24.1
 SHT2x.SetResolution(byte: 0b.76543210) sets the resolution of the readings.

  Copyright (C) 2015  Nuno Chaveiro  nchaveiro[at]gmail.com  Lisbon, Portugal
  Copyright (C) 2020  Thomas Buck <thomas@xythobuz.de>
*/

#include <Wire.h>
#include "SHT2x.h"

SHT2x::SHT2x(uint8_t _addr, TwoWire* _wire) {
    addr = _addr;
    wire = _wire;
}

void SHT2x::begin(void) {
    wire->begin();
}

bool SHT2x::GetAlive(void) {
    uint16_t r = readSensor(TRIGGER_TEMP_MEASURE_HOLD);
    if ((r == ERROR_TIMEOUT) || (r == ERROR_CRC)) {
        return false;
    }
    return true;
}

/**********************************************************
 * GetHumidity
 *  Gets the current humidity from the sensor.
 *
 * @return float - The relative humidity in %RH
 **********************************************************/
float SHT2x::GetHumidity(void) {
    return (-6.0 + 125.0 / 65536.0 * (float)(readSensor(TRIGGER_HUMD_MEASURE_HOLD)));
}

/**********************************************************
 * GetTemperature
 *  Gets the current temperature from the sensor.
 *
 * @return float - The temperature in Deg C
 **********************************************************/
float SHT2x::GetTemperature(void) {
    return (-46.85 + 175.72 / 65536.0 * (float)(readSensor(TRIGGER_TEMP_MEASURE_HOLD)));
}

/**********************************************************
 * Sets the sensor resolution to one of four levels:
 *  0/0 = 12bit RH, 14bit Temp
 *  0/1 = 8bit RH, 12bit Temp
 *  1/0 = 10bit RH, 13bit Temp
 *  1/1 = 11bit RH, 11bit Temp
 *  Power on default is 0/0
 **********************************************************/
void SHT2x::setResolution(uint8_t resolution) {
    uint8_t userRegister = read_user_register(); //Go get the current register state
    userRegister &= B01111110; //Turn off the resolution bits
    resolution &= B10000001; //Turn off all other bits but resolution bits
    userRegister |= resolution; //Mask in the requested resolution bits

    //Request a write to user register
    wire->beginTransmission(addr);
    wire->write(WRITE_USER_REG); //Write to the user register
    wire->write(userRegister); //Write the new resolution bits
    wire->endTransmission();
}

uint8_t SHT2x::read_user_register(void) {
    uint8_t userRegister;

    //Request the user register
    wire->beginTransmission(addr);
    wire->write(READ_USER_REG); //Read the user register
    wire->endTransmission();

    //Read result
    wire->requestFrom(addr, 1);

    userRegister = wire->read();

    return userRegister;
}

uint16_t SHT2x::readSensor(uint8_t command) {
    uint16_t result;

    wire->beginTransmission(addr); //begin
    wire->write(command);                     //send the pointer location
    wire->endTransmission();                  //end

    //Hang out while measurement is taken. 50mS max, page 4 of datasheet.
    delay(55);

    //Comes back in three bytes, data(MSB) / data(LSB) / Checksum
    wire->requestFrom(addr, 3);

    //Wait for data to become available
    int counter = 0;
    while (wire->available() < 3) {
        counter++;
        delay(1);
        if(counter > 100) return ERROR_TIMEOUT; //Error timout
    }

    //Store the result
    result = ((wire->read()) << 8);
    result |= wire->read();
    //Check validity
    uint8_t checksum = wire->read();
    if(check_crc(result, checksum) != 0) return(ERROR_CRC); //Error checksum
    //sensorStatus = rawTemperature & 0x0003; //get status bits
    result &= ~0x0003;   // clear two low bits (status bits)
    return result;
}

//Give this function the 2 byte message (measurement) and the check_value byte from the HTU21D
//If it returns 0, then the transmission was good
//If it returns something other than 0, then the communication was corrupted
//From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
//POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
#define SHIFTED_DIVISOR 0x988000 // This is the 0x0131 polynomial shifted to farthest left of three bytes

uint8_t SHT2x::check_crc(uint16_t message_from_sensor, uint8_t check_value_from_sensor) {
    // Pad with 8 bits because we have to add in the check value
    uint32_t remainder = (uint32_t)message_from_sensor << 8;

    // Add on the check value
    remainder |= check_value_from_sensor;
    uint32_t divsor = (uint32_t)SHIFTED_DIVISOR;

    // Operate on only 16 positions of max 24.
    // The remaining 8 are our remainder and should be zero when we're done.
    for (int i = 0; i < 16; i++) {
        // Check if there is a one in the left position
        if (remainder & ((uint32_t)1 << (23 - i))) {
            remainder ^= divsor;
        }

        // Rotate the divsor max 16 times so that we have 8 bits left of a remainder
        divsor >>= 1;
    }

    return (uint8_t)remainder;
}

