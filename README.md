# MySensors

Modificaded MySensors code to get more consistent Child-IDs when using multiple DS18B20 on one bus.

Problem description:
If you measure different temperatures on the same bus (which is one big advantage of the 1-wire technology), using the standard MySensors Sketches your controller will not necessarily always see the same Child_ID for the same physical measuring point. This especially can happen, if you add or remove chips from the bus and restart your node.

To solve this, one has to store some info about the used physical devices inside the node itself.

First solution approach is to build a short hash for each DS18B20 and store this in EEPROM (DallasTemperatureSensor_Stored_ID). This is based on an idea of leodesinger, see original code at https://github.com/leodesigner/mysensors-arduino. The stored info is then used for an array  to bridge the current Dallas-bus-position to the (time-consistent) Child-IDs. This offers also the possibility to shift the Child-IDs to higher numbers. It seems best, to define the last Child-IDs presented by this node for the temperature values, as the stored hash uses 2 bytes, whereas the mysensors standard logic, e.g. for switches uses 1 byte per Child-ID.

Second approach writes the addresses directly into the sketche's code (Dallas_Addresses_Array_Solution). This method is widely used in public available arduino excamples, but a little more inconvenient. Most of the code here is based on petewill's http://forum.mysensors.org/topic/2607/video-how-to-monitor-your-refrigerator.
For this, one should know the addresses in the time of coding. Tool for preparing this is Dallas_Addresses_print_as_Array.ino. 
After putting the addresses in the Dallas_Addresses_Array_Solution.ino, this could help, if one wants some automatics to be done by the node itself (e.g. a "switch an indicator LED on in case if temperatureX is greater than y°C". This surely also requires to know, which sensor will be placed in the future physical installation

Have fun!
