/*
# MBusino
Ein ESP8266 D1 Mini Shield zur Aufnahme verschiedener Messwerte und deren Versand per MQTT

- M-Bus z.B. Wärmemengenzähler 
- OneWire 5x z.B. DS18B20, Temperatur
- I²C z.B. BME280, Temperatur, rel. Luftfeuchte, Luftdruck

## Hardware
Das Shield ist ein mit Fritzing konstruiertes Board.
Der M-Bus wird über einen aufsteckbaren M-Bus Master bereitgestellt.
https://de.aliexpress.com/item/33008746192.html --> der Master!

Ausser der USB Spannungsversorgung des D1 Mini ist kein weiteres Netzteil nötig.

## known issues
- Flashen über USB ist nur möglich, wenn der MBus Master nicht aufgesteckt ist. OTA Update geht aber.

- Das Projekt befindet sich im Aufbau. Der Code ist funktionsfähig. M-BUS ist bis jetzt nur mit dem Sensostar U implementiert.

## Danke
Der M-Bus Teil ist ein Extrakt aus https://github.com/NerdyProjects/HousebusNode_Heatpump
Danke an User NerdyProjects 


## Lizenz
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/

#include "EspMQTTClient.h"
#include <OneWire.h>              // Libary für den OneWire Bus
#include <DallasTemperature.h>    //Libary für die Temperatursensoren
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <sensostar.h>            // Libary für den MBus
#include <credentials.h>          // <-- Auskommentieren wenn ihr keine Libary für Eure Zugangsdaten nutzt.




#define SEALEVELPRESSURE_HPA (1013.25)

// OneWire data wire is plugged into GPIO xx on the Arduino
// From each data wire one 4K7Ohm Resistor to VCC

#define ONE_WIRE_BUS1 2   //D4  
#define ONE_WIRE_BUS2 13  //D7  
#define ONE_WIRE_BUS3 12  //D6  
#define ONE_WIRE_BUS4 14  //D5  
#define ONE_WIRE_BUS5 0   //D3  

Adafruit_BME280 bme; // I2C

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire1(ONE_WIRE_BUS1);
OneWire oneWire2(ONE_WIRE_BUS2);
OneWire oneWire3(ONE_WIRE_BUS3);
OneWire oneWire4(ONE_WIRE_BUS4);
OneWire oneWire5(ONE_WIRE_BUS5);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);
DallasTemperature sensor4(&oneWire4);
DallasTemperature sensor5(&oneWire5);

float OW1 = 0;  // Variablen für die DS18B20 Onewire Sensoren S1
float OW2 = 0;  // S2
float OW3 = 0;  // S3
float OW4 = 0;  // S4
float OW5 = 0;  // S5
float temperatur = 0; // Variablen für den BLE280 am I2C
float druck = 0;
float hoehe = 0;
float feuchte = 0;
int WMZ_absolute_energy_kwh = 0; // Variablen für den Wärmemengenzähler
int WMZ_power_w = 0;
int WMZ_flow_temp_k = 0;
int WMZ_return_temp_k = 0;
int WMZ_delta_temp_mk = 0 * 10;
int WMZ_error_code = 0;
int WMZ_flow_rate_lph = 0;

bool bmeStatus;

int publIntervalSensoren = 1000; // Veröffentlichungsintervall der Sensoren in Millisekunden
int MbusInterval = 10000; // Inervall für die MBus Abfragen in Millisekunden

EspMQTTClient client(
  WLAN_SSID,          // WLAN-SSID mit Anführungszeichen eintragen "meineSSID"
  WLAN_PASSWORT,     // WLAN Passwort mit Anführungszeichen eintragen "meineSSID"
  MQTT_Broker,        // MQTT Broker server ip mit Anführungszeichen eintragen "192.168.x.x"
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed -- auch Passwort für OTA Update über die Arduino IDE 2
  "MBusino",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

unsigned long timerMQTT = 15000;  
unsigned long timerSensorRefresh1 = 0;
unsigned long timerSensorRefresh2 = 0;
unsigned long timerMbus = 0;

static size_t hm_write(uint8_t c) {
    return Serial.write(c);
  }
Sensostar heat_meter(hm_write, millis);

void setup()
{
  Serial.begin(2400, SERIAL_8E1);
  

  // OneWire vorbereiten 
  sensor1.setWaitForConversion(false);  // makes it async
  sensor2.setWaitForConversion(false);  // makes it async
  sensor3.setWaitForConversion(false);  // makes it async
  sensor4.setWaitForConversion(false);  // makes it async
  sensor5.setWaitForConversion(false);  // makes it async

    // Start up the library
  sensor1.begin();
  sensor2.begin();
  sensor3.begin();
  sensor4.begin();
  sensor5.begin();

  // Optional functionalities of EspMQTTClient
  client.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage("MBusino/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true

// Vorbereitungen für den BME280
  bmeStatus = bme.begin(0x76);  

  if (!bmeStatus) {
    client.publish("MBusino/start", "bme nicht gefunden ");
  }

}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // sendet eine Nachricht wenn mit MQTT Broker verbunden.
  client.publish("MBusino/start", "bin hoch gefahren, WLAN und MQTT seht "); 
}

void sensorRefresh1(){  //stößt aktuallisierte Sensorwerte aus den OneWire Sensoren an
    sensor1.requestTemperatures(); // Send the command to get temperatures
    sensor2.requestTemperatures(); 
    sensor3.requestTemperatures(); 
    sensor4.requestTemperatures(); 
    sensor5.requestTemperatures(); 
}
void sensorRefresh2(){  //holt die aktuallisierten Sensorwerte aus den OneWire Sensoren
    OW1 = sensor1.getTempCByIndex(0);
    OW2 = sensor2.getTempCByIndex(0);
    OW3 = sensor3.getTempCByIndex(0);
    OW4 = sensor4.getTempCByIndex(0);    
    OW5 = sensor5.getTempCByIndex(0);  

    temperatur = bme.readTemperature();
    druck = bme.readPressure()/ 100.0F;
    hoehe = bme.readAltitude(SEALEVELPRESSURE_HPA);
    feuchte = bme.readHumidity();     
            
}



void loop()
{
  client.loop(); //MQTT Funktion 

  if((millis() - timerSensorRefresh1) > 200){ // springt in die Funktion zum anstoßen der aktuallisierung der Sensorwerte
    sensorRefresh1();
    timerSensorRefresh1 = (millis() - 1000);
  }
  if((millis() - timerSensorRefresh2) > 1000){ // springt in die Funktion zum holen der neuen Sensorwerte
    sensorRefresh2();
    timerSensorRefresh1 = millis();
    timerSensorRefresh2 = millis();
  }  

  if(millis() > (timerMQTT + publIntervalSensoren)){ //jede Sekunde folgende MQTT Nachrichten senden
//    client.publish("MBusino/OneWire/S1", String(OW1).c_str()); // nicht angeschlossene Sensoren auskommentiert 
//    client.publish("MBusino/OneWire/S2", String(OW2).c_str()); 
    client.publish("MBusino/OneWire/S3", String(OW3).c_str()); 
//    client.publish("MBusino/OneWire/S4", String(OW4).c_str()); 
//    client.publish("MBusino/OneWire/S5", String(OW5).c_str()); 
    client.publish("MBusino/bme/Temperatur", String(temperatur).c_str()); 
    client.publish("MBusino/bme/Druck", String(druck).c_str()); 
    client.publish("MBusino/bme/Hoehe", String(hoehe).c_str()); 
    client.publish("MBusino/bme/Feuchte", String(feuchte).c_str()); 
    client.publish("MBusino/WMZ/eng", String(WMZ_absolute_energy_kwh).c_str()); 
    client.publish("MBusino/WMZ/err", String(WMZ_error_code).c_str()); 
    client.publish("MBusino/WMZ/pwr", String(WMZ_power_w).c_str()); 
    client.publish("MBusino/WMZ/tvl", String(WMZ_flow_temp_k).c_str()); 
    client.publish("MBusino/WMZ/trl", String(WMZ_return_temp_k).c_str()); 
    client.publish("MBusino/WMZ/spr", String(WMZ_delta_temp_mk).c_str()); 
    client.publish("MBusino/WMZ/dfl", String(WMZ_flow_rate_lph).c_str()); 

    timerMQTT = millis();  
  }
  
  
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
    }
  }
}
