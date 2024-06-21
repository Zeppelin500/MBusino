/*
# MBusino: M-Bus/OneWire/I²C --> MQTT-Gateway with a shield for ESP8266 D1 mini or ESP32 S2 mini

https://github.com/Zeppelin500/MBusino/

## Licence
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/

#include <Arduino.h>
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


#define MBUSINO_VERSION "0.9.8"

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
PubSubClient client(espClient);
DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)

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
  bool haAutodisc;
} userData = {"SSID","Password","MBusino","192.168.1.7",1883,5,"mqttUser","mqttPasword",5000,120000,true};

bool mqttcon = false;
bool apMode = false;
bool credentialsReceived = false;
uint16_t conCounter = 0;

int Startadd = 0x13;  // Start address for decoding

float OW[7] = {0};         // variables for DS18B20 Onewire sensors 
float temperatur = 0;      // Variablen für den BLE280 am I2C
float druck = 0;
float hoehe = 0;
float feuchte = 0;
bool bmeStatus;

uint8_t mbusLoopStatus = 0;
uint8_t fields = 0;
char jsonstring[3074] = { 0 };
bool engelmann = false;
bool waitForRestart = false;

unsigned long timerMQTT = 15000;
unsigned long timerSensorRefresh1 = 0;
unsigned long timerSensorRefresh2 = 0;
unsigned long timerMbus = 0;
unsigned long timerDebug = 0;
unsigned long timerReconnect = 0;
unsigned long timerReboot = 0;
unsigned long timerAutodiscover = 0;

void mbus_request_data(byte address);
void mbus_short_frame(byte address, byte C_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);
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
#include "mbus.h"
#include "html.h"
#include "calibration.h"
#include "sensorRefresh.h"
#include "mqtt.h"
#include "server.h"
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

  sprintf(html_buffer, index_html,userData.ssid,userData.mbusinoName,userData.extension,userData.haAutodisc,userData.sensorInterval/1000,userData.mbusInterval/1000,userData.broker,userData.mqttPort,userData.mqttUser);

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
    //request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
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

  if(userData.extension == 5){
    // Vorbereitungen für den BME280
    bmeStatus = bme.begin(0x76);
  }

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

  if(waitForRestart==true && (millis() - timerReboot) > 1000){
    ESP.restart();
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
    client.publish(String(String(userData.mbusinoName) + "/settings/IP").c_str(), String(WiFi.localIP().toString()).c_str());
    client.publish(String(String(userData.mbusinoName) + "/settings/MQTTreconnections").c_str(), String(conCounter-1).c_str());
    long rssi = WiFi.RSSI();
    client.publish(String(String(userData.mbusinoName) + "/settings/RSSI").c_str(), String(rssi).c_str()); 
    client.publish(String(String(userData.mbusinoName) + "/settings/version").c_str(), MBUSINO_VERSION); 
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
      //OW[i] = 33.7; //simulation
      //OWwO[i] = 34.5;
      //offset[i]= 0.8;
      if(OW[i] != -127){        
        client.publish(String(String(userData.mbusinoName) + "/OneWire/S" + String(i+1)).c_str(), String(OWwO[i]).c_str());
        client.publish(String(String(userData.mbusinoName) + "/OneWire/offset" + String(i+1)).c_str(), String(offset[i]).c_str());
        if(userData.haAutodisc == true && adSensorMessageCounter == 3){
          haHandoverOw(i+1);
        }
       
      }      
    }
  
    if(userData.extension == 5){
      client.publish(String(String(userData.mbusinoName) + "/bme/Temperatur").c_str(), String(temperatur).c_str());
      client.publish(String(String(userData.mbusinoName) + "/bme/Druck").c_str(), String(druck).c_str());
      client.publish(String(String(userData.mbusinoName) + "/bme/Hoehe").c_str(), String(hoehe).c_str());
      client.publish(String(String(userData.mbusinoName) + "/bme/Feuchte").c_str(), String(feuchte).c_str());
      if(userData.haAutodisc == true && adSensorMessageCounter == 3){
        haHandoverBME();
      }
    }
    timerMQTT = millis();
  }
////////// M- Bus ###############################################

  if(millis() - timerMbus > userData.mbusInterval && mbusLoopStatus == 0){ // Request M-Bus Records
    timerMbus = millis();
    mbusLoopStatus = 1;
    mbus_request_data(MBUS_ADDRESS);
  }
  if(millis() - timerMbus > 1000 && mbusLoopStatus == 1){ // Receive and decode M-Bus Records
    mbusLoopStatus = 2;
    bool mbus_good_frame = false;
    byte mbus_data[MBUS_DATA_SIZE] = { 0 };
    mbus_good_frame = mbus_get_response(mbus_data, sizeof(mbus_data));

    /*
    //------------------ only for debug, you will recieve the whole M-Bus telegram bytewise in HEX for analysis -----------------
    for(uint8_t i = 0; i <= mbus_data[1]+6; i++){                                                             //|
      char buffer[3];                                                                                         //|
      sprintf(buffer,"%02X",mbus_data[i]);                                                                    //|
      client.publish(String(String(userData.mbusinoName) + "/debug/telegram_byte_"+String(i)).c_str(), String(buffer).c_str());          //|
    }                                                                                                         //|
    //--------------------------------------------------------------------------------------------------------------------------    
    */
    //bool mbus_good_frame = true;
    //byte mbus_data[] = {0x68,0x9E,0x9E,0x68,0x08,0x65,0x72,0x09,0x76,0x06,0x00,0xA5,0x25,0x1D,0x02,0x02,0x00,0x00,0x00,0x85,0x40,0xAB,0xFF,0x01,0x00,0x36,0x0B,0x47,0x85,0x40,0xAB,0xFF,0x02,0x00,0x2C,0xFA,0x46,0x85,0x40,0xAB,0xFF,0x03,0x00,0x74,0xED,0x46,0x85,0x80,0x40,0xAB,0xFF,0x01,0x00,0xC0,0xE2,0x44,0x85,0x80,0x40,0xAB,0xFF,0x02,0x00,0x40,0x5A,0x45,0x85,0x80,0x40,0xAB,0xFF,0x03,0x00,0x60,0x36,0x45,0x05,0xFD,0xBA,0xFF,0x01,0x78,0xBE,0x7F,0x3F,0x05,0xFD,0xBA,0xFF,0x02,0x40,0x35,0x7E,0x3F,0x05,0xFD,0xBA,0xFF,0x03,0x53,0xB8,0x7E,0x3F,0x05,0xFD,0xC8,0xFF,0x04,0x00,0x90,0x7A,0x45,0x05,0xFD,0xC8,0xFF,0x05,0x00,0x70,0x7B,0x45,0x05,0xFD,0xC8,0xFF,0x06,0x00,0x80,0x7B,0x45,0x05,0xFD,0xD9,0xFF,0x04,0x00,0x50,0x2A,0x47,0x05,0xFF,0x5A,0x00,0x00,0xFA,0x43,0x02,0xFD,0x3A,0xC8,0x00,0x02,0xFD,0x3A,0x0A,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x8B,0x16};
    
    if (mbus_good_frame) {
      adMbusMessageCounter++;
      int packet_size = mbus_data[1] + 6;   
      JsonDocument jsonBuffer;
      JsonArray root = jsonBuffer.add<JsonArray>();  
      fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 
      serializeJson(root, jsonstring); // store the json in a global array
      // test -----------------------------------------------------------------------------------------
      uint16_t arraycounter = 0;
      uint8_t findTheTerminator = 1;
      while(findTheTerminator != 0){
        findTheTerminator = jsonstring[arraycounter];
        arraycounter++;  
      }
      client.publish(String(String(userData.mbusinoName) + "/MBus/jsonlen").c_str(), String(arraycounter).c_str());  
      // test ende -----------------------------------------------------------------------------------------
      client.publish(String(String(userData.mbusinoName) + "/MBus/error").c_str(), String(payload.getError()).c_str());  // kann auskommentiert werden wenn es läuft
      client.publish(String(String(userData.mbusinoName) + "/MBus/jsonstring").c_str(), jsonstring);
      uint8_t address = mbus_data[5];
      client.publish(String(String(userData.mbusinoName) + "/MBus/address").c_str(), String(address).c_str());   
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
        client.publish(String(String(userData.mbusinoName) + "/MBUSerror").c_str(), "no_good_telegram");
    }
  }
  if(millis() - timerMbus > 2000 && mbusLoopStatus == 2){  // Send decoded M-Bus secords via MQTT
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
        haHandoverMbus(i+1, engelmann);
      }else{
        //two messages per value, values comes as number or as ASCII string or both
        client.publish(String(String(userData.mbusinoName) + "/MBus/"+String(i+1)+"_vs_"+String(name)).c_str(), valueString); // send the value if a ascii value is aviable (variable length)
        client.publish(String(String(userData.mbusinoName) + "/MBus/"+String(i+1)+"_"+String(name)).c_str(), String(value,3).c_str()); // send the value if a real value is aviable (standard)
        client.publish(String(String(userData.mbusinoName) + "/MBus/"+String(i+1)+"_"+String(name)+"_unit").c_str(), units);
        //or only one message
        //client.publish(String(String(userData.mbusinoName) + "/MBus/"+String(i+1)+"_"+String(name)+"_in_"+String(units)), String(value,3).c_str());

        if (i == 3 && engelmann == true){  // Sensostar Bugfix --> comment it out if you use not a Sensostar
          float flow = root[5]["value_scaled"].as<float>();
          float delta = root[9]["value_scaled"].as<float>();
          float calc_power = delta * flow * 1163;          
          client.publish(String(String(userData.mbusinoName) + "/MBus/4_power_calc").c_str(), String(calc_power).c_str());                    
        }   
      } 
    }
  }
}




