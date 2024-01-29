# MBusino Tutorial deutsch

**MBusinoAP** ist jetzt online, AP für AccessPoint mit Setup Portal. Keine Anpassungen am Code nötig. Siehe unten.

Ihr könnt euch auch alle Schritte sparen und den Code online Flashen, siehe ganz unten.

### nötige Software

"Arduino IDE 2" downloaden und installieren.
https://docs.arduino.cc/software/ide-v2

### Vorbereitungen

Zuerst muss die nötige Ordnerstruktur erstellt werden.
Während der Installation der Arduino IDE wurde ein Ordner namens "Arduino" angelegt, in dem alle Programme liegen werden. 

* In diesem Ordner ".../Arduino" einen Ordner mit dem Namen **MBusino** bzw. **MBusino_naked** erstellen. 
* In den Ordner "MBusino" die Datei MBusino.ino ablegen. Die Datei findet ihr in der Repo unter SRC. 
* Die eigene MBusinoLib ist jetzt im Library Manager. Daher benötigt man die folgenden 2 Punkte nicht mehr!
* ~~Unter https://github.com/Zeppelin500/mbus-payload den Butten "Code" ausklappen und dann Download ZIP drücken.~~
* ~~Die ZIP unter Arduino/libraries entpacken, so das am Ende die Ordnerstrucktur .../Arduino/libraries/mbus-payload vorhanden ist. Diese Library ist verändert und funktioniert im Gegensatz zur Originalen. Wenn ihr diesen Schritt nicht durchführt, läd er die originale Bibliothek und ihr habt keine Funktion.~~

### Code anpassen --> entfällt bei MBusinoAP

* Arduino IDE 2 öffnen.
* Im Reiter "Datei" auf "Öffnen" drücken, zur MBusino.ino navigieren und diese öffnen.
* oben im Code stehen hinter #include alle benötigten Bibliotheken. Darum braucht ihr euch aber nicht kümmern, denn der integrierte Library Manager läd diese automatisch nach, falls sie noch fehlen.
* Die Bibliothek credentials.h stellt eine persönliche Bibliothek dar, die automatisch beim compilieren Eure Zugangsdaten in den Code bringt. Das hat den Vorteil, dass man neuere Versionen des Codes einfach nachladen kann, ohne jedes mal die Zugangsdaten wieder neu eintragen zu müssen.

### Option1: credentials.h --> entfällt bei MBusinoAP

In .../Arduino/libraries/ den Ordner Credentials erstellen und dort die Beispieldatei credentials.h mit folgendem Inhalt (Auf die eigenen Zgangsdaten angepasst) ablegen. Beispeildatei ist hier im Tutorial Ordner abgelegt

```
#include "Arduino.h"

#define WLAN_SSID "meine_SSID"
#define WLAN_PASSWORD "mein_Passwort"
#define MQTT_Broker "192.168.1.x"
```
 
Dieser Codeschnipsel macht nichts anders, als überall dort, wo er den Code hinter define, z.B. WLAN_SSID findet, durch "meine_SSID" zu ersetzen.


### Option2 Zugansdaten eintragen --> entfällt bei MBusinoAP

relativ weit oben im MBusino Code kommt dieser Block:
```
EspMQTTClient client(
  WLAN_SSID,       // WLAN-SSID in quote marks "mySSID"
  WLAN_PASSWORD,   // WLAN Password in quote marks "myPassword"
  MQTT_Broker,     // MQTT Broker IP address in quote marks "192.168.x.x"
  "MQTTUsername",  // Can be omitted if not needed
  "MQTTPassword",  // Can be omitted if not needed -- also password for OTA Update with Arduino IDE 2
  MBUSINO_NAME,    // Client name that uniquely identify your device
  1883             // The MQTT port, default to 1883. this line can be omitted
);
```

Diese 3 Angaben müsst Ihr durch die eigenen Daten ersetzen. Die Daten müssen in Anführungszeichen stehen.

```
  WLAN_SSID,       // WLAN-SSID in quote marks "mySSID"
  WLAN_PASSWORD,   // WLAN Password in quote marks "myPassword"
  MQTT_Broker,     // MQTT Broker IP address in quote marks "192.168.x.x"
```
Das heist, aus WLAN_SSID wird "meine_SSID" usw.

Auf keinen Fall

```
  MBUSINO_NAME,    // Client name that uniquely identify your device
```
ändern. hierzu weiter unten mehr.

Jetzt muss noch die Einbindung der credentials.h auskomentiert werden.

aus 
```
#include <credentials.h>  // <-- comment it out if you use no library for WLAN access data.
```
wird
```
//#include <credentials.h>  // <-- comment it out if you use no library for WLAN access data.
```
Zwei "/" bedeuten in der Programiersprache C, dass die nachfolgenen Zeichen in der selben Zeile nicht kompiliert werden.

