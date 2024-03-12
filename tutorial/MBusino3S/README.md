# MBusino3S Tutorial, 3S for three Slaves

MBusino3S work on **ESP8266 D1 Mini** or **ESP32 S2 Mini**. For both boards are .bin files provided.

https://github.com/Zeppelin500/MBusino/blob/main/src/MBusino3S/build

The Pinout configuration of the ESP32 S2 mini is exactly the same like D1 mini. Equip only the two outside pin headers. 

**Do all steps of the MBusino tutorial (with the MBusino 3S files) and additional the following steps.**
**!Flashing of ESP32 S2 mini is different to the ESP8266 D1 mini, use this [tutorial](https://github.com/Zeppelin500/MBusino/blob/main/tutorial/Flashing_ESP32S2_Mini/README.md)**

## configure MBusino3S

* Number of connected M-Bus Slaves. The Master can handle a maximum of 3 Slaves.
* If you use only 1 slave, set Address 1 to 254. 254 stands for Broadcast and the real address of the slave does no matter.
* If you use more then one slave, you have to set the primary address of the slaves. Possible addresses are 0-250. The connected slaves addresses has to be different! 
* Note, that the M-Bus interval is the time between the M-Bus requests. So if you use 3 slaves and set the interval to 10 seconds. Every single master will answer in a 30 second interval. Minimum is 4 or 5 seconds before the bus were stumbling.
* In delivery condition, the address is 0.

<img src="https://github.com/Zeppelin500/MBusino/blob/main/pictures/3Sconfig.png" width="400">

## Change the primary addresses of your slaves

* go to **[MBusino_IP]/setaddress**
* only one slave can be connected during setting addresses!
* Choose an address at the drop down and save. 
* the slave should now have the new address 

<img src="https://github.com/Zeppelin500/MBusino/blob/main/pictures/setAddress.png" width="400">

## Output

The MQTT topic output of the MBusino3S is different to the MBusino!
MBus Address is now part of the MQTT topic. So you have to change your Home Assistant config.

MBusino MBus topic:
**MBusino/MBus/6_volume_flow**

MBusino3S MBus topic::
**MBusino/MBus/SlaveAddress3/6_volume_flow**

### Autodiscover

from version 0.9.1, MBusino3S supports Home Assistant autodiscover. You need only the MQTT integration and MBusino will be find as device with all records.
The third record message is a autodiscover message and every 256th recurring.

