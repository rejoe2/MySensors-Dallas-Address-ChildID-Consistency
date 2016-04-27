# MySensors

Enhanced MySensors code to get more consistent Child-IDs when using multiple DS18B20 on one bus.

Problem description:
If you measure different temperatures on the same bus (which is one big advantage of the 1-wire technology), using the standard MySensors Sketches your controller will not necessarily always see the same Child_ID for the same physical measuring point. This especially can happen, if you add or remove chips from the bus and restart your node.

To solve this, one has to store some info about the used physical devices inside the node itself.

First approach to solve this is to build a short hash for each DS18B20 and store this in EEPROM (DallasTemperatureSensor_Stored_ID). This is based on an idea of leodesinger, see original code at https://github.com/leodesigner/mysensors-arduino. Based on the stored info, an array is build an then used to bridge the Dallas-bus-position to the (time-consistent) Child-ID. This offers also the possibility to shift the Child-ID to higher numbers. It seems best, to use the last Child-ID's presented by this node for the temperature values, as the stored hash uses 2 bytes, whereas the mysensors standard logik, e.g. for switches uses 1 byte per Child-ID.

Second approach writes the addresses directly into the sketche's code (Dallas_Addresses_Array_Solution). This method is pretty common, but a little more inconvenient. Most of the code here is based on petewill's http://forum.mysensors.org/topic/2607/video-how-to-monitor-your-refrigerator.
For this, one should know the addresses at first. This is Dallas_Addresses_print_as_Array.ino. 
After putting this information in the Dallas_Addresses_Array_Solution.ino, this could help, if one want's some automatics to be done by the node itself (e.g. a "send a cool down command to my clima, if temperature is greater than".

Have fun!