### *optional -* Abfrage- und Veröffenlichungsintervalle festlegen --> entfällt bei MBusinoAP

Ein paar Zeilen weiter unten bei der Variableninitialisierung kommen diese 2 Zeilen

```
int publIntervalSensoren = 10000;  // publication interval for sensor values in milliseconds
int MbusInterval = 10000;           // interval for MBus request in milliseconds
```

Hier die Angaben im Milisekunden nach eigenem Wunsch ändern.

### *optional -* Name ändern --> entfällt bei MBusinoAP

Ganz oben der erste **define** ist die Festlegung des Namens.

```
#define MBUSINO_NAME "MBusino" // If you have more MBusinos, rename it inside quote marks, or it cause some network and MQTT problems. Also you cant reach your MBusino from Arduino IDE
```

Das funktioniert genau wie in der credentials.h .  Überall wo der Compiler MBUSINO_NAME findet, wird er es durch "MBusino" ersetzen.
Wenn ihr mehrere MBusinos nutzt, muss das unbedingt geändert werden, sonst gibt es Netzwerkprobleme und Beide sind nicht mehr erreichbar.

### Kompilieren und flashen

In der IDE unter dem Reiter "Werkzeuge", "Board" --> "Boardverwaltung" nach ESP8266 suchen und installieren. Danach unter Board das **Lolin(Wemos) D1 R2 & mini** auswählen.
Steckt den D1 Mini an USB an und wählt unter "Werkzeuge" "Port" den richtigen USB Anschluss.

Links über dem Code den Butten "Überprüfen", der als Haken abgebildet ist drücken. Lief das ohne Fehler durch, den Butten rechts daneben mit dem Pfeil drücken.
Das war es. 

Ab jetzt funktioniert auch das OTA update. Hierfür müsst ihr nur den Port auf einen Port mit WLAN Symbol und angezeigter IP Adresse ändern.
Das Passwort ist **MQTTPassword**


### MQTT und HomeAssistant

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

## MBusino AP

* flashen siehe oben. Keinerlei anpassungen nötig.
* MBusinoAP stellt ein WLan Setup Portal zur Verfügung.
* Mit der SSID "MBusino Setup Portal" (ohne Passwort) verbinden und auf IP: 192.168.4.1 gehen.
* Daten eingeben, Speichern, reseten

Wenn der MbusinoAP beim hochfahren kein bekanntes Netzwerk findet, stellt er für 5 Minuten den AP bereit, danach resettet er selbstständig und sucht.
Im Wlan findet ihr das Portal dauerhaft. IP müsst ihr in der Fritzbox o.ä. suchen.

### Hinweise zur Dateneingabe:

* Bei MQTT Broker kann eine IP oder eine Adresse eingegeben weren. z.B. **192.168.1.7** oder **test.mosquitto.org** kein https:// oder mqtt:// nur die Adresse
* Beim Speichern werden immer alle Felder auf einmal gespeichert, die Daten liegen dann im EEPROM bzw Flash und sind dauerhaft verfügbar. Werden Felder leer gelassen, sind nach dem speichern vormals gespeicherte Inhalte gelöscht.
* Bei mehreren Geräten im Netzwerk unbedingt verschiedene Namen vergeben, sonst sind sie nicht mehr erreichbar. 

## You can flash the MBusino with https://web.esphome.io

1. Download the Bin from https://github.com/Zeppelin500/MBusino/blob/main/SRC/MBusinoAP/build/esp8266.esp8266.d1_mini/MBusinoAP.ino.bin

2. open https://web.esphome.io with the Chrome-Browser

3. Connect your 8266er with the COM-Port XX

4. Click on "INSTALL" not "PREPARE FOR FIRST USER
![Step4](https://github.com/Zeppelin500/MBusino/assets/32666091/7ff901e5-4994-49f9-a2c1-f914df4c7fb3)

5. Click on "Datei auswählen" and chose MBusinoAP.ino.bin
![Step5](https://github.com/Zeppelin500/MBusino/assets/32666091/221017c4-1a4b-4e81-8c4e-e0d9ce51a9d1)

6. Click on "INSTALL"
![Step6](https://github.com/Zeppelin500/MBusino/assets/32666091/b70db0ed-884b-4ca2-9f67-002f8dbabd9b)

7. Wait for finishing the Flash-Process
![Step7](https://github.com/Zeppelin500/MBusino/assets/32666091/5b327ce6-a832-4b6d-8381-7ecec4572459)

8. Flashing is completed successfully 
![Step8](https://github.com/Zeppelin500/MBusino/assets/32666091/782f9d0a-45da-4c94-b262-aa562115c390)

9. Search the MBusino Wlan Access-Point "MBusino Setup Portal" with your Smartphone

10. Configure your Mqtt-Broker and your WLAN


