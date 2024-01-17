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

#define MBUSINO_NAME "SBusino" // If you have more MBusinos, rename it inside quote marks, or it cause some network and MQTT problems. Also you cant reach your MBusino from Arduino IDE

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

void mbus_normalize(byte address);
void mbus_request_data(byte address);
void mbus_short_frame(byte address, byte C_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);
void calibrationAverage();
void calibrationSensor(uint8_t sensor);
void calibrationValue(float value);
void calibrationBME();
void calibrationSet0();

uint8_t scanAddress = 10;
unsigned long scanTimer = 0;
uint8_t scanned = false;

void setup() {
  Serial.begin(2400, SERIAL_8E1);

  // Optional functionalities of EspMQTTClient
  client.enableHTTPWebUpdater();                                           // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA();                                                      // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage(MBUSINO_NAME"/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
  client.setMaxPacketSize(6000);

  scanTimer = millis()+10000;
  

}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished() {  // send a message to MQTT broker if connected.
  client.publish(MBUSINO_NAME"/start", "bin hoch gefahren, WLAN und MQTT seht ");  
}

void loop() {
  client.loop();  //MQTT Funktion
  loop_start = millis();

    if((millis() - scanTimer) > 1000 && scanned == false){
    scanTimer = millis();
    client.publish(String(MBUSINO_NAME"/scanning/" + String(scanAddress)), "done");  
    mbus_normalize(scanAddress);
 
    scanned = true;
  }
    if((millis() - scanTimer) > 1000 && scanned == true){
    scanTimer = millis();
    client.publish(String(MBUSINO_NAME "/reading/" + String(scanAddress)), "done");  
    if (Serial.available()) {
      byte val = Serial.read();
      client.publish(String(MBUSINO_NAME "/scanning/recieved_" + String(scanAddress)), String(val)); 
    }
    scanned = false;
    scanAddress--;
  }
   
 
}

void mbus_normalize(byte address) {
  mbus_short_frame(address,0x40);
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


