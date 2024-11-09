# MBusino Tutorial deutsch

**Bei der Edition Z für den Zihatec M-Bus Master ist ein ESP32 S2 mini verbaut! Daher die unten stehenden Anweisungen zum flashen des ESP8266 D1 mini überspringen und diese [Anweisung](https://github.com/Zeppelin500/MBusino/blob/main/tutorial/Flashing_ESP32S2_Mini/README.md) anstatt dessen nutzen. Dann bei Punkt 9 weiter machen**

## für das flashen des MBusino ist keine Software nötig. Wir nutzen https://web.esphome.io

1. Downloaden des .bin Files von https://github.com/Zeppelin500/MBusino/blob/main/src/MBusino/build/esp8266.esp8266.d1_mini/MBusino.ino.bin

2. öffne https://web.esphome.io mit z.B. dem Chrome-Browser (geht nicht am Handy)

3. Verbinde den ESP8266 D1 mini per USB

4. Click auf **INSTALL** nicht "PREPARE FOR FIRST USER
![Step4](https://github.com/Zeppelin500/MBusino/assets/32666091/7ff901e5-4994-49f9-a2c1-f914df4c7fb3)

5. Click auf "Datei auswählen" and chose MBusino.ino.bin
![Step5](https://github.com/Zeppelin500/MBusino/assets/32666091/221017c4-1a4b-4e81-8c4e-e0d9ce51a9d1)

6. Click auf "INSTALL"
![Step6](https://github.com/Zeppelin500/MBusino/assets/32666091/b70db0ed-884b-4ca2-9f67-002f8dbabd9b)

7. Warte bis der Flash-Prozess beendet ist
![Step7](https://github.com/Zeppelin500/MBusino/assets/32666091/5b327ce6-a832-4b6d-8381-7ecec4572459)

8. Flashing is completed successfully 
![Step8](https://github.com/Zeppelin500/MBusino/assets/32666091/782f9d0a-45da-4c94-b262-aa562115c390)

9. Suche und verbinde dich mit dem MBusino Wlan Access-Point **MBusino Setup Portal** mit z.B. dem Handy

10. Gib deine Daten ein und speichere.

![SetupPortal](https://github.com/Zeppelin500/MBusino/blob/main/pictures/MBusino_Setup_Portal.jpg)

* Bei mehreren MBusino im Netz ist der Name zu ändern, sonst kommt es zu Netzwerkproblemen.
* Das Minimum bei den Intervallen dürte bei den Sensoren bei 2 Sec. liegen, bei M-Bus 4 sec. 
* Bei MQTT Broker kann eine IP oder eine Adresse eingegeben weren. z.B. **192.168.1.7** oder **test.mosquitto.org** kein https:// oder mqtt:// nur die Adresse

Wenn der MbusinoAP beim hochfahren kein bekanntes Netzwerk findet, stellt er für 5 Minuten den AP bereit, danach resettet er selbstständig und sucht erneut ein bekanntes Netzwerk.
Im Wlan findet ihr das Portal dauerhaft. IP müsst ihr in der Fritzbox o.ä. suchen.

Falls ihr den Mbusino nicht im eigenen Netzwerk findet, sind die WLAN Zugangsdaten falsch eingegeben. Dann sucht wieder nach dem Acesspoint.

## MQTT und HomeAssistant

### Autodiscover

Von Version 0.9.0 an unterstützt MBusino Home Assistant Autodiscover. Ihr braucht nur die MQTT Integration. Dann wird MBusino automatisch mit allen Werten als Gerät gefunden.
Die dritte MQTT-Nachricht ist eine Autodiscover Nachricht. Danach jede 256te.

### per Hand

Über die Software **MQTT Explorer** am PC oder **MyMQTT** auf Android könnt ihr jetzt sehen was gesendet wird. Ich empfehehle den MQTT Explorer.
Je nach M-Bus Gerät sehen die Nachrichten unterschiedlich aus. Im Anhang findet ihr eine Beispielauszug der Config für HomeAssistant mit einem Sensostar U.
Je nach dem was ihr empfangt, muss die Datei angepasst werden.

Die Logik ist folgende, für jeden Wert einen Sensor. **state_topic** ist das MQTT topic, das ihr im MQTT Explorer findet.
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

Über das ESPHome Webtool werden alle eingegebenen Daten gelöscht. Wenn ihr den MBusino auf eine neuere oder ander Version updaten wollt, gebt ihr im Browser **[IP_des_MBusino]/update** ein und flasht darüber das neue .bin File. Alle Daten bleiben erhalten.

![Update](https://github.com/Zeppelin500/MBusino/blob/main/pictures/update.png)





