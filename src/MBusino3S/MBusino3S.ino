/*
# MBusino for multible slaves
## ESP8266 D1 Mini or ESP32 S2 Mini

https://github.com/Zeppelin500/MBusino/

## Licence
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/


#include <PubSubClient.h>
#include <OneWire.h>            // Library for OneWire Bus
#include <DallasTemperature.h>  //Library for DS18B20 Sensors
#include <Wire.h>
#include <ESPAsyncWebServer.h> 

#include <DNSServer.h>
#include <ArduinoOTA.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <AsyncTCP.h>
HardwareSerial MbusSerial(1);
#endif

#include <MBusinoLib.h>  // Library for decode M-Bus
#include <ArduinoJson.h>
#include <EEPROM.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


#define MBUSINO_VERSION "0.9.11"

#if defined(ESP8266)
#define ONE_WIRE_BUS1 2   //D4
#define ONE_WIRE_BUS2 13  //D7
#define ONE_WIRE_BUS3 12  //D6
#define ONE_WIRE_BUS4 14  //D5
#define ONE_WIRE_BUS5 0   //D3
#define ONE_WIRE_BUS6 5   //D1
#define ONE_WIRE_BUS7 4   //D2
#elif defined(ESP32)
#define ONE_WIRE_BUS1 16 //2   //D4
#define ONE_WIRE_BUS2 11  //13  //D7
#define ONE_WIRE_BUS3 9   //12  //D6
#define ONE_WIRE_BUS4 7   //14  //D5
#define ONE_WIRE_BUS5 18  //0   //D3
#define ONE_WIRE_BUS6 35  //5   //D1
#define ONE_WIRE_BUS7 33  //4   //D2
#endif


#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;  // I2C
MBusinoLib payload(254);
WiFiClient espClient;
DNSServer dnsServer;


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
  uint8_t mbusSlaves;
  uint8_t mbusAddress1;
  uint8_t mbusAddress2;
  uint8_t mbusAddress3;
  bool haAutodisc;
} userData = {"SSID","Password","MBusino","192.168.1.7",1883,5,"mqttUser","mqttPasword",5000,120000,1,0xFE,0,0,true};

uint8_t mbusAddress[3] = {0};

bool mqttcon = false;
bool apMode = false;
bool credentialsReceived = false;
uint16_t conCounter = 0;

uint8_t newAddress = 0;
bool newAddressReceived = false;
bool waitToSetAddress = false;


uint8_t currentAddress = 0;
uint8_t addressCounter = 0;

int Startadd = 0x13;  // Start address for decoding

float OW[7] = {0};         // variables for DS18B20 Onewire sensors 
float temperatur = 0;      // Variablen für den BLE280 am I2C
float druck = 0;
float hoehe = 0;
float feuchte = 0;
bool bmeStatus;

uint8_t mbusLoopStatus = 0;
uint8_t fields = 0;
char jsonstring[4092] = { 0 };
uint8_t address = 0; 
bool engelmann = false;
bool waitForRestart = false;

unsigned long timerMQTT = 15000;
unsigned long timerSensorRefresh1 = 0;
unsigned long timerSensorRefresh2 = 0;
unsigned long timerMbus = 0;
unsigned long timerDebug = 0;
unsigned long timerReconnect = 0;
unsigned long timerReboot = 0;
unsigned long timerSetAddress = 0;

void mbus_request_data(byte address);
void mbus_short_frame(byte address, byte C_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);
void mbus_normalize(byte address);
void mbus_clearRXbuffer();
void mbus_set_address(byte oldaddress, byte newaddress);
void calibrationAverage();
void calibrationSensor(uint8_t sensor);
void calibrationValue(float value);
void calibrationSet0();
void calibrationBME();
void setupServer();

uint8_t eeAddrCalibrated = 0;
uint8_t eeAddrCredentialsSaved = 32;
uint8_t eeAddrOffset[7] = {4,8,12,16,20,24,28};  //EEPROM address to start reading from
uint16_t calibrated = 123;  // shows if EEPROM used befor for offsets
uint16_t credentialsSaved = 123;  // shows if EEPROM used befor for credentials
float offset[7] = {0};
float OWwO[7] = {0};  // Variables for DS18B20 Onewire Sensores with Offset (One Wire1 with Offset)
bool OWnotconnected[7] = {false};
uint8_t sensorToCalibrate = 0;


uint8_t adMbusMessageCounter = 0; // Counter for autodiscouver mbus message.
uint8_t adSensorMessageCounter = 0; // Counter for autodiscouver mbus message.

//outsourced program parts
#include "html.h"
#include "server.h"
#include "mqtt.h"
#include "mbus.h"
#include "calibration.h"
#include "sensorRefresh.h"
#include "autodiscover.h"


void setup() {

  #if defined(ESP8266)
  Serial.setRxBufferSize(256);
  Serial.begin(MBUS_BAUD_RATE, SERIAL_8E1);
  #elif defined(ESP32)
  MbusSerial.setRxBufferSize(256);
  MbusSerial.begin(MBUS_BAUD_RATE, SERIAL_8E1, 37, 39);
  #endif

  EEPROM.begin(512);
  EEPROM.get(eeAddrCalibrated, calibrated);
  if(calibrated==500){ // if calibrated not 500 the EEPROM is not used befor and full of junk 
    for(uint8_t i = 0; i <=userData.extension; i++){
      EEPROM.get(eeAddrOffset[i], offset[i]);
    }
  }
  else{
      for(uint8_t i = 0; i < 7; i++){    
        EEPROM.put(eeAddrOffset[i], 0);  // copy offset 0 to EEPROM
      }
      calibrated = 500;
      EEPROM.put(eeAddrCalibrated, calibrated);  // copy the key to EEPROM that the EEPROM is writen and not full of junk.
  }
  EEPROM.get(eeAddrCredentialsSaved, credentialsSaved);
  if(credentialsSaved == 500){
    EEPROM.get(100, userData );
  }

  EEPROM.commit();
  EEPROM.end();

  sprintf(html_buffer, index_html,userData.ssid,userData.mbusinoName,userData.extension,userData.haAutodisc,userData.sensorInterval/1000,userData.mbusInterval/1000,userData.broker,userData.mqttPort,userData.mqttUser,userData.mbusSlaves,userData.mbusAddress1,userData.mbusAddress2,userData.mbusAddress3);

  WiFi.hostname(userData.mbusinoName);
  client.setServer(userData.broker, userData.mqttPort);
  client.setCallback(callback);

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
  setupServer();
  if(apMode==true){
    dnsServer.start(53, "*", WiFi.softAPIP());
  }
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP

  //attach AsyncWebSocket
 // ws.onEvent(onEvent);
  server.addHandler(&ws);

  // attach AsyncEventSource
  server.addHandler(&events);

  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", update_html);    
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
    waitForRestart = !Update.hasError();
    if(Update.hasError()==true){
      timerReboot = millis();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", waitForRestart?"success, restart now":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      #if defined(ESP8266)
      Update.runAsync(true);
      #endif
      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
        Update.printError(Serial);
      }
    }
    if(!Update.hasError()){
      if(Update.write(data, len) != len){
        Update.printError(Serial);
      }
    }
    if(final){
      if(Update.end(true)){
        //Serial.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
      }
    }
  });
  ArduinoOTA.setPassword((const char *)"mbusino");
  server.onNotFound(onRequest);
  #if defined(ESP8266)
  ArduinoOTA.begin(&server);
  #elif defined(ESP32)
  ArduinoOTA.begin();
  #endif
  server.begin();

  client.setBufferSize(6000);

  // Start up the library
  if(userData.extension > 0){
    sensor1.begin();
    sensor2.begin();
    sensor3.begin();
    sensor4.begin();
    sensor5.begin();
    
    sensor1.setWaitForConversion(false);  // Request temperature conversion - non-blocking / async
    sensor2.setWaitForConversion(false);
    sensor3.setWaitForConversion(false);
    sensor4.setWaitForConversion(false);
    sensor5.setWaitForConversion(false);
  }
  if(userData.extension == 7){
    sensor6.begin();
    sensor7.begin();

    sensor6.setWaitForConversion(false);
    sensor7.setWaitForConversion(false);    
  }

  if(userData.extension == 5){
    // Vorbereitungen für den BME280
    bmeStatus = bme.begin(0x76);
  }
  mbusAddress[0] = userData.mbusAddress1;
  mbusAddress[1] = userData.mbusAddress2;
  mbusAddress[2] = userData.mbusAddress3;
}


void loop() {
  client.loop();  //MQTT Funktion
  ArduinoOTA.handle();
  if(apMode == true){
    dnsServer.processNextRequest();
  }

  if(apMode == true && millis() > 300000){
    ESP.restart();
  }

  if (!client.connected() && ((millis() - timerReconnect) > 5000)) { //espClient.connected() &&
    reconnect();
    timerReconnect = millis();
  }

  if(credentialsReceived == true && waitForRestart == false){
    EEPROM.begin(512);
    EEPROM.put(100, userData);
    credentialsSaved = 500;
    EEPROM.put(eeAddrCredentialsSaved, credentialsSaved);
    EEPROM.commit();
    EEPROM.end();
    timerReboot = millis();
    waitForRestart = true;
  }

  if(waitForRestart==true && (millis() - timerReboot) > 1500){
    ESP.restart();
  }

  if(newAddressReceived == true){
    newAddressReceived = false;
    waitToSetAddress = true;
    timerSetAddress = millis();
    mbus_normalize(254);
    client.publish(String(String(userData.mbusinoName) + "/setAddress/1").c_str(), "done");
  }

  if(waitToSetAddress == true && (millis() - 500) > timerSetAddress){
    waitToSetAddress = false;
    mbus_set_address(254,newAddress);
    client.publish(String(String(userData.mbusinoName) + "/setAddress/2").c_str(), String(newAddress).c_str());
  }

  /////////////////for debug///////////////////////////////////
  if((millis()-timerDebug) >10000){
    timerDebug = millis();
    client.publish(String(String(userData.mbusinoName) + "/settings/ssid").c_str(), userData.ssid); 
    //client.publish(String(String(userData.mbusinoName) + "/settings/password").c_str(), String(userData.password)); 
    client.publish(String(String(userData.mbusinoName) + "/settings/broker").c_str(), userData.broker); 
    client.publish(String(String(userData.mbusinoName) + "/settings/port").c_str(), String(userData.mqttPort).c_str()); 
    client.publish(String(String(userData.mbusinoName) + "/settings/user").c_str(), userData.mqttUser); 
    //client.publish(String(String(userData.mbusinoName) + "/settings/pswd").c_str(), userData.mqttPswrd); 
    client.publish(String(String(userData.mbusinoName) + "/settings/name").c_str(), userData.mbusinoName); 
    client.publish(String(String(userData.mbusinoName) + "/settings/extension").c_str(), String(userData.extension).c_str()); 
    client.publish(String(String(userData.mbusinoName) + "/settings/mbusInterval").c_str(), String(userData.mbusInterval).c_str()); 
    client.publish(String(String(userData.mbusinoName) + "/settings/sensorInterval").c_str(), String(userData.sensorInterval).c_str()); 
    client.publish(String(String(userData.mbusinoName) + "/settings/slaves").c_str(), String(userData.mbusSlaves).c_str());
    client.publish(String(String(userData.mbusinoName) + "/settings/address1").c_str(), String(userData.mbusAddress1).c_str());
    client.publish(String(String(userData.mbusinoName) + "/settings/address2").c_str(), String(userData.mbusAddress2).c_str());
    client.publish(String(String(userData.mbusinoName) + "/settings/address3").c_str(), String(userData.mbusAddress3).c_str());    
    client.publish(String(String(userData.mbusinoName) + "/settings/IP").c_str(), String(WiFi.localIP().toString()).c_str());
    client.publish(String(String(userData.mbusinoName) + "/settings/MQTTreconnections").c_str(), String(conCounter-1).c_str());
    long rssi = WiFi.RSSI();
    client.publish(String(String(userData.mbusinoName) + "/settings/RSSI").c_str(), String(rssi).c_str()); 
    client.publish(String(String(userData.mbusinoName) + "/settings/version").c_str(), MBUSINO_VERSION); 
    client.publish(String(String(userData.mbusinoName) + "/settings/adcounter").c_str(), String(adMbusMessageCounter).c_str());     
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
  if (millis() > (timerMQTT + userData.sensorInterval)) { //MQTT Nachrichten senden
    adSensorMessageCounter++;
    for(uint8_t i = 0; i < userData.extension; i++){
      if(OW[i] != -127){
        client.publish(String(String(userData.mbusinoName) + "/OneWire/S" + String(i+1)).c_str(), String(OWwO[i]).c_str());
        client.publish(String(String(userData.mbusinoName) + "/OneWire/offset" + String(i+1)).c_str(), String(offset[i]).c_str());
        if(userData.haAutodisc == true && adSensorMessageCounter == 3){
          haHandoverOw(i+1);
        }
      }      
    }
  
    if(userData.extension == 5){
      client.publish(String(String(userData.mbusinoName) + "/bme/temperature").c_str(), String(temperatur).c_str());
      client.publish(String(String(userData.mbusinoName) + "/bme/pressure").c_str(), String(druck).c_str());
      client.publish(String(String(userData.mbusinoName) + "/bme/altitude").c_str(), String(hoehe).c_str());
      client.publish(String(String(userData.mbusinoName) + "/bme/humidity").c_str(), String(feuchte).c_str());
      if(userData.haAutodisc == true && adSensorMessageCounter == 3){
        haHandoverBME();
      }      
    }
     timerMQTT = millis();
  }
////////// M- Bus ###############################################
/*
mbusLoopStatus
0 = ready
1 = mbus cleared
2 = records requested
3 = records received


*/

  if(millis() - timerMbus > userData.mbusInterval && mbusLoopStatus == 0){ // Normalize the M-Bus 
    timerMbus = millis();  
    mbusLoopStatus = 1;
    if(addressCounter >= userData.mbusSlaves){
      addressCounter = 0;
    }
    currentAddress = mbusAddress[addressCounter];
    addressCounter++;
    mbus_normalize(currentAddress);
  }

  if(millis() - timerMbus > 500 && mbusLoopStatus == 1){ // Request M-Bus Records
    mbusLoopStatus = 2;
    mbus_clearRXbuffer();
    mbus_request_data(currentAddress);
  }
  if(millis() - timerMbus > 2000 && mbusLoopStatus == 2){ // Receive and decode M-Bus Records
    mbusLoopStatus = 3;
    bool mbus_good_frame = false;
    byte mbus_data[MBUS_DATA_SIZE] = { 0 };
    mbus_good_frame = mbus_get_response(mbus_data, sizeof(mbus_data));

    /*
    //------------------ only for debug, you will recieve the whole M-Bus telegram bytewise in HEX for analysis -----------------
    for(uint8_t i = 0; i <= mbus_data[1]+1; i++){                                                             //|
      char buffer[3];                                                                                         //|
      sprintf(buffer,"%02X",mbus_data[i]);                                                                    //|
      client.publish(String(String(userData.mbusinoName) + "/debug/telegram_byte_"+String(i)).c_str(), String(buffer).c_str());          //|
    }                                                                                                         //|
    //--------------------------------------------------------------------------------------------------------------------------    
    */
    //mbus_good_frame = true;
    //byte mbus_data[] = {0x68,0xC1,0xC1,0x68,0x08,0x00,0x72,0x09,0x34,0x75,0x73,0xC5,0x14,0x00,0x0D,0x43,0x00,0x00,0x00,0x04,0x78,0x41,0x63,0x65,0x04,0x04,0x06,0xAA,0x29,0x00,0x00,0x04,0x13,0x40,0xA1,0x75,0x00,0x04,0x2B,0x00,0x00,0x00,0x00,0x14,0x2B,0x3C,0xF3,0x00,0x00,0x04,0x3B,0x48,0x06,0x00,0x00,0x14,0x3B,0x4E,0x0E,0x00,0x00,0x02,0x5B,0x19,0x00,0x02,0x5F,0x19,0x00,0x02,0x61,0xFA,0xFF,0x02,0x23,0xAC,0x08,0x04,0x6D,0x03,0x2A,0xF1,0x2A,0x44,0x06,0x92,0x0C,0x00,0x00,0x44,0x13,0x2D,0x9B,0x1C,0x00,0x42,0x6C,0xDF,0x2C,0x01,0xFD,0x17,0x00,0x03,0xFD,0x0C,0x05,0x00,0x00,0x84,0x10,0x06,0x1A,0x00,0x00,0x00,0xC4,0x10,0x06,0x05,0x00,0x00,0x00,0x84,0x20,0x06,0x00,0x00,0x00,0x00,0xC4,0x20,0x06,0x00,0x00,0x00,0x00,0x84,0x30,0x06,0x00,0x00,0x00,0x00,0xC4,0x30,0x06,0x00,0x00,0x00,0x00,0x84,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0x40,0x13,0x00,0x00,0x00,0x00,0x84,0x80,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0x80,0x40,0x13,0x00,0x00,0x00,0x00,0x84,0xC0,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0xC0,0x40,0x13,0x00,0x00,0x00,0x00,0x75,0x16};


    if (mbus_good_frame) {
      if(addressCounter == 1){
        adMbusMessageCounter++;
      }
      int packet_size = mbus_data[1] + 6; 
      JsonDocument jsonBuffer;
      JsonArray root = jsonBuffer.add<JsonArray>();  
      fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 
      address = mbus_data[5]; 
      serializeJson(root, jsonstring);
      client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/error").c_str(), String(payload.getError()).c_str());  // kann auskommentiert werden wenn es läuft
      client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/jsonstring").c_str(), jsonstring);      
      if(mbus_data[12]==0x14&&mbus_data[11]==0xC5){
        engelmann = true;
      }
      else{
        engelmann = false;
      }
    }
    else {  //Fehlermeldung
        mbusLoopStatus = 0;
        jsonstring[0] = 0;
        client.publish(String(String(userData.mbusinoName)  +"/MBus/SlaveAddress"+String(currentAddress)+ "/MBUSerror").c_str(), "no_good_telegram");
    }
    mbus_normalize(currentAddress);
  } 
  if(millis() - timerMbus > 2500 && mbusLoopStatus == 3){  // Send decoded M-Bus secords via MQTT
    mbusLoopStatus = 0;
    JsonDocument root;
    deserializeJson(root, jsonstring); // load the json from a global array
    jsonstring[0] = 0;

    for (uint8_t i=0; i<fields; i++) {
      uint8_t code = root[i]["code"].as<int>();
      const char* name = root[i]["name"];
      const char* units = root[i]["units"];           
      double value = root[i]["value_scaled"].as<double>();
      const char* valueString = root[i]["value_string"];     

      if(userData.haAutodisc == true && adMbusMessageCounter == 3){  //every 264 message is a HA autoconfig message
        strcpy(adVariables.haName,name);
        if(units != NULL){
          strcpy(adVariables.haUnits,units);
        }else{
          strcpy(adVariables.haUnits,""); 
        }
        strcpy(adVariables.stateClass,payload.getStateClass(code));
        strcpy(adVariables.deviceClass,payload.getDeviceClass(code));     
        haHandoverMbus(i+1, engelmann, address);
      }else{               

        //two messages per value, values comes as number or as ASCII string or both
        client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/"+String(i+1)+"_vs_"+String(name)).c_str(), valueString); // send the value if a ascii value is aviable (variable length)
        client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/"+String(i+1)+"_"+String(name)).c_str(), String(value,3).c_str()); // send the value if a real value is aviable (standard)
        client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/"+String(i+1)+"_"+String(name)+"_unit").c_str(), units);
        //or only one message
        //client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/MBus/"+String(i+1)+"_"+String(name)+"_in_"+String(units)), String(value,3).c_str());

        if (i == 3 && engelmann == true){  // Sensostar Bugfix --> comment it out if you use not a Sensostar
          float flow = root[5]["value_scaled"].as<float>();
          float delta = root[9]["value_scaled"].as<float>();
          float calc_power = delta * flow * 1163;          
          client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/4_power_calc").c_str(), String(calc_power).c_str());                    
        } 
      }      
    }
    address = 0; 
  } 
}




 


