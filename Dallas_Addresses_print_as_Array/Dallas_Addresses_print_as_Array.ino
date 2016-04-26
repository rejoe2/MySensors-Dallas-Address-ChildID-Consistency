#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 3 //Pin where Dallas sensor is connected

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature dallasTemp(&oneWire);

// arrays to hold device addresses
DeviceAddress tempAddress[8];

void setup(void)
{
  // start serial port
  Serial.begin(115200);

  // Start up the library
  dallasTemp.begin();

  // show the addresses we found on the bus
  for (uint8_t i = 0; i < dallasTemp.getDeviceCount(); i++) {
    if (!dallasTemp.getAddress(tempAddress[i], i))
    {
      Serial.print("Unable to find address for Device ");
      Serial.println(i);
      Serial.println();
    }
    Serial.print("Device ");
    Serial.print(i);
    Serial.print(" Address: ");
    printAddress(tempAddress[i]);
    Serial.println();
  }
}

void printAddress(DeviceAddress deviceAddress)
{
  Serial.print("{")
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    //if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print("0x");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(", ");
    }
  }
  Serial.println("}")
}


void loop(void)
{

}
