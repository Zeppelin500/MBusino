/*
# MBusino
Ein ESP8266 D1 Mini Shield zur Aufnahme verschiedener Messwerte und deren Versand per MQTT
 

## Code funktioniert nur mit der veränderten MBUSPayload Libary. 

* Jetzt sollten verschiedene M-Bus Slaves ausgelesen werden können.  

https://github.com/Zeppelin500/mbus-payload

## Danke  

* an AllWize für die Libary zum dekodieren und
* an HWHardsoft für die M-Bus Kommunikation


## Lizenz GPL3
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/

#include <EspMQTTClient.h>
#include <OneWire.h>              // Libary für den OneWire Bus
#include <DallasTemperature.h>    //Libary für die Temperatursensoren
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <credentials.h>          // <-- Auskommentieren wenn ihr keine Libary für Eure Zugangsdaten nutzt.


#include <MBUSPayload.h>        // Libary für das decodieren des Mbus
#include "ArduinoJson.h"


#define MBUS_BAUD_RATE 2400
#define MBUS_ADDRESS 0xFE // brodcast
#define MBUS_TIMEOUT 1000 // milliseconds
#define MBUS_DATA_SIZE 510
#define MBUS_GOOD_FRAME true
#define MBUS_BAD_FRAME false

#define ONE_WIRE_BUS1 2   //D4  
#define ONE_WIRE_BUS2 13  //D7  
#define ONE_WIRE_BUS3 12  //D6  
#define ONE_WIRE_BUS4 14  //D5  
#define ONE_WIRE_BUS5 0   //D3  

#define SEALEVELPRESSURE_HPA (1013.25)

EspMQTTClient client(
  WLAN_SSID,          // WLAN-SSID mit Anführungszeichen eintragen "meineSSID"
  WLAN_PASSWORT,     // WLAN Passwort mit Anführungszeichen eintragen "meineSSID"
  MQTT_Broker,        // MQTT Broker server ip mit Anführungszeichen eintragen "192.168.x.x"
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed -- auch Passwort für OTA Update über die Arduino IDE 2
  "MBusino",     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

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

unsigned long loop_start = 0;
unsigned long last_loop = 0;
bool firstrun = true;
int Startadd = 0x13; // Start address for decoding
char jsonstring [4080] = {0};

float OW1 = 0;  // Variablen für die DS18B20 Onewire Sensoren S1
float OW2 = 0;  // S2
float OW3 = 0;  // S3
float OW4 = 0;  // S4
float OW5 = 0;  // S5
float temperatur = 0; // Variablen für den BLE280 am I2C
float druck = 0;
float hoehe = 0;
float feuchte = 0;

bool bmeStatus;

int publIntervalSensoren = 10000; // Veröffentlichungsintervall der Sensoren in Millisekunden
int MbusInterval = 10000; // Inervall für die MBus Abfragen in Millisekunden

unsigned long timerMQTT = 15000;  
unsigned long timerSensorRefresh1 = 0;
unsigned long timerSensorRefresh2 = 0;
unsigned long timerMbus = 0;

void mbus_request_data(byte address);
void mbus_short_frame(byte address, byte C_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);


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
  client.setMaxPacketSize(6000);

// Vorbereitungen für den BME280
  bmeStatus = bme.begin(0x76);  

}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished(){
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
  loop_start = millis();

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

    timerMQTT = millis();  
  }
  


  if ((loop_start-last_loop)>= MbusInterval || firstrun) { // 9800 = ~10 seconds
    last_loop = loop_start; firstrun = false;

    bool mbus_good_frame = false;
    byte mbus_data[MBUS_DATA_SIZE] = { 0 };
    mbus_request_data(MBUS_ADDRESS);
    mbus_good_frame = mbus_get_response(mbus_data, sizeof(mbus_data));

    if (mbus_good_frame) {
      int packet_size = mbus_data[1] + 6; 
      MBUSPayload payload(255);  
      DynamicJsonDocument jsonBuffer(4080);
      JsonArray root = jsonBuffer.createNestedArray();  
      uint8_t fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 
      
      serializeJsonPretty(root, jsonstring);
      client.publish("MBusino/MBus/error", String(payload.getError()));  // kann auskommentiert werden wenn es läuft
      client.publish("MBusino/MBus/jsonstring", String(jsonstring));


      for (uint8_t i=0; i<fields; i++) {
        float value = root[i]["value_scaled"].as<float>();
        uint8_t code = root[i]["code"].as<int>();

        client.publish(String("MBusino/MBus/"+String(i+1)+"_"+payload.getCodeName(code)), String(value,3).c_str());
        client.publish(String("MBusino/MBus/"+String(i+1)+"_"+payload.getCodeName(code))+"_unit", payload.getCodeUnits(code));

        if (i == 3){  // Sensostar Bugfix --> comment it out if you use not a Sensostar
          float flow = root[5]["value_scaled"].as<float>();
          float delta = root[9]["value_scaled"].as<float>();
          float calc_power =  calc_power = delta * flow * 1163;          
          client.publish("MBusino/MBus/4_power_calc", String(calc_power).c_str());           
        }
      } 
    } 
    else {
  //Fehlermeldung
        client.publish(String("MBusino/MBUSerror"), "no_good_telegram");
    }
  
  }
              
}

void mbus_request_data(byte address) {
  mbus_short_frame(address,0x5b);
}

void mbus_short_frame(byte address, byte C_field) {
  byte data[6];

  data[0] = 0x10;
  data[1] = C_field;
  data[2] = address;
  data[3] = data[1]+data[2];
  data[4] = 0x16;
  data[5] = '\0';

  Serial.write((char*)data);
}

bool mbus_get_response(byte *pdata, unsigned char len_pdata) {
  byte bid = 0;             // current byte of response frame
  byte bid_end = 255;       // last byte of frame calculated from length byte sent
  byte bid_checksum = 255;  // checksum byte of frame (next to last)
  byte len = 0;
  byte checksum = 0;
  bool long_frame_found = false;
  bool complete_frame  = false;
  bool frame_error = false;
 
  unsigned long timer_start = millis();
  while (!frame_error && !complete_frame && (millis()-timer_start) < MBUS_TIMEOUT)
  {
    while (Serial.available()) {
      byte received_byte = (byte) Serial.read();

      // Try to skip noise
      if (bid == 0 && received_byte != 0xE5 && received_byte != 0x68) {
        continue;
      }
      
      if (bid > len_pdata) {
        return MBUS_BAD_FRAME;
      }
      pdata[bid] = received_byte;

      // Single Character (ACK)
      if (bid == 0 && received_byte == 0xE5) {
        return MBUS_GOOD_FRAME;
      }
      
      // Long frame start
      if (bid == 0 && received_byte == 0x68) {
        long_frame_found = true;
      }

      if (long_frame_found) {
        // 2nd byte is the frame length
        if (bid == 1) {
          len = received_byte;
          bid_end = len+4+2-1;
          bid_checksum = bid_end-1;
        }
            
        if (bid == 2 && received_byte != len) {                          // 3rd byte is also length, check that its the same as 2nd byte
          frame_error = true;
        }
        if (bid == 3 && received_byte != 0x68) {        ;                // 4th byte is the start byte again
          frame_error = true;
        }
        if (bid > 3 && bid < bid_checksum) checksum += received_byte;    // Increment checksum during data portion of frame
        
        if (bid == bid_checksum && received_byte != checksum) {          // Validate checksum
          frame_error = true;
        }
        if (bid == bid_end && received_byte == 0x16) {                   // Parse frame if still valid
          complete_frame  = true;
        }
      }
      bid++;
    }
  }

  if (complete_frame && !frame_error) {
    return MBUS_GOOD_FRAME;
  } else {
    return MBUS_BAD_FRAME;
  }
}
