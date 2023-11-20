# MBusino Tutorial deutsch

### nötige Software

"Arduino IDE 2" downloaden und installieren.
https://docs.arduino.cc/software/ide-v2

### Vorbereitungen

Zuerst muss die nötige Ordnerstruktur erstellt werden.
Während der Installation der Arduino IDE wurde ein Ordner namens "Arduino" angelegt, in dem alle Programme liegen werden. 

* In diesem Ordner ".../Arduino" einen Ordner mit dem Namen MBusino erstellen. 
* In den Ordner "MBusino" die Datei MBusino.ino ablegen. Die Datei findet ihr in der Repo unter SRC. 
* Unter https://github.com/Zeppelin500/mbus-payload den Butten "Code" ausklappen und dann Download ZIP drücken.
* Die ZIP unter Arduino/libraries entpacken, so das am Ende die Ordnerstrucktur .../Arduino/libraries/mbus-payload vorhanden ist. Diese Library ist verändert und funktioniert im Gegensatz zur Originalen. Wenn ihr diesen Schritt nicht durchführt, läd er die originale Bibliothek und ihr habt keine Funktion.

### Code anpassen

* Arduino IDE 2 öffnen.
* Im Reiter "Datei" auf "Öffnen" drücken, zur MBusino.ino navigieren und diese öffnen.
* oben im Code stehen hinter #include alle benötigten Bibliotheken. Darum braucht ihr euch aber nicht kümmern, denn der integrierte Library Manager läd diese automatisch nach, falls sie noch fehlen.
* Die Bibliothek credentials.h stellt eine persönliche Bibliothek dar, die automatisch beim compilieren Eure Zugangsdaten in den Code bringt. Das hat den Vorteil, dass man neuere Versionen des Codes einfach nachladen kann, ohne jedes mal die Zugangsdaten wieder neu eintragen zu müssen.

### Option1: credentials.h

In .../Arduino/libraries/ den Ordner Credentials erstellen und dort die Beispieldatei credentials.h mit folgendem Inhalt (Auf die eigenen Zgangsdaten angepasst) ablegen. Beispeildatei ist hier im Tutorial Ordner abgelegt

```
#include "Arduino.h"

#define WLAN_SSID "meine_SSID"
#define WLAN_PASSWORD "mein_Passwort"
#define MQTT_Broker "192.168.1.x"
```
 
Dieser Codeschnipsel macht nichts anders, als überall dort, wo er den Code hinter define, z.B. WLAN_SSID findet, durch "meine_SSID" zu ersetzen.


### Option2 Zugansdaten eintragen

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

### *optional -* Abfrage- und Veröffenlichungsintervalle festlegen

Ein paar Zeilen weiter unten bei der Variableninitialisierung kommen diese 2 Zeilen

```
int publIntervalSensoren = 10000;  // publication interval for sensor values in milliseconds
int MbusInterval = 10000;           // interval for MBus request in milliseconds
```

Hier die Angaben im Milisekunden nach eigenem Wunsch ändern.

### *optional -* Name ändern

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

Die Logik ist folgende, für jeden Wert einen Sensor. state_topic ist das MQTT topic, das ihr im MQTT Eplorer findet.
```
mqtt:
 sensor:
     - unique_id: MBusino_Leistung
      name: "Leistung"
      device_class: power
      state_topic: "MBusino/MBus/4_power"     
      unit_of_measurement: "W"
```



