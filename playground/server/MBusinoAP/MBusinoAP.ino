/*
# MBusinoAP - MBusino with Access Point setup portal

https://github.com/Zeppelin500/MBusino/

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
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
//#include <ESPAsyncWebSrv.h> 
//#include <ESPAsyncTCP.h>


#include <MBusinoLib.h>  // Library for decode M-Bus
#include <ArduinoJson.h>
#include <EEPROM.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define MBUS_BAUD_RATE 2400
#define MBUS_ADDRESS 0xFE  // brodcast
#define MBUS_TIMEOUT 1000  // milliseconds
#define MBUS_DATA_SIZE 255
#define MBUS_GOOD_FRAME true
#define MBUS_BAD_FRAME false

#define ONE_WIRE_BUS1 2   //D4
#define ONE_WIRE_BUS2 13  //D7
#define ONE_WIRE_BUS3 12  //D6
#define ONE_WIRE_BUS4 14  //D5
#define ONE_WIRE_BUS5 0   //D3
#define ONE_WIRE_BUS6 5   //D1
#define ONE_WIRE_BUS7 4   //D2

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;  // I2C

ESP8266WebServer    server(80);
EspMQTTClient client;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire1(ONE_WIRE_BUS1);
OneWire oneWire2(ONE_WIRE_BUS2);
OneWire oneWire3(ONE_WIRE_BUS3);
OneWire oneWire4(ONE_WIRE_BUS4);
OneWire oneWire5(ONE_WIRE_BUS5);

OneWire oneWire6(ONE_WIRE_BUS6);
OneWire oneWire7(ONE_WIRE_BUS7);


// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensor1(&oneWire1);
DallasTemperature sensor2(&oneWire2);
DallasTemperature sensor3(&oneWire3);
DallasTemperature sensor4(&oneWire4);
DallasTemperature sensor5(&oneWire5);

DallasTemperature sensor6(&oneWire6);
DallasTemperature sensor7(&oneWire7);


struct settings {
  char ssid[30];
  char password[30];
  char mbusinoName[11];
  char broker[20];
  uint16_t mqttPort;
  uint16_t extension;
  char mqttUser[30];
  char mqttPswrd[30]; 
  uint32_t sensorInterval;
  uint32_t mbusInterval; 
} userData = {};

bool mqttcon = false;
bool apMode = false;

int Startadd = 0x13;  // Start address for decoding

float OW[7] = {0};         // variables for DS18B20 Onewire sensors 
float temperatur = 0;      // Variablen für den BLE280 am I2C
float druck = 0;
float hoehe = 0;
float feuchte = 0;
bool bmeStatus;


int sensorInterval = 10000;  // publication interval for sensor values in milliseconds
int mbusInterval = 5000;           // interval for MBus request in milliseconds
bool mbusReq = false;

unsigned long timerMQTT = 15000;
unsigned long timerSensorRefresh1 = 0;
unsigned long timerSensorRefresh2 = 0;
unsigned long timerMbus = 0;
unsigned long timerMbus2 = 0;
unsigned long timer = 0;

void mbus_request_data(byte address);
void mbus_short_frame(byte address, byte C_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);
void calibrationAverage();
void calibrationSensor(uint8_t sensor);
void calibrationValue(float value);
void calibrationSet0();
void calibrationBME();


uint8_t eeAddrCalibrated = 0;
uint8_t eeAddrCredentialsSaved = 32;
uint8_t eeAddrOffset[7] = {4,8,12,16,20,24,28};  //EEPROM address to start reading from
uint16_t calibrated = 123;  // shows if EEPROM used befor for offsets
uint16_t credentialsSaved = 123;  // shows if EEPROM used befor for credentials
float offset[7] = {0};
float OWwO[7] = {0};  // Variables for DS18B20 Onewire Sensores with Offset (One Wire1 with Offset)
bool OWnotconnected[7] = {false};
uint8_t sensorToCalibrate = 0;


void setup() {
  Serial.setRxBufferSize(256);
  Serial.begin(MBUS_BAUD_RATE, SERIAL_8E1);

  EEPROM.begin(512);
  EEPROM.get(eeAddrCalibrated, calibrated);
  if(calibrated==500){ // if calibrated not 500 the EEPROM is not used befor and full of junk 
    for(uint8_t i = 0; i <=userData.extension; i++){
      EEPROM.get(eeAddrOffset[i], offset[i]);
    }
  }
  else{
      for(uint8_t i = 0; i < userData.extension; i++){    
        EEPROM.put(eeAddrOffset[i], 0);  // copy offset 0 to EEPROM
      }
      calibrated = 500;
      EEPROM.put(eeAddrCalibrated, calibrated);  // copy the key to EEPROM that the EEPROM is writen and not full of junk.
  }
  EEPROM.get(eeAddrCredentialsSaved, credentialsSaved);
  if(credentialsSaved == 500){
    EEPROM.get(100, userData );
  }
  else{
    userData.sensorInterval = 10000;
    userData.mbusInterval = 10000;
  }
  EEPROM.commit();
  EEPROM.end();

  WiFi.hostname(userData.mbusinoName);
  client.setMqttClientName(userData.mbusinoName);
  client.setMqttServer(userData.broker, userData.mqttUser, userData.mqttPswrd, userData.mqttPort);

  WiFi.mode(WIFI_STA);
  WiFi.begin(userData.ssid, userData.password);

  byte tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (tries++ > 10) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("MBusino Setup Portal"); //, "secret");
      apMode = true;
      break;
    }
  }
  server.on("/",  handlePortal);
  server.begin();

  // OneWire vorbereiten
  if(userData.extension > 0){
    sensor1.setWaitForConversion(false);  // makes it async
    sensor2.setWaitForConversion(false);  // makes it async
    sensor3.setWaitForConversion(false);  // makes it async
    sensor4.setWaitForConversion(false);  // makes it async
    sensor5.setWaitForConversion(false);  // makes it async
  }
  if(userData.extension == 7){
    sensor6.setWaitForConversion(false);  // makes it async
    sensor7.setWaitForConversion(false);  // makes it async
  }

  // Start up the library
  if(userData.extension > 0){
    sensor1.begin();
    sensor2.begin();
    sensor3.begin();
    sensor4.begin();
    sensor5.begin();
  }
  if(userData.extension == 7){
    sensor6.begin();
    sensor7.begin();
  }

  // Optional functionalities of EspMQTTClient
  //client.enableHTTPWebUpdater();                                 // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA("mbusino"); 
  char lwBuffer[30] = {0};
  sprintf(lwBuffer, userData.mbusinoName, "/lastwill");                    // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage(lwBuffer , "I am going offline");  // You can activate the retain flag by setting the third parameter to true
  client.setMaxPacketSize(6000);

  if(userData.extension == 5){
    // Vorbereitungen für den BME280
    bmeStatus = bme.begin(0x76);
  }
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished() {  // send a message to MQTT broker if connected.
  mqttcon = true;
  client.publish(String(userData.mbusinoName) + "/start", "bin hoch gefahren, WLAN und MQTT seht ");
  if(userData.extension > 0){
    client.subscribe(String(userData.mbusinoName) + "/calibrateAverage", [](const String &mqttpayload) {
      calibrationAverage();
    });
    client.subscribe(String(userData.mbusinoName) + "/calibrateSensor", [](const String &mqttpayload) {
      calibrationSensor(mqttpayload.toInt()-1);
    });
    client.subscribe(String(userData.mbusinoName) + "/calibrateValue", [](const String &mqttpayload) {
      calibrationValue(mqttpayload.toFloat());
    });  
    if(userData.extension == 5){
      client.subscribe(String(userData.mbusinoName) + "/calibrateBME", [](const String &mqttpayload) {
        calibrationBME();
    });
    }
    client.subscribe(String(userData.mbusinoName) + "/calibrateSet0", [](const String &mqttpayload) {
      calibrationSet0();
    });  
  }  
}


void loop() {
  client.loop();  //MQTT Funktion
  //if(mqttcon == false){
    server.handleClient();
  //}
  if(apMode == true && millis() > 300000){
    ESP.restart();
  }
  
  /////////////////for debug///////////////////////////////////
  if((millis()-timer) >10000){
    timer = millis();
    client.publish(String(userData.mbusinoName) + "/eeprom/ssid", String(userData.ssid)); 
    //client.publish(String(userData.mbusinoName) + "/eeprom/password", String(userData.password)); 
    client.publish(String(userData.mbusinoName) + "/eeprom/broker", String(userData.broker)); 
    client.publish(String(userData.mbusinoName) + "/eeprom/port", String(userData.mqttPort)); 
    client.publish(String(userData.mbusinoName) + "/eeprom/user", String(userData.mqttUser)); 
    client.publish(String(userData.mbusinoName) + "/eeprom/pswd", String(userData.mqttPswrd)); 
    client.publish(String(userData.mbusinoName) + "/eeprom/name", String(userData.mbusinoName)); 
    client.publish(String(userData.mbusinoName) + "/eeprom/extension", String(userData.extension)); 
    client.publish(String(userData.mbusinoName) + "/eeprom/mbusInterval", String(userData.mbusInterval)); 
    client.publish(String(userData.mbusinoName) + "/eeprom/sensorInterval", String(userData.sensorInterval));     
  }
  ///////////////////////////////////////////////////////////
  

  if(userData.extension > 0){
    if ((millis() - timerSensorRefresh1) > 200) {  // springt in die Funktion zum anstoßen der aktuallisierung der Sensorwerte
      sensorRefresh1();
      timerSensorRefresh1 = (millis() - 1000);
    }
    if ((millis() - timerSensorRefresh2) > 1000) {  // springt in die Funktion zum holen der neuen Sensorwerte
      sensorRefresh2();
      timerSensorRefresh1 = millis();
      timerSensorRefresh2 = millis();
    }
  }  
  if (millis() > (timerMQTT + sensorInterval)) { //MQTT Nachrichten senden
    for(uint8_t i = 0; i < userData.extension; i++){
      if(OW[i] != -127){
        client.publish(String(userData.mbusinoName) + "/OneWire/S" + String(i+1), String(OWwO[i]).c_str());
        client.publish(String(userData.mbusinoName) + "/OneWire/offset" + String(i+1), String(offset[i]).c_str());
      }      
    }
  
    if(userData.extension == 5){
      client.publish(String(userData.mbusinoName) + "/bme/Temperatur", String(temperatur).c_str());
      client.publish(String(userData.mbusinoName) + "/bme/Druck", String(druck).c_str());
      client.publish(String(userData.mbusinoName) + "/bme/Hoehe", String(hoehe).c_str());
      client.publish(String(userData.mbusinoName) + "/bme/Feuchte", String(feuchte).c_str());
    }
    timerMQTT = millis();
  }

  if(millis() - timerMbus > mbusInterval){
    timerMbus = millis();
    timerMbus2 = millis();
    mbusReq = true;
    mbus_request_data(MBUS_ADDRESS);
  }
  if(millis() - timerMbus2 > 1000 && mbusReq == true){
    mbusReq = false;
    bool mbus_good_frame = false;
    byte mbus_data[MBUS_DATA_SIZE] = { 0 };
    mbus_good_frame = mbus_get_response(mbus_data, sizeof(mbus_data));

    /*
    //------------------ only for debug, you will recieve the whole M-Bus telegram bytewise in HEX for analysis -----------------
    for(uint8_t i = 0; i <= mbus_data[1]+1; i++){                                                             //|
      char buffer[3];                                                                                         //|
      sprintf(buffer,"%02X",mbus_data[i]);                                                                    //|
      client.publish(String(String(userData.mbusinoName) + "/debug/telegram_byte_"+String(i)), String(buffer).c_str());          //|
    }                                                                                                         //|
    //--------------------------------------------------------------------------------------------------------------------------    
    */
    //mbus_good_frame = true;
    //byte mbus_data2[] = {0x68,0xC1,0xC1,0x68,0x08,0x00,0x72,0x09,0x34,0x75,0x73,0xC5,0x14,0x00,0x0D,0x43,0x00,0x00,0x00,0x04,0x78,0x41,0x63,0x65,0x04,0x04,0x06,0xAA,0x29,0x00,0x00,0x04,0x13,0x40,0xA1,0x75,0x00,0x04,0x2B,0x00,0x00,0x00,0x00,0x14,0x2B,0x3C,0xF3,0x00,0x00,0x04,0x3B,0x48,0x06,0x00,0x00,0x14,0x3B,0x4E,0x0E,0x00,0x00,0x02,0x5B,0x19,0x00,0x02,0x5F,0x19,0x00,0x02,0x61,0xFA,0xFF,0x02,0x23,0xAC,0x08,0x04,0x6D,0x03,0x2A,0xF1,0x2A,0x44,0x06,0x92,0x0C,0x00,0x00,0x44,0x13,0x2D,0x9B,0x1C,0x00,0x42,0x6C,0xDF,0x2C,0x01,0xFD,0x17,0x00,0x03,0xFD,0x0C,0x05,0x00,0x00,0x84,0x10,0x06,0x1A,0x00,0x00,0x00,0xC4,0x10,0x06,0x05,0x00,0x00,0x00,0x84,0x20,0x06,0x00,0x00,0x00,0x00,0xC4,0x20,0x06,0x00,0x00,0x00,0x00,0x84,0x30,0x06,0x00,0x00,0x00,0x00,0xC4,0x30,0x06,0x00,0x00,0x00,0x00,0x84,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0x40,0x13,0x00,0x00,0x00,0x00,0x84,0x80,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0x80,0x40,0x13,0x00,0x00,0x00,0x00,0x84,0xC0,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0xC0,0x40,0x13,0x00,0x00,0x00,0x00,0x75,0x16};


    if (mbus_good_frame) {
      int packet_size = mbus_data[1] + 6; 
      MBusinoLib payload(255);  
      DynamicJsonDocument jsonBuffer(4080);
      JsonArray root = jsonBuffer.createNestedArray();  
      uint8_t fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 
      char jsonstring[2048] = { 0 };
      yield();
      serializeJson(root, jsonstring);
      client.publish(String(userData.mbusinoName) + "/MBus/error", String(payload.getError()));  // kann auskommentiert werden wenn es läuft
      yield();
      client.publish(String(userData.mbusinoName) + "/MBus/jsonstring", String(jsonstring));

      for (uint8_t i=0; i<fields; i++) {
        uint8_t code = root[i]["code"].as<int>();
        const char* name = root[i]["name"];
        const char* units = root[i]["units"];           
        double value = root[i]["value_scaled"].as<double>(); 
        const char* valueString = root[i]["value_string"];            

        //two messages per value, values comes as number or as ASCII string or both
        client.publish(String(String(userData.mbusinoName) + "/MBus/"+String(i+1)+"_vs_"+String(name)), valueString); // send the value if a ascii value is aviable (variable length)
        client.publish(String(String(userData.mbusinoName) + "/MBus/"+String(i+1)+"_"+String(name)), String(value,3).c_str()); // send the value if a real value is aviable (standard)
        client.publish(String(String(userData.mbusinoName) + "/MBus/"+String(i+1)+"_"+String(name)+"_unit"), units);
        //or only one message
        //client.publish(String(String(userData.mbusinoName) + "/MBus/"+String(i+1)+"_"+String(name)+"_in_"+String(units)), String(value,3).c_str());

        if (i == 3){  // Sensostar Bugfix --> comment it out if you use not a Sensostar
          float flow = root[5]["value_scaled"].as<float>();
          float delta = root[9]["value_scaled"].as<float>();
          float calc_power = delta * flow * 1163;          
          client.publish(String(userData.mbusinoName) + "/MBus/4_power_calc", String(calc_power).c_str());                    
        }   
        yield();      
      }
    } 
    else {
  //Fehlermeldung
        client.publish(String(String(userData.mbusinoName) + "/MBUSerror"), "no_good_telegram");
    }
  }
}

