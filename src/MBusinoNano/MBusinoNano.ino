/*
# MBusino Nano: M-Bus --> MQTT-Gateway via Ethernet (or Wifi)
Only M-Bus, no accessories.

Ethernet is prioritized over wifi. MQTT connection is possible with both. If a connection get lost, MBusino will try the other one during ongoing operation.
If no known wifi is found when starting, but ethernet is connected, wifi will be switched off completely. Even if there is no Ethernet connection, 
an AP with a captive portal is set up.


https://github.com/Zeppelin500/MBusino/

## Licence
****************************************************
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************
*/

#include <SPI.h>
#include <ETH.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ESPAsyncWebServer.h> 
#include <DNSServer.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <MBusinoLib.h>  // Library for decode M-Bus
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <MBusCom.h>

#define MBUSINO_VERSION "0.9.21"

#define MBUS_ADDRESS 254


// Pins for an ESP32 C3 Supermini standard SPI
#ifndef ETH_PHY_CS
#define ETH_PHY_TYPE     ETH_PHY_W5500
#define ETH_PHY_ADDR     0
#define ETH_PHY_CS       7
#define ETH_PHY_IRQ      2
#define ETH_PHY_RST      3
#define ETH_PHY_SPI_HOST SPI2_HOST
#define ETH_PHY_SPI_SCK  4
#define ETH_PHY_SPI_MISO 5
#define ETH_PHY_SPI_MOSI 6
#endif

/*
// Pins for an ESP32 C3 Supermini V01 SPI
#ifndef ETH_PHY_CS
#define ETH_PHY_TYPE     ETH_PHY_W5500
#define ETH_PHY_ADDR     1
#define ETH_PHY_CS       7
#define ETH_PHY_IRQ      2
#define ETH_PHY_RST      3
#define ETH_PHY_SPI_HOST SPI2_HOST
#define ETH_PHY_SPI_SCK  8
#define ETH_PHY_SPI_MISO 9
#define ETH_PHY_SPI_MOSI 10
#endif
*/
static bool eth_connected = false;

NetworkClient ethClient;
HardwareSerial MbusSerial(1);
MBusinoLib payload(254);
WiFiClient wfClient;
PubSubClient client;
DNSServer dnsServer;
AsyncWebServer server(80);
MBusCom mbus(&MbusSerial,20,21);

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
  bool telegramDebug;
} userData = {"SSID","Password","MBusino","192.168.1.8",1883,5,"mqttUser","mqttPasword",5000,120000,true,false};

bool mqttcon = false;
bool apMode = false;
bool credentialsReceived = false;
uint16_t conCounter = 0;

int Startadd = 0x13;  // Start address for decoding

uint8_t mbusLoopStatus = 0;
uint8_t fields = 0;
bool fcb = 0; // M-Bus Frame Count Bit
char jsonstring[4096] = { 0 };
bool engelmann = false;
bool waitForRestart = false;
bool polling = false;
bool wifiReconnect = false;
uint8_t usedMQTTconection = 0;
bool networkLost = false;
bool gotIP = false;

unsigned long timerMQTT = 15000;
unsigned long timerMbus = 0;
unsigned long timerDebug = 0;
unsigned long timerReconnect = 0;
unsigned long timerWifiReconnect = 0;
unsigned long timerReboot = 0;
unsigned long timerAutodiscover = 0;
unsigned long timerNetworkChange = 0;
unsigned long timerETHmessage = 0;

void setupServer();

uint8_t eeAddrCalibrated = 0;
uint8_t eeAddrCredentialsSaved = 32;
uint16_t credentialsSaved = 123;  // shows if EEPROM used befor for credentials

uint8_t adMbusMessageCounter = 0; // Counter for autodiscouver mbus message.

uint32_t minFreeHeap = 0;

//outsourced program parts
#include "networkEvents.h"
#include "html.h"
#include "mqtt.h"
#include "guiServer.h"
#include "autodiscover.h"

