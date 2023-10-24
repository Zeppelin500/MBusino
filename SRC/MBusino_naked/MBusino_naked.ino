/*
# MBusino_naked

MBusino ohne weitere Sensorik, M-Bus pur.
Überflüssige Bauteile der Leiterplatte können weggelassen werden

## Anleitung

* Die .ino Datei muss in einen Ordner mit dem selben Namen
* Die 2 Dateien .h und .cpp müssen entweder in einen gemeinsamen Ordner mit dem selben namen in ....arduino/libaries/ abgelegt werden oder direkt in den Ordner mit der .ino Datei.
* SSID, WLANpasswort und MQTT-Broker eintragen wie im Code angegeben
* ggfls. Abfrageintervall (MbusInterval) anpassen
* erstes flashen erfolgt über USB, danach ist OTA flashen möglich



## Lizenz
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/

#include <EspMQTTClient.h>    
#include <Wire.h>

#include <credentials.h>          // <-- Auskommentieren wenn ihr keine Libary für Eure Zugangsdaten nutzt.

#include "sensostar.h"            // Libary für den MBus

int WMZ_absolute_energy_kwh = 0; // Variablen für den Wärmemengenzähler
int WMZ_power_w = 0;
int WMZ_flow_temp_k = 0;
int WMZ_return_temp_k = 0;
int WMZ_delta_temp_mk = 0;
int WMZ_error_code = 0;
int WMZ_flow_rate_lph = 0;


int MbusInterval = 120000; // Inervall für die MBus Abfragen in Millisekunden --> ohne Netzteil mindestens 120Sekunden (120000)
unsigned long timerMbus = 0;

EspMQTTClient client(
  WLAN_SSID,          // WLAN-SSID mit Anführungszeichen eintragen "meineSSID"
  WLAN_PASSWORT,     // WLAN Passwort mit Anführungszeichen eintragen "meineSSID"
  MQTT_Broker,        // MQTT Broker server ip mit Anführungszeichen eintragen "192.168.x.x"
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed -- auch Passwort für OTA Update über die Arduino IDE 2
  "MBusino",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

static size_t hm_write(uint8_t c) {
  return Serial.write(c);
}
Sensostar heat_meter(hm_write, millis);

void setup(){

  Serial.begin(2400, SERIAL_8E1);
  
  // Optional functionalities of EspMQTTClient
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage("MBusino/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // sendet eine Nachricht wenn mit MQTT Broker verbunden.
  client.publish("MBusino/start", "bin hoch gefahren, WLAN und MQTT seht "); 
}

void loop()
{
  client.loop(); //MQTT Funktion 
  
  if (millis() > timerMbus + MbusInterval) {
    heat_meter.request();
    timerMbus = millis();
  }

  while(Serial.available()) {
    char c = Serial.read();
    struct sensostar_data result;
    uint8_t res;
    res = heat_meter.process(c, &result);
    if (res) {
      /* Ergebnis liegt vor in result */
      WMZ_absolute_energy_kwh = result.total_heat_energy;
      WMZ_power_w = result.power;
      WMZ_flow_temp_k = result.flow_temperature;
      WMZ_return_temp_k = result.return_temperature;
      WMZ_delta_temp_mk = result.flow_return_difference_temperature * 10;
      WMZ_error_code = result.error;
      WMZ_flow_rate_lph = result.flow_speed;
      client.publish("MBusino/WMZ/eng", String(WMZ_absolute_energy_kwh).c_str()); 
      client.publish("MBusino/WMZ/err", String(WMZ_error_code).c_str()); 
      client.publish("MBusino/WMZ/pwr", String(WMZ_power_w).c_str()); 
      client.publish("MBusino/WMZ/tvl", String(WMZ_flow_temp_k).c_str()); 
      client.publish("MBusino/WMZ/trl", String(WMZ_return_temp_k).c_str()); 
      client.publish("MBusino/WMZ/spr", String(WMZ_delta_temp_mk).c_str()); 
      client.publish("MBusino/WMZ/dfl", String(WMZ_flow_rate_lph).c_str());
    }
  }
}