void sensorRefresh1() {           //stößt aktuallisierte Sensorwerte aus den OneWire Sensoren an
  sensor1.requestTemperatures();  // Send the command to get temperatures
  sensor2.requestTemperatures();
  sensor3.requestTemperatures();
  sensor4.requestTemperatures();
  sensor5.requestTemperatures();
  if(userData.extension == 7){
    sensor6.requestTemperatures();
    sensor7.requestTemperatures();
  }
}
void sensorRefresh2() {  //holt die aktuallisierten Sensorwerte aus den OneWire Sensoren
  OW[0] = sensor1.getTempCByIndex(0);
  OW[1] = sensor2.getTempCByIndex(0);
  OW[2] = sensor3.getTempCByIndex(0);
  OW[3] = sensor4.getTempCByIndex(0);
  OW[4] = sensor5.getTempCByIndex(0);
  if(userData.extension == 7){
    OW[5] = sensor6.getTempCByIndex(0);
    OW[6] = sensor7.getTempCByIndex(0);
  }

  OWwO[0] = OW[0] + offset[0];  // Messwert plus Offset from calibration
  OWwO[1] = OW[1] + offset[1];
  OWwO[2] = OW[2] + offset[2];
  OWwO[3] = OW[3] + offset[3];
  OWwO[4] = OW[4] + offset[4];
  if(userData.extension == 7){
    OWwO[5] = OW[5] + offset[5];
    OWwO[6] = OW[6] + offset[6];
  }
  else{
    temperatur = bme.readTemperature();
    druck = bme.readPressure() / 100.0F;
    hoehe = bme.readAltitude(SEALEVELPRESSURE_HPA);
    feuchte = bme.readHumidity();
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
  uint16_t j = 0;

  while (!frame_error && !complete_frame){
    j++;
    if(j>255){
      frame_error = true;
    }
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
      yield();
    }
  }

  if (complete_frame && !frame_error) {
    return MBUS_GOOD_FRAME;
  } else {
    return MBUS_BAD_FRAME;
  }
}