void heapCalc();

void setup() {

  pinMode(LED_BUILTIN, OUTPUT); // LED on if MQTT connectet to server
  digitalWrite(LED_BUILTIN,HIGH);
  Serial.begin(115200);
  mbus.begin();

  //Serial.setDebugOutput(true);

  minFreeHeap = ESP.getFreeHeap();

  EEPROM.begin(512);
  EEPROM.get(eeAddrCredentialsSaved, credentialsSaved);
  if(credentialsSaved == 500){
    EEPROM.get(100, userData );
  }
  EEPROM.commit();
  EEPROM.end();

  if(userData.telegramDebug > 1){
    userData.telegramDebug = 0;
  }

  sprintf(html_buffer, index_html,userData.ssid,userData.mbusinoName,userData.haAutodisc,userData.telegramDebug,userData.mbusInterval/1000,userData.broker,userData.mqttPort,userData.mqttUser);
  
  WiFi.onEvent(WiFiEvent);
  WiFi.hostname(userData.mbusinoName);
  //WiFi.hostname(String(String(userData.mbusinoName)+"-WiFi").c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(userData.ssid, userData.password);  
  
  Network.onEvent(onEvent);
  delay(100);
  //ETH.setLinkSpeed(10);
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
  //ETH.setLinkSpeed(10);
  delay(2000);

  byte tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (tries++ > 2) {
      if(eth_connected == false){
        WiFi.mode(WIFI_AP);
        WiFi.softAP("MBusino Setup Portal"); //, "secret");
        apMode = true;
        break;
      }
      else{
        WiFi.mode(WIFI_OFF);
        Serial.println("WIFI set to off, no known Network and Ethernet is already connected");
        break;
      }
    }
  }

  if(eth_connected == true){
    client.setClient(ethClient);
    usedMQTTconection = 1;
    Serial.println("MQTT set connection via Ethernet");
  }
  else{
    client.setClient(wfClient);
    usedMQTTconection = 2;
    Serial.println("MQTT set connection via Wifi");
  }

  client.setServer(userData.broker, userData.mqttPort);
  client.setCallback(callback);

  setupServer();
  if(apMode==true){
    dnsServer.start(53, "*", WiFi.softAPIP());
  }
    
  setupServer();
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP


  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html",  (uint8_t *)update_html, update_htmlLength);  
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
        Serial.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
      }
    }
  });

  ArduinoOTA.setPassword((const char *)"mbusino");
  server.onNotFound(onRequest);
  ArduinoOTA.begin();
  server.begin();

  client.setBufferSize(6000);
}


void loop() {
  //delay(50);
  heapCalc();

  ArduinoOTA.handle();


  if(gotIP == false && eth_connected == true && (millis() - timerETHmessage > 2000) && waitForRestart == false){
    Serial.println("no IP received, restart ETH");
    timerETHmessage = millis();
    ETH.end();
    delay(200);
    //SPI.end();
    delay(200);
    ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI,16);
  }
