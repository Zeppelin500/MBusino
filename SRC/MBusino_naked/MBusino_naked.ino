/*
# MBusino_naked --> only M-Bus 

21 October 2023: new code based at MBusino with payload library. You find the old "Sensostar only" code in archive.

documentation see MBusino

## Licence
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/

#include <EspMQTTClient.h>

#include <credentials.h>  // <-- comment it out if you use no library for WLAN access data.

#include <MBusinoLib.h>  // Library for decode M-Bus
#include "ArduinoJson.h"
#include <EEPROM.h>

#define MBUSINO_NAME "MBusino" // If you have more MBusinos, rename it inside quote marks, or it cause some network and MQTT problems. Also you cant reach your MBusino from Arduino IDE

#define MBUS_BAUD_RATE 2400
#define MBUS_ADDRESS 0xFE  // brodcast
#define MBUS_TIMEOUT 1000  // milliseconds
#define MBUS_DATA_SIZE 510
#define MBUS_GOOD_FRAME true
#define MBUS_BAD_FRAME false


EspMQTTClient client(
  WLAN_SSID,       // WLAN-SSID in quote marks "mySSID"
  WLAN_PASSWORD,   // WLAN Password in quote marks "myPassword"
  MQTT_Broker,     // MQTT Broker IP address in quote marks "192.168.x.x"
  "MQTTUsername",  // Can be omitted if not needed
  "MQTTPassword",  // Can be omitted if not needed -- also password for OTA Update with Arduino IDE 2
  MBUSINO_NAME,    // Client name that uniquely identify your device
  1883             // The MQTT port, default to 1883. this line can be omitted
);


unsigned long loop_start = 0;
unsigned long last_loop = 0;
bool firstrun = true;
int Startadd = 0x13;  // Start address for decoding
char jsonstring[6144] = { 0 };

int MbusInterval = 5000;           // interval for MBus request in milliseconds

unsigned long timerMbus = 0;

void mbus_request_data(byte address);
void mbus_short_frame(byte address, byte C_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);
void calibrationAverage();
void calibrationSensor(uint8_t sensor);
void calibrationValue(float value);
void calibrationBME();
void calibrationSet0();

void setup() {
  Serial.begin(2400, SERIAL_8E1);

  // Optional functionalities of EspMQTTClient
  client.enableHTTPWebUpdater();                                           // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA();                                                      // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage(MBUSINO_NAME"/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
  client.setMaxPacketSize(6000);

}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished() {  // send a message to MQTT broker if connected.
  client.publish(MBUSINO_NAME"/start", "bin hoch gefahren, WLAN und MQTT seht ");  
}

void loop() {
  client.loop();  //MQTT Funktion
  loop_start = millis();


  if ((loop_start-last_loop)>= MbusInterval || firstrun) { 
    last_loop = loop_start; firstrun = false;

    bool mbus_good_frame = false;
    byte mbus_data[MBUS_DATA_SIZE] = { 0 };
    mbus_request_data(MBUS_ADDRESS);
    mbus_good_frame = mbus_get_response(mbus_data, sizeof(mbus_data));
    /*
    //------------------ only for debug, you will recieve the whole M-Bus telegram bytewise in HEX for analysis -----------------
    for(uint8_t i = 0; i <= mbus_data[1]+1; i++){                                                             //|
      char buffer[3];                                                                                         //|
      sprintf(buffer,"%02X",mbus_data[i]);                                                                    //|
      client.publish(String(MBUSINO_NAME"/debug/telegram_byte_"+String(i)), String(buffer).c_str());          //|
    }                                                                                                         //|
    //--------------------------------------------------------------------------------------------------------------------------    
    */
    if (mbus_good_frame) {
      int packet_size = mbus_data[1] + 6; 
      MBusinoLib payload(255);  
      DynamicJsonDocument jsonBuffer(4080);
      JsonArray root = jsonBuffer.createNestedArray();  
      uint8_t fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 
      
      serializeJsonPretty(root, jsonstring);
      client.publish(MBUSINO_NAME"/MBus/error", String(payload.getError()));  // kann auskommentiert werden wenn es läuft
      client.publish(MBUSINO_NAME"/MBus/jsonstring", String(jsonstring));


      for (uint8_t i=0; i<fields; i++) {
        uint8_t code = root[i]["code"].as<int>();
        const char* name = root[i]["name"];
        const char* units = root[i]["units"];           
        double value = root[i]["value_scaled"].as<double>(); 
        const char* valueString = root[i]["value_string"];            

        //two messages per value, values comes as number or as ASCII string or both
        client.publish(String(MBUSINO_NAME"/MBus/"+String(i+1)+"_vs_"+String(name)), valueString); // send the value if a ascii value is aviable (variable length)
        client.publish(String(MBUSINO_NAME"/MBus/"+String(i+1)+"_"+String(name)), String(value,3).c_str()); // send the value if a real value is aviable (standard)
        client.publish(String(MBUSINO_NAME"/MBus/"+String(i+1)+"_"+String(name)+"_unit"), units);
        //or only one message
        //client.publish(String(MBUSINO_NAME"/MBus/"+String(i+1)+"_"+String(name)+"_in_"+String(units)), String(value,3).c_str());


        if (i == 3){  // Sensostar Bugfix --> comment it out if you use not a Sensostar
          float flow = root[5]["value_scaled"].as<float>();
          float delta = root[9]["value_scaled"].as<float>();
          float calc_power = delta * flow * 1163;          
          client.publish(MBUSINO_NAME"/MBus/4_power_calc", String(calc_power).c_str());           
        }  
      
      }
  
    } 
    else {
  //Fehlermeldung
        client.publish(String(MBUSINO_NAME"/MBUSerror"), "no_good_telegram");
    }
  }
}

void mbus_request_data(byte address) {
  mbus_short_frame(address, 0x5b);
}

void mbus_short_frame(byte address, byte C_field) {
  byte data[6];

  data[0] = 0x10;
  data[1] = C_field;
  data[2] = address;
  data[3] = data[1] + data[2];
  data[4] = 0x16;
  data[5] = '\0';

  Serial.write((char *)data);
}

bool mbus_get_response(byte *pdata, unsigned char len_pdata) {
  byte bid = 0;             // current byte of response frame
  byte bid_end = 255;       // last byte of frame calculated from length byte sent
  byte bid_checksum = 255;  // checksum byte of frame (next to last)
  byte len = 0;
  byte checksum = 0;
  bool long_frame_found = false;
  bool complete_frame = false;
  bool frame_error = false;

  unsigned long timer_start = millis();
  while (!frame_error && !complete_frame && (millis() - timer_start) < MBUS_TIMEOUT) {
    while (Serial.available()) {
      byte received_byte = (byte)Serial.read();

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
          bid_end = len + 4 + 2 - 1;
          bid_checksum = bid_end - 1;
        }

        if (bid == 2 && received_byte != len) {  // 3rd byte is also length, check that its the same as 2nd byte
          frame_error = true;
        }
        if (bid == 3 && received_byte != 0x68) {
          ;  // 4th byte is the start byte again
          frame_error = true;
        }
        if (bid > 3 && bid < bid_checksum) checksum += received_byte;  // Increment checksum during data portion of frame

        if (bid == bid_checksum && received_byte != checksum) {  // Validate checksum
          frame_error = true;
        }
        if (bid == bid_end && received_byte == 0x16) {  // Parse frame if still valid
          complete_frame = true;
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


