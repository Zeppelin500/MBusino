/*
# MBusino_calibrate 

M-Bus/OneWire/I²C --> MQTT-Gateway with a shield for ESP8266 D1 mini

- M-Bus e.g. heatmeter 
- OneWire 5x e.g. DS18B20, temperature
- I²C e.g.. BME280, temperatur, r. humidity, air pressure

## Licence
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/

#include <EspMQTTClient.h>
#include <OneWire.h>            // Library for OneWire Bus
#include <DallasTemperature.h>  //Library for DS18B20 Sensors
#include <Wire.h>


#include <credentials.h>  // <-- comment it out if you use no library for WLAN access data.

#include <MBusinoLib.h>  // Library for decode M-Bus
#include "ArduinoJson.h"
#include <EEPROM.h>

#define MBUSINO_NAME "MBusino" // If you have more MBusinos, rename it inside quote marks, or it cause some network and MQTT problems. Also you cant reach your MBusino from Arduino IDE
#define DS_EXTENSION 7 // Number of possible OnWires, only 5 or 7 are possible

#if DS_EXTENSION != 7
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#endif

#define MBUS_BAUD_RATE 2400
#define MBUS_ADDRESS 0xFE  // brodcast
#define MBUS_TIMEOUT 1000  // milliseconds
#define MBUS_DATA_SIZE 510
#define MBUS_GOOD_FRAME true
#define MBUS_BAD_FRAME false

#define ONE_WIRE_BUS1 2   //D4
#define ONE_WIRE_BUS2 13  //D7
#define ONE_WIRE_BUS3 12  //D6
#define ONE_WIRE_BUS4 14  //D5
#define ONE_WIRE_BUS5 0   //D3

#if DS_EXTENSION == 7
#define ONE_WIRE_BUS6 5   //D1
#define ONE_WIRE_BUS7 4   //D2
#else
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;  // I2C
#endif

EspMQTTClient client(
  WLAN_SSID,       // WLAN-SSID in quote marks "mySSID"
  WLAN_PASSWORD,   // WLAN Password in quote marks "myPassword"
  MQTT_Broker,     // MQTT Broker IP address in quote marks "192.168.x.x"
  "MQTTUsername",  // Can be omitted if not needed
  "MQTTPassword",  // Can be omitted if not needed -- also password for OTA Update with Arduino IDE 2
  MBUSINO_NAME,    // Client name that uniquely identify your device
  1883             // The MQTT port, default to 1883. this line can be omitted
);


// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire1(ONE_WIRE_BUS1);
OneWire oneWire2(ONE_WIRE_BUS2);
OneWire oneWire3(ONE_WIRE_BUS3);
OneWire oneWire4(ONE_WIRE_BUS4);
OneWire oneWire5(ONE_WIRE_BUS5);
#if DS_EXTENSION == 7
OneWire oneWire6(ONE_WIRE_BUS6);
OneWire oneWire7(ONE_WIRE_BUS7);
#endif

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);
DallasTemperature sensor4(&oneWire4);
DallasTemperature sensor5(&oneWire5);
#if DS_EXTENSION == 7
DallasTemperature sensor6(&oneWire6);
DallasTemperature sensor7(&oneWire7);
#endif

unsigned long loop_start = 0;
unsigned long last_loop = 0;
bool firstrun = true;
int Startadd = 0x13;  // Start address for decoding
char jsonstring[6144] = { 0 };

float OW[DS_EXTENSION] = {0};         // variables for DS18B20 Onewire sensors 
#if DS_EXTENSION != 7
float temperatur = 0;      // Variablen für den BLE280 am I2C
float druck = 0;
float hoehe = 0;
float feuchte = 0;
bool bmeStatus;
#endif

int publIntervalSensoren = 10000;  // publication interval for sensor values in milliseconds
int MbusInterval = 5000;           // interval for MBus request in milliseconds

unsigned long timerMQTT = 15000;
unsigned long timerSensorRefresh1 = 0;
unsigned long timerSensorRefresh2 = 0;
unsigned long timerMbus = 0;

void mbus_request_data(byte address);
void mbus_short_frame(byte address, byte C_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);
void calibrationAverage();
void calibrationSensor(uint8_t sensor);
void calibrationValue(float value);
void calibrationSet0();
#if DS_EXTENSION != 7
void calibrationBME();
#endif
// ------------------- new code for calibration -----------------------------------
uint8_t eeAddrCalibrated = 0;
uint8_t eeAddrOffset[7] = {4,8,12,16,20,24,28};  //EEPROM address to start reading from
int calibrated = 500;  // shows if EEPROM used befor
float offset[DS_EXTENSION] = {0};
float OWwO[DS_EXTENSION] = {0};  // Variables for DS18B20 Onewire Sensores with Offset (One Wire1 with Offset)
bool OWnotconnected[DS_EXTENSION] = {false};
uint8_t sensorToCalibrate = 0;


void setup() {
  Serial.begin(2400, SERIAL_8E1);

  EEPROM.begin(512);
  EEPROM.get(eeAddrCalibrated, calibrated);
  if(calibrated==500){ // if calibrated not 500 the EEPROM is not used befor and full of junk 
    for(uint8_t i = 0; i <=DS_EXTENSION; i++){
      EEPROM.get(eeAddrOffset[i], offset[i]);
    }
  }
  else{
      for(uint8_t i = 0; i < DS_EXTENSION; i++){    
        EEPROM.put(eeAddrOffset[i], 0);  // copy offset 0 to EEPROM
      }
      calibrated = 500;
      EEPROM.put(eeAddrCalibrated, calibrated);  // copy the key to EEPROM that the EEPROM is writen and not full of junk.
  }
  EEPROM.commit();
  EEPROM.end();



  // OneWire vorbereiten
  sensor1.setWaitForConversion(false);  // makes it async
  sensor2.setWaitForConversion(false);  // makes it async
  sensor3.setWaitForConversion(false);  // makes it async
  sensor4.setWaitForConversion(false);  // makes it async
  sensor5.setWaitForConversion(false);  // makes it async
  #if DS_EXTENSION == 7
  sensor6.setWaitForConversion(false);  // makes it async
  sensor7.setWaitForConversion(false);  // makes it async
  #endif

  // Start up the library
  sensor1.begin();
  sensor2.begin();
  sensor3.begin();
  sensor4.begin();
  sensor5.begin();
  #if DS_EXTENSION == 7
  sensor6.begin();
  sensor7.begin();
  #endif

  // Optional functionalities of EspMQTTClient
  client.enableHTTPWebUpdater();                                           // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA();                                                      // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage(MBUSINO_NAME"/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
  client.setMaxPacketSize(6000);

  #if DS_EXTENSION != 7
  // Vorbereitungen für den BME280
  bmeStatus = bme.begin(0x76);
  #endif
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished() {  // send a message to MQTT broker if connected.
  client.publish(MBUSINO_NAME"/start", "bin hoch gefahren, WLAN und MQTT seht ");

  client.subscribe(MBUSINO_NAME"/calibrateAverage", [](const String &mqttpayload) {
    calibrationAverage();
  });
  client.subscribe(MBUSINO_NAME"/calibrateSensor", [](const String &mqttpayload) {
    calibrationSensor(mqttpayload.toInt()-1);
  });
  client.subscribe(MBUSINO_NAME"/calibrateValue", [](const String &mqttpayload) {
    calibrationValue(mqttpayload.toFloat());
  });  
  #if DS_EXTENSION != 7
  client.subscribe(MBUSINO_NAME"/calibrateBME", [](const String &mqttpayload) {
    calibrationBME();
  });
  #endif
  client.subscribe(MBUSINO_NAME"/calibrateSet0", [](const String &mqttpayload) {
    calibrationSet0();
  });    
}

void sensorRefresh1() {           //stößt aktuallisierte Sensorwerte aus den OneWire Sensoren an
  sensor1.requestTemperatures();  // Send the command to get temperatures
  sensor2.requestTemperatures();
  sensor3.requestTemperatures();
  sensor4.requestTemperatures();
  sensor5.requestTemperatures();
  #if DS_EXTENSION == 7
  sensor6.requestTemperatures();
  sensor7.requestTemperatures();
  #endif
}
void sensorRefresh2() {  //holt die aktuallisierten Sensorwerte aus den OneWire Sensoren
  OW[0] = sensor1.getTempCByIndex(0);
  OW[1] = sensor2.getTempCByIndex(0);
  OW[2] = sensor3.getTempCByIndex(0);
  OW[3] = sensor4.getTempCByIndex(0);
  OW[4] = sensor5.getTempCByIndex(0);
  #if DS_EXTENSION == 7
  OW[5] = sensor5.getTempCByIndex(0);
  OW[6] = sensor5.getTempCByIndex(0);
  #endif

  OWwO[0] = OW[0] + offset[0];  // Messwert plus Offset from calibration
  OWwO[1] = OW[1] + offset[1];
  OWwO[2] = OW[2] + offset[2];
  OWwO[3] = OW[3] + offset[3];
  OWwO[4] = OW[4] + offset[4];
  #if DS_EXTENSION == 7
  OWwO[5] = OW[5] + offset[5];
  OWwO[6] = OW[6] + offset[6];
  #else
  temperatur = bme.readTemperature();
  druck = bme.readPressure() / 100.0F;
  hoehe = bme.readAltitude(SEALEVELPRESSURE_HPA);
  feuchte = bme.readHumidity();
  #endif

}


void loop() {
  client.loop();  //MQTT Funktion
  loop_start = millis();



  if ((millis() - timerSensorRefresh1) > 200) {  // springt in die Funktion zum anstoßen der aktuallisierung der Sensorwerte
    sensorRefresh1();
    timerSensorRefresh1 = (millis() - 1000);
  }
  if ((millis() - timerSensorRefresh2) > 1000) {  // springt in die Funktion zum holen der neuen Sensorwerte
    sensorRefresh2();
    timerSensorRefresh1 = millis();
    timerSensorRefresh2 = millis();
  }

  if (millis() > (timerMQTT + publIntervalSensoren)) { //jede Sekunde folgende MQTT Nachrichten senden
    for(uint8_t i = 0; i < DS_EXTENSION; i++){
      if(OW[i] != -127){
        client.publish(MBUSINO_NAME"/OneWire/S" + String(i+1), String(OWwO[i]).c_str());
        client.publish(MBUSINO_NAME"/OneWire/offset" + String(i+1), String(offset[i]).c_str());
      }
    }
    #if DS_EXTENSION != 7
    client.publish(MBUSINO_NAME"/bme/Temperatur", String(temperatur).c_str());
    client.publish(MBUSINO_NAME"/bme/Druck", String(druck).c_str());
    client.publish(MBUSINO_NAME"/bme/Hoehe", String(hoehe).c_str());
    client.publish(MBUSINO_NAME"/bme/Feuchte", String(feuchte).c_str());
    #endif
    timerMQTT = millis();
  }

  if ((loop_start-last_loop)>= MbusInterval || firstrun) { // 9800 = ~10 seconds
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

void calibrationAverage() {

  client.publish(MBUSINO_NAME"/cal/started", "buliding average");

  float sum = 0;
  uint8_t connected = 0;
    for(uint8_t i = 0; i < DS_EXTENSION; i++){
      if(OW[i] == -127){ 
        OWnotconnected[i] = true;       
      }  
      else{  
        sum += OW[i];
        connected++;
      }
    }
  float average = sum / connected;
  client.publish(MBUSINO_NAME"/cal/connected", String(connected).c_str());
  client.publish(MBUSINO_NAME"/cal/sum", String(sum).c_str());
  client.publish(MBUSINO_NAME"/cal/average", String(average).c_str()); 

  for(uint8_t i = 0; i < DS_EXTENSION; i++){
    if(!OWnotconnected[i]){
      offset[i] = average - OW[i];
    }

  }
  for(uint8_t i = 0; i < DS_EXTENSION; i++){
    if(!OWnotconnected[i]){
      client.publish(MBUSINO_NAME"/cal/offsetS" + String(i+1), String(offset[i]).c_str());
    }
  }
  EEPROM.begin(512);
  for(uint8_t i = 0; i < DS_EXTENSION; i++){
    if(!OWnotconnected[i]){
      EEPROM.put(eeAddrOffset[i], offset[i]);  // copy offset to EEPROM
    }
  }
  EEPROM.commit();
  EEPROM.end();
}

void calibrationSensor(uint8_t sensor){
    if((sensor<DS_EXTENSION) && (OW[sensor]!= -127)){
      sensorToCalibrate = sensor;
    }
    else{
      client.publish(MBUSINO_NAME"/cal/offffsetS" + String(sensor+1), "No valid sensor");
    }

}

void calibrationValue(float value){
  client.publish(MBUSINO_NAME"/cal/started", "set new offset");
   
  if(OW[sensorToCalibrate] != -127){
    offset[sensorToCalibrate] += value;
    client.publish(MBUSINO_NAME"/cal/offsetS" + String(sensorToCalibrate+1), String(offset[sensorToCalibrate]).c_str());
    EEPROM.begin(512);
    EEPROM.put(eeAddrOffset[sensorToCalibrate], offset[sensorToCalibrate]);  // copy offset to EEPROM
    EEPROM.commit();
    EEPROM.end();
  }
  else{
    client.publish(MBUSINO_NAME"/cal/offsetS" + String(sensorToCalibrate), "No Sensor");
  }
}
#if DS_EXTENSION != 7
void calibrationBME(){

  client.publish(MBUSINO_NAME"/cal/started", "set BME280 based offset");
  client.publish(MBUSINO_NAME"/cal/BME", String(temperatur).c_str());

  for(uint8_t i = 0; i < 5; i++){
    if(OW[i] == -127){ 
     OWnotconnected[i] = true;       
     }  
  }

  for(uint8_t i = 0; i < 5; i++){
    if(!OWnotconnected[i]){
      offset[i] = temperatur - OW[i];
    }

  }
  for(uint8_t i = 0; i < 5; i++){
    if(!OWnotconnected[i]){
      client.publish(MBUSINO_NAME"/cal/offsetS" + String(i+1), String(offset[i]).c_str());
    }
  }
  EEPROM.begin(512);
  for(uint8_t i = 0; i < 5; i++){
    if(!OWnotconnected[i]){
      EEPROM.put(eeAddrOffset[i], offset[i]);  // copy offset to EEPROM
    }
  }
  EEPROM.commit();
  EEPROM.end();
}
#endif

void calibrationSet0(){

  client.publish(MBUSINO_NAME"/cal/started", "set all offsets 0");

  for(uint8_t i = 0; i < DS_EXTENSION; i++){
    if(OW[i] == -127){ 
     OWnotconnected[i] = true;       
     }  
  }
  for(uint8_t i = 0; i < DS_EXTENSION; i++){
      offset[i] = 0;
  }
  for(uint8_t i = 0; i < DS_EXTENSION; i++){
    if(!OWnotconnected[i]){
      client.publish(MBUSINO_NAME"/cal/offsetS" + String(i+1), String(offset[i]).c_str());
    }
  }
  EEPROM.begin(512);
  for(uint8_t i = 0; i < DS_EXTENSION; i++){
    EEPROM.put(eeAddrOffset[i], offset[i]);  // copy offset to EEPROM
  }
  EEPROM.commit();
  EEPROM.end();
}