void calibrationAverage() {

  client.publish(String(userData.mbusinoName) + "/cal/started", "buliding average");

  float sum = 0;
  uint8_t connected = 0;
    for(uint8_t i = 0; i < userData.extension; i++){
      if(OW[i] == -127){ 
        OWnotconnected[i] = true;       
      }  
      else{  
        sum += OW[i];
        connected++;
      }
    }
  float average = sum / connected;
  client.publish(String(userData.mbusinoName) + "/cal/connected", String(connected).c_str());
  client.publish(String(userData.mbusinoName) + "/cal/sum", String(sum).c_str());
  client.publish(String(userData.mbusinoName) + "/cal/average", String(average).c_str()); 

  for(uint8_t i = 0; i < userData.extension; i++){
    if(!OWnotconnected[i]){
      offset[i] = average - OW[i];
    }

  }
  for(uint8_t i = 0; i < userData.extension; i++){
    if(!OWnotconnected[i]){
      client.publish(String(userData.mbusinoName) + "/cal/offsetS" + String(i+1), String(offset[i]).c_str());
    }
  }
  EEPROM.begin(512);
  for(uint8_t i = 0; i < userData.extension; i++){
    if(!OWnotconnected[i]){
      EEPROM.put(eeAddrOffset[i], offset[i]);  // copy offset to EEPROM
    }
  }
  EEPROM.commit();
  EEPROM.end();
}

