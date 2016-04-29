# MySensors

Modificaded MySensors code to get more consistent Child-IDs when using multiple DS18B20 on one bus.

Problem description:
If you measure different temperatures on the same bus (which is one big advantage of the 1-wire technology), using the standard MySensors Sketches your controller will not necessarily always see the same Child_ID for the same physical measuring point. This especially can happen, if you add or remove sensors from the bus and restart your node.

To solve this, one has to store some info about the used physical devices inside the node itself.

First solution approach is to build a short hash for each DS18B20 and store this in EEPROM (DallasTemperatureSensor_Stored_ID). This is based on an idea of leodesinger, see original code at https://github.com/leodesigner/mysensors-arduino. The stored info is then used for an array  to bridge the current Dallas-bus-position to the (time-consistent) Child-IDs. This offers also the possibility to shift the Child-IDs to higher numbers. It seems best, to define the last Child-IDs presented by this node for the temperature values, as the stored hash uses 2 bytes, whereas the mysensors standard logic, e.g. for switches uses 1 byte (uint8_t) per Child-ID.
Additional remarks on this:
- The code has its own interpretation of hardware-changes, so keep this in mind. Most likely it is best, to connect all the sensors you may have connected in the end at least for one time all together to the node; do this asap.
- Don't blame me for not disabling the EEPROM-clearing feature!!!
- Node seems not to react on hot-plugging of hardware. (Just restart the node after (re-) connecting sensors).  
- The EEPROM-info also seems to survive reflashes

Second approach writes the addresses directly into the sketch's code (Dallas_Addresses_Array_Solution). This method is widely used in public available arduino excamples, but a little more inconvenient. Most of the code here is based on petewill's http://forum.mysensors.org/topic/2607/video-how-to-monitor-your-refrigerator. The "trick" used is to notuse the "sensors.getTempC(address)" call instead of "sensors.getTempCByIndex(i)". 

For this, one should know the addresses in the time of coding. So there are two operation modes for this sketch, one for getting the required array definition and the number of recognized devices. After registering the addresses in the Dallas_Addresses_Array_Solution.ino, doing it in this way could help, if one wants some automatics to be done by the node itself (e.g. a "switch an indicator LED on in case if temperatureX is greater than y°C" or similar things used by aquarists or hobbyist brewers. This surely also requires to know, which sensor will be placed where in the future physical installation, but if you get to this kind of problem, you surely will find a way to get this managed also...
Additional remarks on this:
- The address array you got printed is just a proposal; it may be rearranged (eg. in case you want to upgrade with additional sensors, you copy the additional array field to the end of the existent array definition). You just have to keep in mind the fact that the position in the array will determine the sensors ChildID, so also using a ","-field in between or leaving an no-longer-needed HW-address in the array could be a good idea to avoid reconfiguration on your controller.
- The Node will not recognise any sensor not mentioned in the array... So:
  -   Make sure to avoid typos! Don't forget the "," after the last entry of your array.
  -   You have to reflash, if you add hardware
- The node seems to accept (re)connects to the sensors also after having started (hot-plugging)

Some further remarks:
- My personal intention was to get time-consistent info sent to my controller, so I am not sad about both versions requiring a lot more memory than the original MySensors sketch. And: These sketches here are designed to understand and present the principles and therefore print a lot of stuff to the console. Most likely disabling this could improve memory usage a lot. I also suspect the ID sending feature and the presentation of the Dallas-Address as a comment to be not optimal wrt to memory usage. Feel free to delete these parts! 
- Some people are interested in measurement speed. Don't ask me, I am ok with sending info every 3-5 min... My guess: using the address-array method is fast (or even most efficient way of addressing this type of sensor). 
- I got into trouble trying to use one node/arduino-pin for my entire 1-wire network (just temperature, nothing else). This didn't work as expected. My "then" thinking was to blame it 
  -  on the amount of sensors (7-8), 
  -  the additional sensors and internal functionality I put into this specific node (some 80+% of memory used) or 
  -  the cable length (may have been 70m or so). 
With 5 temp's (actively powerd, not parasitic) it seem to work fine now for weeks or even months (array-method). Didn't test it myself, but some people report using a smaller pullup resistor (2k or even less) may lead to better results. Perhaps I will give it a try in case I have to modificate the hardware? (Or place an additional res. (4.7k) somewhere else on the cabeling???)

Have fun!
