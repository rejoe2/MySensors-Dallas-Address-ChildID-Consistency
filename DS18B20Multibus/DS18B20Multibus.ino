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
   Enhanced Version also sending the Dallas-ROM-ID, MySensors Version >=2.1.0
   Multibus version!
   */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define COMPARE_TEMP 1 // Send temperature only if changed? 1 = Yes 0 = No
#define MAX_ATTACHED_DS18B20 10 //This is per bus!
#define ONE_WIRE_BUSSES = 3;
unsigned long ONE_WIRE_TIMER = {200000,300000,100000}; // Differen timers for reading different busses between reads (in milliseconds)

OneWire oneWire[3] = {6,4,5}; // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs) on mentionned pins

DallasTemperature sensors[ONE_WIRE_BUSES]; // Pass the oneWire reference to Dallas Temperature.
float lastTemperature[ONE_WIRE_BUSES][MAX_ATTACHED_DS18B20];
unsigned long lastSend[ONE_WIRE_BUSES] = {0};

uint8_t DS_First_Child_ID[ONE_WIRE_BUSES] = {10,20,30}; //First Child-ID to be used by differen Dallas Buses; set this to be higher than other Child-ID's who need EEPROM storage to avoid conflicts
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature.

int numSensors = 0;
bool receivedConfig = false;
bool metric = true;
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address
int resolution = 12; // precision: 12 bits = 0.0625°C, 11 bits = 0.125°C, 10 bits = 0.25°C, 9 bits = 0.5°C
int conversionTime = 750;
// Initialize temperature message
MyMessage msgTemp(0, V_TEMP);
MyMessage msgId(0, V_ID);

char* charAddr = "Check for faults";
#define SEND_ID

void before()
{
  // 12 bits = 750 ms, 11 bits = 375ms, 10 bits = 187.5ms, 9 bits = 93.75ms
  conversionTime = 750 / (1 << (12 - resolution));
  // Startup up the OneWire library
  for (uint8_t i=0; i<ONE_WIRE_BUSES; i++) {
    sensors[i].setOneWire(&oneWire[i]);
    sensors[i].begin();

  // requestTemperatures() will not block current thread
    sensors[i].setWaitForConversion(false);

    // Fetch the number of attached temperature sensors and set resolution
    for (uint8_t j=0; j < sensors[i].getDeviceCount(); j++) {
      sensors[i].setResolution(j, resolution);
    }
  }
  sensors.begin();
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Multibus Temp Sensor", "1.2");

  // Fetch the number of attached temperature sensors
  numSensors = sensors.getDeviceCount();

  // Present all sensors to controller
  for (int i = 0; i < ONE_WIRE_BUSES; i++) {
	for (uint8_t j=0; j < numSensors; j++) {
		sensors[i].getAddress(tempDeviceAddress, j);
    	sensors.getAddress(tempDeviceAddress, j);
		charAddr = addrToChar(tempDeviceAddress);
		present(j + DS_First_Child_ID[i], S_TEMP, charAddr);
#ifdef MY_DEBUG
		Serial.println(charAddr);
#endif
	}  
  }
}

void setup()
{
  for (int i = 0; i < ONE_WIRE_BUSES; i++) {
	numSensors = sensors.getDeviceCount();
	for (uint8_t j=0; j < numSensors; j++) {
		sensors[i].getAddress(tempDeviceAddress, j);
    	charAddr = addrToChar(tempDeviceAddress);
		// 8 will assure a length of 16 of the sent ROM-ID
		send(msgId.setSensor(j + DS_First_Child_ID[i]).set(tempDeviceAddress, 8));
		sensors[i].setResolution(tempDeviceAddress, resolution);
#ifdef MY_DEBUG
		Serial.println(charAddr);
#endif
	}  
  }
  metric = getControllerConfig().isMetric;
}

void loop()
{
  unsigned long currentTime = millis();

  for (int i = 0; i < ONE_WIRE_BUSES; i++) {
	if (currentTime - lastSend[i] > ONE_WIRE_TIMER[i]) {
		// Start measurement
		sensors[i].requestTemperatures();
    }
  }
  
  for (int i = 0; i < ONE_WIRE_BUSES; i++) {
	if (currentTime - lastSend[i]-conversionTime > ONE_WIRE_TIMER[i]) {
		numSensors = sensors.getDeviceCount();
		for (uint8_t j=0; j < numSensors; j++) {
			float temperature = static_cast<float>(static_cast<int>((metric ? sensors[i].getTempCByIndex(j) : sensors[i].getTempFByIndex(j)) * 10.)) / 10.;
#if COMPARE_TEMP == 1
		if (lastTemperature[i][j] != temperature && temperature != -127.00 && temperature != 85.00) {
#else
		if (temperature != -127.00 && temperature != 85.00) {
#endif
			// Send in the new temperature
			send(msgTemp.setSensor(j + DS_First_Child_ID[i]).set(temperature, 1));
#ifdef MY_RADIO_NRF24
			wait(20);
#endif			
			// Save new temperatures for next compare
			lastTemperature[i][j] = temperature;
    }
  }	
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