void calibrationSensor(uint8_t sensor){
    if((sensor<userData.extension) && (OW[sensor]!= -127)){
      sensorToCalibrate = sensor;
    }
    else{
      client.publish(String(userData.mbusinoName) + "/cal/offffsetS" + String(sensor+1), "No valid sensor");
    }

}

void calibrationValue(float value){
  client.publish(String(userData.mbusinoName) + "/cal/started", "set new offset");
   
  if(OW[sensorToCalibrate] != -127){
    offset[sensorToCalibrate] += value;
    client.publish(String(userData.mbusinoName) + "/cal/offsetS" + String(sensorToCalibrate+1), String(offset[sensorToCalibrate]).c_str());
    EEPROM.begin(512);
    EEPROM.put(eeAddrOffset[sensorToCalibrate], offset[sensorToCalibrate]);  // copy offset to EEPROM
    EEPROM.commit();
    EEPROM.end();
  }
  else{
    client.publish(String(userData.mbusinoName) + "/cal/offsetS" + String(sensorToCalibrate), "No Sensor");
  }
}

void calibrationBME(){

  client.publish(String(userData.mbusinoName) + "/cal/started", "set BME280 based offset");
  client.publish(String(userData.mbusinoName) + "/cal/BME", String(temperatur).c_str());

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
      client.publish(String(userData.mbusinoName) + "/cal/offsetS" + String(i+1), String(offset[i]).c_str());
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


void calibrationSet0(){

  client.publish(String(userData.mbusinoName) + "/cal/started", "set all offsets 0");

  for(uint8_t i = 0; i < userData.extension; i++){
    if(OW[i] == -127){ 
     OWnotconnected[i] = true;       
     }  
  }
  for(uint8_t i = 0; i < userData.extension; i++){
      offset[i] = 0;
  }
  for(uint8_t i = 0; i < userData.extension; i++){
    if(!OWnotconnected[i]){
      client.publish(String(userData.mbusinoName) + "/cal/offsetS" + String(i+1), String(offset[i]).c_str());
    }
  }
  EEPROM.begin(512);
  for(uint8_t i = 0; i < userData.extension; i++){
    EEPROM.put(eeAddrOffset[i], offset[i]);  // copy offset to EEPROM
  }
  EEPROM.commit();
  EEPROM.end();
}

void handlePortal() {

  
  if (server.method() == HTTP_POST) {
    char bufferStr[6] = {0};
    char bufferStr2[6] = {0};
    char bufferStr3[6] = {0};
    char bufferStr4[6] = {0};    
    strncpy(userData.ssid,     server.arg("ssid").c_str(),     sizeof(userData.ssid) );
    strncpy(userData.password, server.arg("password").c_str(), sizeof(userData.password) );
    strncpy(userData.mbusinoName, server.arg("deviceName").c_str(), sizeof(userData.mbusinoName) );
    strncpy(userData.broker, server.arg("broker").c_str(), sizeof(userData.broker) );
    strncpy(bufferStr, server.arg("mqttPort").c_str(), sizeof(bufferStr) );
    strncpy(bufferStr2, server.arg("extension").c_str(), sizeof(bufferStr2) );
    strncpy(bufferStr3, server.arg("sensorInterval").c_str(), sizeof(bufferStr3) );
    strncpy(bufferStr4, server.arg("mbusInterval").c_str(), sizeof(bufferStr4) );
    strncpy(userData.mqttUser, server.arg("mqttUser").c_str(), sizeof(userData.mqttUser) );
    strncpy(userData.mqttPswrd, server.arg("mqttPswrd").c_str(), sizeof(userData.mqttPswrd) );
    userData.mqttPort = atoi(bufferStr);
    userData.extension = atoi(bufferStr2);
    userData.sensorInterval = 1000 * atoi(bufferStr3);
    userData.mbusInterval = 1000 * atoi(bufferStr4);
    userData.ssid[server.arg("ssid").length()] = '\0';
    userData.password[server.arg("password").length()] = '\0';
    userData.mbusinoName[server.arg("deviceName").length()] = '\0';
    userData.broker[server.arg("broker").length()] = '\0';
    userData.mqttUser[server.arg("mqttUser").length()] = '\0';
    userData.mqttPswrd[server.arg("mqttPswrd").length()] = '\0';
    EEPROM.begin(512);
    EEPROM.put(100, userData);
    credentialsSaved = 500;
    EEPROM.put(eeAddrCredentialsSaved, credentialsSaved);
    EEPROM.commit();
    EEPROM.end();

    server.send(200,   "text/html",  "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1>Wifi Setup</h1> <br/> <p>Your settings have been saved successfully!<br />Please restart the device.<br />MQTT should now work. <br /> If you find the Acces Point network again, your credentials were wrong.</p></main></body></html>" );
  } else {

    server.send(200,   "text/html", "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>MBusino Setup</title><style>*,::after,::before{box-sizing:border-box}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#fff;background-color:#438287}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da}button{cursor:pointer;border:1px solid transparent;color:#fff;background-color:#304648;border-color:#304648;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto}h1{text-align:center}</style></head><body><main class='form-signin'><form action='/' method='post'><h1 class=''><i>MBusino</i> Setup</h1><br><div class='form-floating'><label>SSID</label><input type='text' class='form-control' name='ssid'></div><div class='form-floating'><label>Password</label><input type='password' class='form-control' name='password'></div><div class='form-floating'><label>Device Name</label><input type='text' value='MBusino' class='form-control' name='deviceName'></div><br><label for='extension'>Stage of Extension:</label><br><select name='extension' id='extension'><option value='5'>5x DS18B20 + BME</option><option value='7'>7x DS18B20 no BME</option><option value='0'>only M-Bus</option></select><br><br><div class='form-floating'><label>Sensor publish interval sec.</label><input type='text' value='5' class='form-control' name='sensorInterval'></div><div class='form-floating'><label>M-Bus publish interval sec.</label><input type='text' value='120' class='form-control' name='mbusInterval'></div><div class='form-floating'><label>MQTT Broker</label><input type='text' class='form-control' name='broker'></div><div class='form-floating'><label>MQTT Port</label><input type='text' value='1883' class='form-control' name='mqttPort'></div><div class='form-floating'><label>MQTT User (optional)</label><input type='text' class='form-control' name='mqttUser'></div><div class='form-floating'><label>MQTT Password (optional)</label><input type='password' class='form-control' name='mqttPswrd'></div><br>All Fields will be saved, empty fields delete the values<br><button type='submit'>Save</button><p style='text-align:right'><a href='https://www.github.com/zeppelin500/mbusino' style='color:#fff'>MBusino</a></p></form></main></body></html>" );
  }
}