/*
  if(gotIP == false && eth_connected == true && (millis() - timerETHmessage > 1000) && waitForRestart == false){
    Serial.println("no IP received, restart soon");
    timerReboot = millis();
    waitForRestart = true;
  }

*/

  if(wifiReconnect == true && (millis() - timerWifiReconnect > 2000)){
    Serial.println("try to reconnect"); 
    wifiReconnect = false;
    WiFi.reconnect();
  }


  if(credentialsReceived == true && waitForRestart == false){
    Serial.println("credentials received, save and restart soon");
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
    Serial.println("restart");
    ESP.restart();
  }

  if((apMode == true && eth_connected == false) && millis() > 300000){
    ESP.restart();
  }

  if(apMode == true){
    dnsServer.processNextRequest();
  }

  //to notice Changes in Network an assign the right connection to the MQTT client
  if((eth_connected == false) && (usedMQTTconection == 1) && (millis()-timerNetworkChange) > 5000){
    client.setClient(wfClient);
    usedMQTTconection = 2;
    Serial.println("MQTT set connection via Wifi");
    reconnect();
    timerNetworkChange = millis();
  }
  if((eth_connected == true) && (usedMQTTconection == 2) && (millis()-timerNetworkChange) > 5000){
    client.setClient(ethClient);
    usedMQTTconection = 1;
    Serial.println("MQTT set connection via Ethernet");
    reconnect();
    timerNetworkChange = millis();
  }

  if (!client.connected() && ((millis() - timerReconnect) > 5000)) {  
    Serial.println("MQTT no connection");      
    reconnect();
    timerReconnect = millis();
  }
  else{ // the whole main code run only if MQTT is connectet
    client.loop();  //MQTT Funktion

    ///////////////// publish settings ///////////////////////////////////
    if((millis()-timerDebug) >10000){
      timerDebug = millis();

      client.publish(String(String(userData.mbusinoName) + "/settings/wl_Connected").c_str(), String(WL_CONNECTED).c_str());    
      client.publish(String(String(userData.mbusinoName) + "/settings/apMode").c_str(), String(apMode).c_str());    
      client.publish(String(String(userData.mbusinoName) + "/settings/eth_connected").c_str(), String(eth_connected).c_str());    

      client.publish(String(String(userData.mbusinoName) + "/settings/ssid").c_str(), userData.ssid); 
      //client.publish(String(String(userData.mbusinoName) + "/settings/password").c_str(), String(userData.password)); 
      client.publish(String(String(userData.mbusinoName) + "/settings/broker").c_str(), userData.broker); 
      client.publish(String(String(userData.mbusinoName) + "/settings/port").c_str(), String(userData.mqttPort).c_str()); 
      client.publish(String(String(userData.mbusinoName) + "/settings/user").c_str(), userData.mqttUser); 
      //client.publish(String(String(userData.mbusinoName) + "/settings/pswd").c_str(), userData.mqttPswrd); 
      client.publish(String(String(userData.mbusinoName) + "/settings/name").c_str(), userData.mbusinoName); 
      client.publish(String(String(userData.mbusinoName) + "/settings/mbusInterval").c_str(), String(userData.mbusInterval).c_str());    
      client.publish(String(String(userData.mbusinoName) + "/settings/wifiIP").c_str(), String(WiFi.localIP().toString()).c_str());
      client.publish(String(String(userData.mbusinoName) + "/settings/ethIP").c_str(), String(ETH.localIP().toString()).c_str());
      client.publish(String(String(userData.mbusinoName) + "/settings/MQTTreconnections").c_str(), String(conCounter-1).c_str());
      if(usedMQTTconection == 1){
        client.publish(String(String(userData.mbusinoName) + "/settings/MQTTconnectedVia").c_str(), "ethernet");
      }
      if(usedMQTTconection == 2){
        client.publish(String(String(userData.mbusinoName) + "/settings/MQTTconnectedVia").c_str(), "WiFi");
      }
      long rssi = WiFi.RSSI();
      client.publish(String(String(userData.mbusinoName) + "/settings/RSSI").c_str(), String(rssi).c_str()); 
      client.publish(String(String(userData.mbusinoName) + "/settings/version").c_str(), MBUSINO_VERSION);  
      client.publish(String(String(userData.mbusinoName) + "/settings/freeHeap").c_str(), String(ESP.getFreeHeap()).c_str()); 
      client.publish(String(String(userData.mbusinoName) + "/settings/minFreeHeap").c_str(), String(minFreeHeap).c_str()); 
    }

  ////////// M- Bus ###############################################

    if((millis() - timerMbus > userData.mbusInterval || polling == true) && mbusLoopStatus == 0){ // Request M-Bus Records
      timerMbus = millis();
      polling = false;
      mbusLoopStatus = 1;
      mbus.request_data(MBUS_ADDRESS, fcb);
    }
    if(millis() - timerMbus > 1500 && mbusLoopStatus == 1){ // Receive and decode M-Bus Records
      mbusLoopStatus = 2;
      bool mbus_good_frame = false;
      byte mbus_data[MBUS_DATA_SIZE] = { 0 };
      mbus_good_frame = mbus.get_response(mbus_data, sizeof(mbus_data));

      //bool mbus_good_frame = true;
      //byte mbus_data[] = {0x68,0x9E,0x9E,0x68,0x08,0x65,0x72,0x09,0x76,0x06,0x00,0xA5,0x25,0x1D,0x02,0x02,0x00,0x00,0x00,0x85,0x40,0xAB,0xFF,0x01,0x00,0x36,0x0B,0x47,0x85,0x40,0xAB,0xFF,0x02,0x00,0x2C,0xFA,0x46,0x85,0x40,0xAB,0xFF,0x03,0x00,0x74,0xED,0x46,0x85,0x80,0x40,0xAB,0xFF,0x01,0x00,0xC0,0xE2,0x44,0x85,0x80,0x40,0xAB,0xFF,0x02,0x00,0x40,0x5A,0x45,0x85,0x80,0x40,0xAB,0xFF,0x03,0x00,0x60,0x36,0x45,0x05,0xFD,0xBA,0xFF,0x01,0x78,0xBE,0x7F,0x3F,0x05,0xFD,0xBA,0xFF,0x02,0x40,0x35,0x7E,0x3F,0x05,0xFD,0xBA,0xFF,0x03,0x53,0xB8,0x7E,0x3F,0x05,0xFD,0xC8,0xFF,0x04,0x00,0x90,0x7A,0x45,0x05,0xFD,0xC8,0xFF,0x05,0x00,0x70,0x7B,0x45,0x05,0xFD,0xC8,0xFF,0x06,0x00,0x80,0x7B,0x45,0x05,0xFD,0xD9,0xFF,0x04,0x00,0x50,0x2A,0x47,0x05,0xFF,0x5A,0x00,0x00,0xFA,0x43,0x02,0xFD,0x3A,0xC8,0x00,0x02,0xFD,0x3A,0x0A,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x8B,0x16};
      

      if(userData.telegramDebug == true){
      //------------------ only for debug, you will recieve the whole M-Bus telegram bytewise in HEX for analysis -----------------
        //char telegram[(mbus_data[1]+6)*2] = {0};
        char telegram[520] = {0};
        for(uint8_t i = 0; i <= mbus_data[1]+6; i++){                                                             //|
          char buffer[3];                                                                                         //|
          sprintf(buffer,"%02X",mbus_data[i]);                                                                    //|
          client.publish(String(String(userData.mbusinoName) + "/debug/telegram_byte_"+String(i)).c_str(), String(buffer).c_str());  
          telegram[i*2] = buffer[0];
          telegram[(i*2)+1] = buffer[1];       //|
        } 
        //client.publish(String(String(userData.mbusinoName) + "/debug/telegram"), String(telegram).c_str());    
        client.publish(String(String(userData.mbusinoName) +"/debug/telegram").c_str(), telegram);                                                                                                       //|
        //--------------------------------------------------------------------------------------------------------------------------    
      }
      if (mbus_good_frame) {
        if(fcb == true){ // toggle the FCB (Frame Count Bit) to signalize good response in the next request
          fcb = false;
        }
        else{
          fcb = true;
        }
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
        heapCalc();
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
    if(millis() - timerMbus > 2500 && mbusLoopStatus == 2){  // Send decoded M-Bus secords via MQTT
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
    heapCalc();
  }

}

void heapCalc(){
  if(minFreeHeap > ESP.getFreeHeap()){
    minFreeHeap = ESP.getFreeHeap();
  }
}

