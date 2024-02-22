# MBusino Tutorial deutsch



## You can flash the MBusino with https://web.esphome.io

1. Download the Bin from https://github.com/Zeppelin500/MBusino/blob/main/SRC/MBusino/build/esp8266.esp8266.d1_mini/MBusino.ino.bin

2. open https://web.esphome.io with the Chrome-Browser

3. Connect your 8266er with the COM-Port XX

4. Click on "INSTALL" not "PREPARE FOR FIRST USER
![Step4](https://github.com/Zeppelin500/MBusino/assets/32666091/7ff901e5-4994-49f9-a2c1-f914df4c7fb3)

5. Click on "Datei auswählen" and chose MBusino.ino.bin
![Step5](https://github.com/Zeppelin500/MBusino/assets/32666091/221017c4-1a4b-4e81-8c4e-e0d9ce51a9d1)

6. Click on "INSTALL"
![Step6](https://github.com/Zeppelin500/MBusino/assets/32666091/b70db0ed-884b-4ca2-9f67-002f8dbabd9b)

7. Wait for finishing the Flash-Process
![Step7](https://github.com/Zeppelin500/MBusino/assets/32666091/5b327ce6-a832-4b6d-8381-7ecec4572459)

8. Flashing is completed successfully 
![Step8](https://github.com/Zeppelin500/MBusino/assets/32666091/782f9d0a-45da-4c94-b262-aa562115c390)

9. Search the MBusino Wlan Access-Point "MBusino Setup Portal" with your Smartphone

10. Configure your Mqtt-Broker and your WLAN

![SetupPortal](https://github.com/Zeppelin500/MBusino/blob/main/pictures/MBusino_Setup_Portal.jpg)

* With multible MBusino in network, you have to change the name or it caouse in network problems .
* Minimum for intervalls is Sensors 2 sec., M-Bus 4 sec. 
* At MQTT Broker you can set an IP or a addresse- E.g. **192.168.1.7** oder **test.mosquitto.org** no https:// oder mqtt:// only addresse


If MBusino do not find a known network, Accesspoint will accessable for 5 minutes. after this periode he restart and search again.
You will find the IP in your router.


## MQTT und HomeAssistant

With the Software **MQTT Explorer** at PC or **MyMQTT** at your mobile you will see the mqtt messages.
in attachement you will find a HA Config file as example. But you have to change something to your MQTT messages.

Every value has its own sensor. **state_topic** is the MQTT topic, depends on the topics at MQTT Explorer.
```
mqtt:
 sensor:
     - unique_id: MBusino_Leistung
      name: "Leistung"
      device_class: power
      state_topic: "MBusino/MBus/4_power"     
      unit_of_measurement: "W"
```

## MBusino updaten

With the ESPHome Webtool, you will loose al your datas during update. Use instead  **[IP_of_MBusino]/update** and all datas will retain.

![Update](https://github.com/Zeppelin500/MBusino/blob/main/pictures/update.png)


