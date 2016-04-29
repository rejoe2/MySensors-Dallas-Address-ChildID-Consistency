/**
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.
   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2015 Sensnology AB
   Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
 *******************************
   DESCRIPTION
   Example sketch showing how to send in DS1820B OneWire temperature readings back to the controller
   http://www.mysensors.org/build/temp
   Enhanced Version to keep SensorID icluding a lot of ideas from leodesigner and hauswart (forum.fhem.de)
   Improvements:
   - it will remember a sensor index in EEPROM (2 bytes per sensor)
      in case if you need to replace or add new sensor to the 1Wire bus - all other sensors will keep own sensor index#
  Last modifications:
  - Revert back to standard Mysensor EEPROM storage (in line with leodesigner code base)
  - First ID for DS has to be defined
  - Bug fixed with not changed "spot_used" value
  - Present Dallas-Hardware-ID as comment (still not working!!!)
  - Improve reading of code by opting for some more subroutines
*/

// Enable debug prints to serial monitor
#define MY_DEBUG
// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensor.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define COMPARE_TEMP 1 // Send temperature only if changed?
#define ERASE_HASH 1 // Clear EEPROM, if no 1w-device is present? 1 = Yes, 0 = No
#define SEND_ID // Send also Dallas-Addresses?
#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
#define EEPROM_DEVICE_ADDR_START 64 // start byte in eeprom for remembering our sensors
#define EEPROM_DEVICE_ADDR_END EEPROM_DEVICE_ADDR_START+MAX_ATTACHED_DS18B20*2 // end byte in eeprom for remembering our sensors

uint8_t DS_First_Child_ID = 7; //First Child-ID to be used by Dallas Bus; set this to be higher than other Child-ID's who need EEPROM storage to avoid conflicts
uint16_t SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.
float lastTemperature[MAX_ATTACHED_DS18B20];
uint8_t numSensors = 0;
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address
char* charAddr = "Check for faults";
uint8_t ts_spot[MAX_ATTACHED_DS18B20]; // array for matching bus-id to EEPROM-index
bool spot_used[MAX_ATTACHED_DS18B20]; // used spot array
// Initialize temperature message
MyMessage msgTemp(0, V_TEMP);
#ifdef SEND_ID
MyMessage msgId(0, V_ID);
#endif
boolean receivedConfig = false;
boolean metric = true;

// Initialize temperature message
void setup() {
  // Startup up the OneWire library
  sensors.begin();
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
  // Fetch the number of attached temperature sensors
  numSensors = sensors.getDeviceCount();
  initialiseIdArray();
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Temperature Sensor", "1.3");
  // Present all sensors to controller
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    sensors.getAddress(tempDeviceAddress, i);
    Serial.print("Hardware presented: ");
    charAddr = addrToChar(tempDeviceAddress);
    Serial.println(charAddr);
    present(ts_spot[i], S_TEMP, (char*) charAddr); //, ((char*) (tempDeviceAddress, 8))); //seem that this is not working as intended
#ifdef SEND_ID
    //8 sorgt dafür, dass alle 16 Stellen übermittelt werden
    send(msgId.setSensor(ts_spot[i]).set(tempDeviceAddress, 8));
#endif
  }
}
void loop() {
  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();
  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  wait(conversionTime);
  // Read temperatures and send them to controller
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getConfig().isMetric ? sensors.getTempCByIndex(i) : sensors.getTempFByIndex(i)) * 10.)) / 10.;
    // Only send data if temperature has changed and no error
#if COMPARE_TEMP == 1
    if (lastTemperature[i] != temperature && temperature != -127.00 && temperature != 85.00) {
#else
    if (temperature != -127.00 && temperature != 85.00) {
#endif
      // Send in the new temperature
      send(msgTemp.setSensor(ts_spot[i]).set(temperature, 1));
      // Save new temperatures for next compare
      lastTemperature[i] = temperature;
    }
  }
  wait(SLEEP_TIME);
}

// Subroutines
//##########################################################
//
void initialiseIdArray() {
  uint8_t knownSensors = 0;
#if ERASE_HASH == 1
  if (numSensors < 1) {
    for (uint8_t i = EEPROM_DEVICE_ADDR_START; i < EEPROM_DEVICE_ADDR_END + 1; i++) {
      saveState(i, 0xFF); //0xFF seems to be better in line with mysensors standards, was 0x00
    }
    Serial.println("EEPROM cleared...");
  }
#endif
  // initialise spot_used array to default false
  for (uint8_t i = 0; i < MAX_ATTACHED_DS18B20; i++) {
    spot_used[i] = false;
  }
  // first loop: filling Address-Array with IDs already stored, and clear used spot array
  for (uint8_t i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    sensors.getAddress(tempDeviceAddress, i);
    // check if we know this sensor
    int8_t sidx = getSensorIndex(tempDeviceAddress);
    if (sidx < 0) {
      ts_spot[i] = 0;
    }
    else {
      // we know this sensor
      ts_spot[i] = sidx + DS_First_Child_ID;
      spot_used[sidx] = true;
      knownSensors++;
    }
  }
#ifdef MY_DEBUG
  Serial.println();
  Serial.print(F("# Stored Devices: "));
  Serial.println(knownSensors);
#endif
  // second loop for filling Address-Array with IDs not stored yet
  for (uint8_t i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    sensors.getAddress(tempDeviceAddress, i);
    // check if we do not know this sensor yet
    if (ts_spot[i] == 0) {
      uint8_t k = getUnusedSpot();
      ts_spot[i] = k + DS_First_Child_ID;
      spot_used[k] = true;
      storeSensorAddr(tempDeviceAddress, k);
    }
  }
// print IDs and EEPROM spaces
#ifdef MY_DEBUG
  for (int i = 0; i < numSensors && i < MAX_ATTACHED_DS18B20; i++) {
    sensors.getAddress(tempDeviceAddress, i);
    Serial.println();
    Serial.print(i);
    Serial.print(F(" index: "));
    Serial.print(ts_spot[i]);
    Serial.print(F(" address: "));
    charAddr = addrToChar(tempDeviceAddress);
    Serial.println(charAddr);
#endif
  }
}

// a simple hash function for 1wire address to reduce id to two bytes
uint16_t simpleAddrHash(DeviceAddress a) {
  return ((a[1] ^ a[2] ^ a[3] ^ a[4] ^ a[5] ^ a[6]) << 8) + a[7];
}

// search for device address hash in eeprom
// return -1 if not found
int8_t getSensorIndex(DeviceAddress a) {
  uint16_t hash = simpleAddrHash(a);
  int8_t idx = -1;
  uint8_t aidx = 0;
  uint8_t ptr = EEPROM_DEVICE_ADDR_START;
  while (ptr < EEPROM_DEVICE_ADDR_END && idx == -1) {
    uint8_t hash1 = loadState(ptr);
    uint8_t hash2 = loadState(ptr + 1);
    if ( hash1 == (uint8_t)(hash >> 8) && hash2 == (uint8_t)(hash & 0x00FF)) {
      // found device index
      idx = aidx;
    }
    aidx++;
    ptr += 2;
  }
  return idx;
}

// search for first unused spot in eeprom
uint8_t getUnusedSpot() {
  int8_t j = 0;
  while (spot_used[j] == true) {
    j++;
  }
  return j;
}

// save address hash in EEPROM under index
void storeSensorAddr(DeviceAddress a, uint8_t index) {
  uint16_t hash = simpleAddrHash(a);
  uint8_t ptr = EEPROM_DEVICE_ADDR_START + index * 2;
  if (ptr < EEPROM_DEVICE_ADDR_END) {
    saveState(ptr,   hash >> 8);
    saveState(ptr + 1, hash & 0x00FF);
  }
#ifdef MY_DEBUG
  Serial.print(F("storeSensorAddr under index: "));
  Serial.println(index);
#endif
}

char* addrToChar(uint8_t* data) {
  String strAddr = String(data[0], HEX); //Chip Version; should be higher than 16
  byte first ;
  int j = 0;
  for (uint8_t i = 1; i < 8; i++) {
    if (data[i] < 16) strAddr = strAddr + 0;
    strAddr = strAddr + String(data[i], HEX);
    strAddr.toUpperCase();
  }
    for (int j = 0; j < 16; j++) {
      charAddr[j] = strAddr[j];
    }
  return charAddr;
}
