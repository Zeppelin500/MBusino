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

#define MBUSINO_VERSION "1.2.0"

// EEPROM flag constants
#define EEPROM_CREDENTIALS_OLD 500
#define EEPROM_CREDENTIALS_NEW 501

#define MBUS_ADDRESS 254

#define EEPROM_ETH_LINK_MODE 40 // last working Ethernet link mode, 1 = forced 10M full duplex, otherwise auto negotiation
#define EEPROM_ETH_MODE_SETTING 41 // Ethernet link mode chosen in the web interface, see ETH_MODE_...
#define ETH_DHCP_TIMEOUT 15000 // ms after link up without IP before the other link mode is tried
#define ETH_LINK_TIMEOUT 8000 // ms after ETH.begin() without link up before the other link mode is tried. Link up is decided at PHY level well below 1 s, so 8 s is already a generous margin; the previous 20 s only delayed the fallback for strict equipment (Fritzbox 7590, PoE extenders)
#define ETH_LOSS_GRACE 15000 // ms an already working Ethernet connection (IP) may stay away before a controlled restart; the boot then re-picks the transport (Ethernet -> Wi-Fi -> setup AP)

#define ETH_MODE_AUTO 0      // try auto negotiation and forced 10M full duplex, remember the working one
#define ETH_MODE_AUTONEG 1   // always auto negotiation
#define ETH_MODE_FORCED10M 2 // always forced 10M full duplex


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
  char ssid[65];
  char password[65];
  char mbusinoName[31];
  char broker[65];
  uint16_t mqttPort;
  uint16_t extension;
  char mqttUser[65];
  char mqttPswrd[65]; 
  uint32_t sensorInterval;
  uint32_t mbusInterval; 
  uint8_t mbusSlaves;
  uint8_t mbusAddress1;
  uint8_t mbusAddress2;
  uint8_t mbusAddress3;
  uint8_t mbusAddress4;
  uint8_t mbusAddress5;
  bool haAutodisc;
  bool telegramDebug;
} userData = {"SSID","Password","MBusino","192.168.1.8",1883,5,"mqttUser","mqttPassword",5000,120000,1,0xFE,0,0,0,0,true,false};

struct oldSettings {
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
  uint8_t mbusAddress4;
  uint8_t mbusAddress5;
  bool haAutodisc;
  bool telegramDebug;
} oldUserData = {"SSID","Password","MBusino","192.168.1.8",1883,5,"mqttUser","mqttPassword",5000,120000,1,0xFE,0,0,0,0,true,false};

uint8_t mbusAddress[5] = {0};

bool mqttcon = false;
bool apMode = false;
bool credentialsReceived = false;
uint16_t conCounter = 0;

uint8_t newAddress = 0;
bool newAddressReceived = false;
bool waitToSetAddress = false;

uint8_t currentAddress = 0;
uint8_t addressCounter = 0;
uint8_t currentAddressCounter = 0;
uint8_t pollingAddress = 0;

int Startadd = 0x13;  // Start address for decoding

uint8_t mbusLoopStatus = 0;
bool shc = true; //Switch Has Changed
uint8_t fields = 0;
bool fcb[5] = {0}; // M-Bus Frame Count Bit
bool initializeSlave[5] = {true}; // m-bus normalizing is needed
bool firstrun = true;
uint8_t recordCounter[5] = {0}; // count the received records for multible telegrams
char jsonstring[4096] = { 0 };
uint8_t address = 0; 
bool engelmann = false;
bool waitForRestart = false;
bool polling = false;
bool mtPolling = false;
bool wifiReconnect = false;
uint8_t usedMQTTconection = 0;
bool networkLost = false;
bool gotIP = false;
bool ethForced10M = false; // false = auto negotiation, true = forced 10M full duplex (needed by some strict equipment, e.g. Fritzbox 7590)
bool ethForced10MStored = false; // link mode saved in EEPROM
bool ethLinkSeen = false; // link was up at least once since the last ETH.begin()
bool ethModeConfirmed = false; // got an IP with the current link mode since start
uint8_t ethModeSetting = ETH_MODE_AUTO;
unsigned long timerETHbegin = 0;
unsigned long ethLostSince = 0; // millis() when an established Ethernet connection (IP) was lost, 0 = not lost; used for the restart grace period in loop()
unsigned long ethRestarting = 0; // millis() when loop() intentionally restarted ETH for a link mode change, 0 = not restarting; suppresses transport switching while ETH re-initialises

unsigned long timerMQTT = 15000;
unsigned long timerMbus = 0;
unsigned long timerInitialize = 0;
unsigned long timerSerialAvailable = 0;
unsigned long timerMbusDecoded = 0;
unsigned long timerMbusReq = 0;
unsigned long timerDebug = 0;
unsigned long timerReconnect = 0;
unsigned long timerWifiReconnect = 0;
unsigned long timerReboot = 0;
unsigned long timerSetAddress = 0;
unsigned long timerAutodiscover = 0;
unsigned long timerNetworkChange = 0;
unsigned long timerETHmessage = 0;
unsigned long timerLedAPmode = 0;

void setupServer();

uint8_t eeAddrCalibrated = 0;
uint8_t eeAddrCredentialsSaved = 32;
uint16_t credentialsSaved = 123;  // shows if EEPROM used befor for credentials

uint8_t adMbusMessageCounter = 0; // Counter for autodiscouver mbus message.

uint32_t minFreeHeap = 0;

//outsourced program parts
#include "networkEvents.h"
#include "html.h"
#include "mbusDebug.h"
#include "mqtt.h"
#include "guiServer.h"
#include "autodiscover.h"

void heapCalc();

// apply the Ethernet link mode, must be called before every ETH.begin()
void setEthLinkMode(){
  ETH.setAutoNegotiation(!ethForced10M);
  if(ethForced10M == true){
    ETH.setFullDuplex(true);
    ETH.setLinkSpeed(10);
  }
  ethLinkSeen = false;
  timerETHbegin = millis();
  Serial.printf("[ETH] %lu ms: link mode applied: %s\n", millis(), ethForced10M ? "forced 10M full duplex" : "auto negotiation");
}

void setup() {

  pinMode(LED_BUILTIN, OUTPUT); // LED on if MQTT connectet to server
  digitalWrite(LED_BUILTIN,HIGH);
  Serial.begin(115200);
  mbus.begin();

  //Serial.setDebugOutput(true);

  minFreeHeap = ESP.getFreeHeap();

  EEPROM.begin(512);
  EEPROM.get(eeAddrCredentialsSaved, credentialsSaved);
  if(credentialsSaved == EEPROM_CREDENTIALS_OLD){  // Old version before 1.0 (size of some variables has changed)
    EEPROM.get(100, oldUserData );
    strncpy(userData.ssid, oldUserData.ssid, sizeof(userData.ssid) - 1);
    strncpy(userData.password, oldUserData.password, sizeof(userData.password) - 1);
    strncpy(userData.mbusinoName, oldUserData.mbusinoName, sizeof(userData.mbusinoName) - 1);
    strncpy(userData.broker, oldUserData.broker, sizeof(userData.broker) - 1);
    userData.mqttPort = oldUserData.mqttPort;
    userData.extension = oldUserData.extension ;
    strncpy(userData.mqttUser, oldUserData.mqttUser, sizeof(userData.mqttUser) - 1);
    strncpy(userData.mqttPswrd, oldUserData.mqttPswrd, sizeof(userData.mqttPswrd) - 1);
    userData.sensorInterval= oldUserData.sensorInterval;
    userData.mbusInterval = oldUserData.mbusInterval; 
    userData.mbusSlaves = oldUserData.mbusSlaves;
    userData.mbusAddress1 = oldUserData.mbusAddress1;
    userData.mbusAddress2 = oldUserData.mbusAddress2;
    userData.mbusAddress3 = oldUserData.mbusAddress3;
    userData.mbusAddress4 = oldUserData.mbusAddress4;
    userData.mbusAddress5 = oldUserData.mbusAddress5;
    userData.haAutodisc = oldUserData.haAutodisc;
    userData.telegramDebug = oldUserData.telegramDebug;

    EEPROM.put(100, userData);
    credentialsSaved = EEPROM_CREDENTIALS_NEW;
    EEPROM.put(eeAddrCredentialsSaved, credentialsSaved);

  }
  else if(credentialsSaved == EEPROM_CREDENTIALS_NEW){  // Version after 1.0 (size of some variables has changed)
    EEPROM.get(100, userData );
  }
  uint8_t ethLinkMode = 0;
  EEPROM.get(EEPROM_ETH_LINK_MODE, ethLinkMode);
  ethForced10M = (ethLinkMode == 1);
  ethForced10MStored = ethForced10M;
  EEPROM.get(EEPROM_ETH_MODE_SETTING, ethModeSetting);
  if(ethModeSetting > ETH_MODE_FORCED10M){ // EEPROM never written
    ethModeSetting = ETH_MODE_AUTO;
  }
  if(ethModeSetting == ETH_MODE_AUTONEG){
    ethForced10M = false;
  }else if(ethModeSetting == ETH_MODE_FORCED10M){
    ethForced10M = true;
  }
  Serial.printf("[ETH] stored link mode from EEPROM: %s, web setting: %u\n", ethForced10M ? "forced 10M full duplex" : "auto negotiation", ethModeSetting);
  EEPROM.commit();
  EEPROM.end();

  if(userData.telegramDebug > 1){
    userData.telegramDebug = 0;
  }

  snprintf(html_buffer, sizeof(html_buffer), index_html,userData.ssid,userData.mbusinoName,userData.haAutodisc,userData.telegramDebug,ethModeSetting,userData.mbusInterval/1000,userData.broker,userData.mqttPort,userData.mqttUser,userData.mbusSlaves,userData.mbusAddress1,userData.mbusAddress2,userData.mbusAddress3,userData.mbusAddress4,userData.mbusAddress5);
  
  WiFi.onEvent(WiFiEvent);
  WiFi.hostname(userData.mbusinoName);
  //WiFi.hostname(String(String(userData.mbusinoName)+"-WiFi").c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(userData.ssid, userData.password);  
  
  Network.onEvent(onEvent);
  delay(100);
  setEthLinkMode();
  ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI);
  delay(2000);

  byte tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (tries++ > 5) {
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

  mbusLogBegin();
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
  ArduinoOTA.setMdnsEnabled(false); // the mDNS component crashes (IGMP join in lwIP) when a netif appears or is torn down while it is running, e.g. Ethernet coming up while the setup AP is shutting down; MBusino does not use mDNS
  server.onNotFound(onRequest);
  ArduinoOTA.begin();
  server.begin();

  client.setBufferSize(6000);
  Serial.printf("[BOOT] setup done at %lu ms\n", millis());

  mbusAddress[0] = userData.mbusAddress1;
  mbusAddress[1] = userData.mbusAddress2;
  mbusAddress[2] = userData.mbusAddress3;
  mbusAddress[3] = userData.mbusAddress4;
  mbusAddress[4] = userData.mbusAddress5;

  delay(20000);

}


void loop() {
  //delay(50);
  heapCalc();

  ArduinoOTA.handle();


  bool ethNoIP = gotIP == false && eth_connected == true && (millis() - timerETHmessage > ETH_DHCP_TIMEOUT);
  bool ethNoLink = ethLinkSeen == false && ethModeConfirmed == false && ethModeSetting == ETH_MODE_AUTO && (millis() - timerETHbegin > ETH_LINK_TIMEOUT); // with the wrong mode some W5500 get no link at all
  if((ethNoIP == true || ethNoLink == true) && waitForRestart == false){
    if(ethModeSetting == ETH_MODE_AUTO && ethModeConfirmed == false){
      ethForced10M = !ethForced10M; // try the other link mode
    }
    Serial.printf("[ETH] %lu ms: %s, restart ETH with %s\n", millis(), ethNoLink ? "no link" : "no IP received", ethForced10M ? "forced 10M full duplex" : "auto negotiation");
    ethRestarting = millis(); // suppress transport switching until ETH is back
    timerETHmessage = millis();
    ETH.end();
    delay(200);
    //SPI.end();
    delay(200);
    setEthLinkMode();
    ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_CS, ETH_PHY_IRQ, ETH_PHY_RST, ETH_PHY_SPI_HOST, ETH_PHY_SPI_SCK, ETH_PHY_SPI_MISO, ETH_PHY_SPI_MOSI,16);
  }

  if(gotIP == true && ethModeConfirmed == false){
    ethModeConfirmed = true;
    if(ethModeSetting == ETH_MODE_AUTO && ethForced10M != ethForced10MStored){ // remember the working link mode for the next boot
      Serial.printf("[ETH] %lu ms: link mode %s got an IP, remembering it for the next boot\n", millis(), ethForced10M ? "forced 10M full duplex" : "auto negotiation");
      uint8_t ethLinkMode = ethForced10M;
      EEPROM.begin(512);
      EEPROM.put(EEPROM_ETH_LINK_MODE, ethLinkMode);
      EEPROM.commit();
      EEPROM.end();
      ethForced10MStored = ethForced10M;
    }
  }

  // an intentional ETH restart (link mode change) is finished once an IP is back, or after the grace period at the latest
  if(ethRestarting != 0 && (gotIP == true || (millis() - ethRestarting) > ETH_LOSS_GRACE)){
    ethRestarting = 0;
  }

  //--- keep exactly one transport alive: Ethernet > Wi-Fi > setup AP -------------
  // As soon as Ethernet has an IP it is the only transport. The setup AP and a
  // possibly connected Wi-Fi STA are shut down here, otherwise the device ends up
  // with two IPs and MQTT would flip between them on every link change.
  if(gotIP == true && eth_connected == true){
    if(apMode == true){
      Serial.printf("[NET] %lu ms: Ethernet has an IP -> shutting down the setup AP\n", millis());
      dnsServer.stop();
      WiFi.softAPdisconnect(true);
      apMode = false;
    }
    if(WiFi.getMode() != WIFI_OFF){
      Serial.printf("[NET] %lu ms: Ethernet has an IP -> turning Wi-Fi off (single transport)\n", millis());
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
    }
  }

  //--- Ethernet loss handling ---------------------------------------------------
  // A working Ethernet connection may briefly drop (switch reboot, cable wiggle,
  // STP). Give it a grace period and restart only if it does not come back; after
  // the restart the boot picks the transport again (Ethernet -> Wi-Fi -> AP).
  // Without this the device could end up without any usable connection.
  if(ethLostSince != 0){
    if(eth_connected == true && gotIP == true){
      ethLostSince = 0; // safety net, the ETH_GOT_IP event normally clears it already
    }
    else if((millis() - ethLostSince) > ETH_LOSS_GRACE){
      Serial.printf("[NET] %lu ms: Ethernet gone for more than %lu ms -> restarting to re-pick the transport\n", millis(), (unsigned long)ETH_LOSS_GRACE);
      Serial.flush();
      ESP.restart();
    }
  }
/*
  if(gotIP == false && eth_connected == true && (millis() - timerETHmessage > 1000) && waitForRestart == false){
    Serial.println("no IP received, restart soon");
    timerReboot = millis();
    waitForRestart = true;
  }

*/

  if(wifiReconnect == true && (millis() - timerWifiReconnect > 2000)){
    wifiReconnect = false;
    if(eth_connected == true){ // Ethernet is the active transport, do not bring Wi-Fi up in addition
      Serial.printf("[NET] %lu ms: Wi-Fi reconnect skipped, Ethernet is connected\n", millis());
    }
    else if(WiFi.getMode() == WIFI_OFF){ // Wi-Fi was switched off for the Ethernet transport, do not bring it back silently
      Serial.printf("[NET] %lu ms: Wi-Fi reconnect skipped, Wi-Fi is off (Ethernet transport)\n", millis());
    }
    else{
      Serial.println("try to reconnect");
      WiFi.reconnect();
    }
  }


  if(credentialsReceived == true && waitForRestart == false){
    Serial.println("credentials received, save and restart soon");
    EEPROM.begin(512);
    EEPROM.put(100, userData);
    credentialsSaved = EEPROM_CREDENTIALS_NEW;
    EEPROM.put(eeAddrCredentialsSaved, credentialsSaved);
    EEPROM.put(EEPROM_ETH_MODE_SETTING, ethModeSetting);
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
    if((millis() - timerLedAPmode) > 500 && digitalRead(LED_BUILTIN)  == LOW){
      digitalWrite(LED_BUILTIN,HIGH);
      timerLedAPmode = millis();
    }
    if((millis() - timerLedAPmode) > 500 && digitalRead(LED_BUILTIN) == HIGH){
      digitalWrite(LED_BUILTIN,LOW);
      timerLedAPmode = millis();
    }    
  }

  //to notice Changes in Network an assign the right connection to the MQTT client
  //Do not fall back to Wi-Fi while ETH is intentionally re-initialised (link mode change)
  //or when Wi-Fi is switched off for the Ethernet transport.
  if((eth_connected == false) && (usedMQTTconection == 1) && ethRestarting == 0 && (WiFi.getMode() != WIFI_OFF) && (millis()-timerNetworkChange) > 5000){
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

  if (!client.connected() ) {  
    if((millis() - timerReconnect) > 5000){
      Serial.println("MQTT no connection");   
      if(apMode == false || gotIP == true){ // Ethernet may come up after the AP was started   
        reconnect();
      }
      else{
        Serial.println(" --> AP is running"); 
      }
      timerReconnect = millis();
    }
  }
  else{ // the whole main code run only if MQTT is connectet
    client.loop();  //MQTT Funktion

    if(newAddressReceived == true){
      newAddressReceived = false;
      waitToSetAddress = true;
      timerSetAddress = millis();
      mbus.normalize(254);
      client.publish(String(String(userData.mbusinoName) + "/setAddress/1").c_str(), "done");
    }

    if(waitToSetAddress == true && (millis() - 500) > timerSetAddress){
      waitToSetAddress = false;
      mbus.set_address(254,newAddress);
      client.publish(String(String(userData.mbusinoName) + "/setAddress/2").c_str(), String(newAddress).c_str());
    }

    ///////////////// publish settings ///////////////////////////////////
    if((millis()-timerDebug) >10000){
      timerDebug = millis();

      client.publish(String(String(userData.mbusinoName) + "/settings/wl_Connected").c_str(), String(WL_CONNECTED).c_str());    
      client.publish(String(String(userData.mbusinoName) + "/settings/apMode").c_str(), String(apMode).c_str());    
      client.publish(String(String(userData.mbusinoName) + "/settings/eth_connected").c_str(), String(eth_connected).c_str());    
      client.publish(String(String(userData.mbusinoName) + "/settings/ethLinkMode").c_str(), ethForced10M ? "forced 10M full duplex" : "auto negotiation");

      client.publish(String(String(userData.mbusinoName) + "/settings/ssid").c_str(), userData.ssid); 
      //client.publish(String(String(userData.mbusinoName) + "/settings/password").c_str(), String(userData.password)); 
      client.publish(String(String(userData.mbusinoName) + "/settings/broker").c_str(), userData.broker); 
      client.publish(String(String(userData.mbusinoName) + "/settings/port").c_str(), String(userData.mqttPort).c_str()); 
      client.publish(String(String(userData.mbusinoName) + "/settings/user").c_str(), userData.mqttUser); 
      //client.publish(String(String(userData.mbusinoName) + "/settings/pswd").c_str(), userData.mqttPswrd); 
      client.publish(String(String(userData.mbusinoName) + "/settings/name").c_str(), userData.mbusinoName); 
      client.publish(String(String(userData.mbusinoName) + "/settings/mbusInterval").c_str(), String(userData.mbusInterval).c_str()); 
      client.publish(String(String(userData.mbusinoName) + "/settings/slaves").c_str(), String(userData.mbusSlaves).c_str());
      client.publish(String(String(userData.mbusinoName) + "/settings/address1").c_str(), String(userData.mbusAddress1).c_str());
      client.publish(String(String(userData.mbusinoName) + "/settings/address2").c_str(), String(userData.mbusAddress2).c_str());
      client.publish(String(String(userData.mbusinoName) + "/settings/address3").c_str(), String(userData.mbusAddress3).c_str());    
      client.publish(String(String(userData.mbusinoName) + "/settings/address4").c_str(), String(userData.mbusAddress4).c_str());
      client.publish(String(String(userData.mbusinoName) + "/settings/address5").c_str(), String(userData.mbusAddress5).c_str());           
      client.publish(String(String(userData.mbusinoName) + "/settings/wifiIP").c_str(), String(WiFi.localIP().toString()).c_str());
      client.publish(String(String(userData.mbusinoName) + "/settings/ethIP").c_str(), String(ETH.localIP().toString()).c_str());
      client.publish(String(String(userData.mbusinoName) + "/settings/MQTTreconnections").c_str(), String(conCounter-1).c_str());
      if(usedMQTTconection == 1){
        client.publish(String(String(userData.mbusinoName) + "/settings/MQTTconnectedVia").c_str(), "ethernet");
      }
      if(usedMQTTconection == 2){
        client.publish(String(String(userData.mbusinoName) + "/settings/MQTTconnectedVia").c_str(), "WiFi");
      }
      long rssi = (WiFi.getMode() != WIFI_OFF) ? WiFi.RSSI() : 0; // Wi-Fi is off while Ethernet is the transport, an RSSI value would be meaningless
      client.publish(String(String(userData.mbusinoName) + "/settings/RSSI").c_str(), String(rssi).c_str()); 
      client.publish(String(String(userData.mbusinoName) + "/settings/version").c_str(), MBUSINO_VERSION);  
      client.publish(String(String(userData.mbusinoName) + "/settings/freeHeap").c_str(), String(ESP.getFreeHeap()).c_str()); 
      client.publish(String(String(userData.mbusinoName) + "/settings/minFreeHeap").c_str(), String(minFreeHeap).c_str()); 
    }

    ////////// M- Bus ###############################################


    if(firstrun == true){
      mbus.clearRXbuffer();
      for(uint8_t i = 0; i < userData.mbusSlaves; i++){
        initializeSlave[i] = false;
        mbus.normalize(mbusAddress[i]);
        client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalizeAtStart/address").c_str(), String(mbusAddress[i]).c_str()); 
        delay(2000);
        byte rxbuffer[256] = {0};
        uint8_t rxbuffercontent = 0;
        rxbuffercontent = mbus.read_rxbuffer(rxbuffer, sizeof(rxbuffer));
        if(rxbuffercontent > 0 && rxbuffer[0] == 0xE5) {
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalizeAtStart/ack").c_str(), String(rxbuffer[0]).c_str()); 
        }else{
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalizeAtStart/no_ack").c_str(), String(mbusAddress[i]).c_str()); 
          for(uint8_t i=0; i<=rxbuffercontent; i++){
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalizeAtStart/byte" + String(i)).c_str(), String(rxbuffer[i]).c_str());
          }
        }
      }
      firstrun = false;
    }
        /*
        mbusLoopStatus
        0 = preparation of addresses ect. for the next switch loop 
        1 = if needed, M-Bus normalisation / initialisation (FCB set to 0), else direct to case 3
        2 = only in case of an failure, wait for the answer (ACK) from M-Bus normalisation / initialisation
        3 = request the records from the slave
        4 = wait for the mbus response
        5 = get response from the rx buffer and decode the telegram
        6 = Send decoded M-Bus secords via MQTT
        */


    switch(mbusLoopStatus){
      case 0:
        if(shc){ // print the current mbusLoopStatus
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str()); 
          shc = false;
        }
        if((millis() - timerMbus) > userData.mbusInterval || polling == true || mtPolling == true ){ 
          if(polling == false && mtPolling == false){ // if polling is true or a following telegram of a multi telegram, dont touch the timer
            timerMbus = millis();
          }
          if(polling == true){ 
            currentAddress = pollingAddress;
            polling = false;
          }else if(mtPolling == true){ // mtPolling is to notice, the is another telegram with more records aviable
            //currentAddress dont change
            mtPolling = false;
          }else{ // preperation of address and counter for the next mbus loop
            if(addressCounter >= userData.mbusSlaves){
              addressCounter = 0;
            }
            currentAddress = mbusAddress[addressCounter];
            currentAddressCounter = addressCounter;
            addressCounter++;
            recordCounter[currentAddressCounter] = 0;
          }
          mbusLoopStatus = 1;
          shc = true;
        }
        break;

      case 1:
        if(shc){ // print the current mbusLoopStatus
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str()); 
          shc = false;
        }
        if(initializeSlave[currentAddressCounter] == true){
          initializeSlave[currentAddressCounter] = false;
          recordCounter[currentAddressCounter] = 0;
          mbus.clearRXbuffer();
          mbus.normalize(mbusAddress[currentAddressCounter]);    
          fcb[currentAddressCounter] = false;
          timerInitialize = millis();
          mbusLoopStatus = 2;
          shc = true;          
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/address").c_str(), String(mbusAddress[currentAddressCounter]).c_str()); 
        }else{
          mbusLoopStatus = 3;
          shc = true;
        }
        break;

      case 2: //only in case of an failure, wait for the answer (ACK) from M-Bus normalisation / initialisation
        if(shc){ // print the current mbusLoopStatus
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str()); 
          shc = false;
        }      
        if((millis() - timerInitialize) > 2000){
          if(mbus.available()){//mbus.available()){
            byte rxbuffer[256] = {0};
            uint8_t rxbuffercontent = 0;
            rxbuffercontent = mbus.read_rxbuffer(rxbuffer, sizeof(rxbuffer));
            if(rxbuffercontent == 1 && rxbuffer[0] == 0xE5) {
              client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/ack").c_str(), String(rxbuffer[0]).c_str()); 
            }else{
              client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/no_ack").c_str(), String(mbusAddress[currentAddressCounter]).c_str()); 
              for(uint8_t i=0; i<=rxbuffercontent; i++){
                client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/byte" + String(i)).c_str(), String(rxbuffer[i]).c_str());
              }
            }
          }else{
            client.publish(String(String(userData.mbusinoName) + "/MBus/debug/nomalize/no_answer").c_str(), String(mbusAddress[currentAddressCounter]).c_str()); 
          }
          mbusLoopStatus = 3;
          shc = true;
        }

        break;
      
      case 3: // request the records from the slave
        if(shc){ // print the current mbusLoopStatus
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str()); 
          shc = false;
        }      
        mbus.clearRXbuffer();
        mbus.request_data(currentAddress,fcb[currentAddressCounter]);
        client.publish(String(String(userData.mbusinoName) + "/MBus/debug/request/currentAddress").c_str(), String(currentAddress).c_str()); 
        client.publish(String(String(userData.mbusinoName) + "/MBus/debug/request/fcb").c_str(),String(fcb[currentAddressCounter]).c_str()); 
        client.publish(String(String(userData.mbusinoName) + "/MBus/debug/request/cac").c_str(),String(currentAddressCounter).c_str()); 
        client.publish(String(String(userData.mbusinoName) + "/MBus/debug/request/ac").c_str(),String(addressCounter).c_str());
        mbusLoopStatus = 4;
        shc = true;
        timerMbusReq = millis();
        break;

      case 4: // wait for the mbus response
        if(shc){ // print the current mbusLoopStatus
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str()); 
          shc = false;
        }      
        if(mbus.available()){
          mbusLoopStatus = 5;
          shc = true;
          timerSerialAvailable = millis();
        }
        if(millis() - timerMbusReq > 2000){ // failure, no data received
          initializeSlave[currentAddressCounter] = true;
          recordCounter[currentAddressCounter] = 0;
          fcb[currentAddressCounter] = false;              
          client.publish(String(String(userData.mbusinoName)  +"/MBus/SlaveAddress"+String(currentAddress)+ "/MBUSerror").c_str(), "no_Data_received");
          mbusLogAdd(MBUS_LOG_NO_DATA, currentAddress, NULL);
          mbusLoopStatus = 0;
          shc = true;
          mtPolling = false;
        }
        break;

      case 5: //get response from the rx buffer and decode the telegram
        if(shc){ // print the current mbusLoopStatus
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str()); 
          shc = false;
        }      
        if((millis() - timerSerialAvailable) > 1500){ // Receive and decode M-Bus Records
          mbusLoopStatus = 6;
          shc = true;
          bool mbus_good_frame = false;
          byte mbus_data[MBUS_DATA_SIZE] = { 0 };
          mbus_good_frame = mbus.get_response(mbus_data, sizeof(mbus_data));
          mbusLogAdd(mbus_good_frame ? MBUS_LOG_OK : MBUS_LOG_BAD_FRAME, currentAddress, mbus_data);

          if(userData.telegramDebug == true){
          //------------------ only for debug, you will recieve the whole M-Bus telegram bytewise in HEX for analysis -----------------
            //char telegram[(mbus_data[1]+6)*2] = {0};
            char telegram[520] = {0};
            for(uint8_t i = 0; i <= mbus_data[1]+6; i++){                                                             //|
              char buffer[3];                                                                                         //|
              sprintf(buffer,"%02X",mbus_data[i]);                                                                    //|
              //client.publish(String(String(userData.mbusinoName) + "/MBus/debug/telegram_byte_"+String(i)).c_str(), String(buffer).c_str());  
              telegram[i*2] = buffer[0];
              telegram[(i*2)+1] = buffer[1];       //|
            }  
            client.publish(String(String(userData.mbusinoName) +"/MBus/debug/telegram"+ String(mbusAddress[currentAddressCounter])).c_str(), telegram);                                                                                                       //|
            //--------------------------------------------------------------------------------------------------------------------------    
          }
          //mbus_good_frame = true;
          //byte mbus_data[] = {0x68,0xC1,0xC1,0x68,0x08,0x00,0x72,0x09,0x34,0x75,0x73,0xC5,0x14,0x00,0x0D,0x43,0x00,0x00,0x00,0x04,0x78,0x41,0x63,0x65,0x04,0x04,0x06,0xAA,0x29,0x00,0x00,0x04,0x13,0x40,0xA1,0x75,0x00,0x04,0x2B,0x00,0x00,0x00,0x00,0x14,0x2B,0x3C,0xF3,0x00,0x00,0x04,0x3B,0x48,0x06,0x00,0x00,0x14,0x3B,0x4E,0x0E,0x00,0x00,0x02,0x5B,0x19,0x00,0x02,0x5F,0x19,0x00,0x02,0x61,0xFA,0xFF,0x02,0x23,0xAC,0x08,0x04,0x6D,0x03,0x2A,0xF1,0x2A,0x44,0x06,0x92,0x0C,0x00,0x00,0x44,0x13,0x2D,0x9B,0x1C,0x00,0x42,0x6C,0xDF,0x2C,0x01,0xFD,0x17,0x00,0x03,0xFD,0x0C,0x05,0x00,0x00,0x84,0x10,0x06,0x1A,0x00,0x00,0x00,0xC4,0x10,0x06,0x05,0x00,0x00,0x00,0x84,0x20,0x06,0x00,0x00,0x00,0x00,0xC4,0x20,0x06,0x00,0x00,0x00,0x00,0x84,0x30,0x06,0x00,0x00,0x00,0x00,0xC4,0x30,0x06,0x00,0x00,0x00,0x00,0x84,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0x40,0x13,0x00,0x00,0x00,0x00,0x84,0x80,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0x80,0x40,0x13,0x00,0x00,0x00,0x00,0x84,0xC0,0x40,0x13,0x00,0x00,0x00,0x00,0xC4,0xC0,0x40,0x13,0x00,0x00,0x00,0x00,0x75,0x16};


          if (mbus_good_frame) {
            if(fcb[currentAddressCounter] == true){ // toggle the FCB (Frame Count Bit) to signalize good response in the next request
              fcb[currentAddressCounter] = false;
            }else{
              fcb[currentAddressCounter] = true;
            }

            int packet_size = mbus_data[1] + 6; 
            JsonDocument doc;
            JsonObject headerObj = doc["header"].to<JsonObject>();
            bool headerOk = payload.decodeHeader(mbus_data, packet_size, headerObj);
            JsonArray recordsArr = doc["records"].to<JsonArray>();  
            fields = payload.decodeRecords(&mbus_data[Startadd], packet_size - Startadd - 2, recordsArr); 
            address = mbus_data[5]; 
            
            serializeJson(doc, jsonstring);
            mbusLogSetJson(address, jsonstring);
            client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/error").c_str(), String(payload.getError()).c_str());  // kann auskommentiert werden wenn es läuft
            client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/jsonstring").c_str(), jsonstring);  
            client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/fcb").c_str(), String(fcb[currentAddressCounter]).c_str());      
            heapCalc();        
            if(mbus_data[12]==0x14&&mbus_data[11]==0xC5){
              engelmann = true;
            }
            else{
              engelmann = false;
            }
          }else{  //Fehlermeldung          
              initializeSlave[currentAddressCounter] = true;
              recordCounter[currentAddressCounter] = 0;
              fcb[currentAddressCounter] = false;
              jsonstring[0] = 0;
              client.publish(String(String(userData.mbusinoName)  +"/MBus/SlaveAddress"+String(currentAddress)+ "/MBUSerror").c_str(), "no_good_telegram");
              mbusLoopStatus = 0;
              shc = true;
              mtPolling = false;
          }
        } 
        break;

      case 6: // Send decoded M-Bus secords via MQTT
        if(shc){ // print the current mbusLoopStatus
          client.publish(String(String(userData.mbusinoName) + "/MBus/debug/mbus_Loop").c_str(), String(mbusLoopStatus).c_str()); 
          shc = false;
        }
        if(millis() - timerMbusDecoded > 100){  
          mbusLoopStatus = 0;
          shc = true;
          JsonDocument doc;
          deserializeJson(doc, jsonstring);
          JsonObject headerObj = doc["header"];
          JsonArray recordsArr = doc["records"];
          jsonstring[0] = 0;

          // Publish M-Bus header via MQTT
          if (!headerObj.isNull()) {
            for(uint8_t i = 0; i < HEADER_PUBLISH_COUNT; i++){
              char valBuf[20] = {0};
              const char* val = nullptr;
              switch(headerPublishFields[i].type){
                case HT_STR: val = headerObj[headerPublishFields[i].jsonKey].as<const char*>(); break;
                case HT_INT: snprintf(valBuf, sizeof(valBuf), "%d", headerObj[headerPublishFields[i].jsonKey].as<int>()); val = valBuf; break;
                case HT_HEX: snprintf(valBuf, sizeof(valBuf), "%x", headerObj[headerPublishFields[i].jsonKey].as<int>()); val = valBuf; break;
                default: break;
              }
              if(val) client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress" + String(address) + "/header/" + headerPublishFields[i].jsonKey).c_str(), val);
            }
            JsonObject sd = headerObj["status_details"];
            for(uint8_t i = 0; i < HEADER_STATUS_COUNT; i++){
              client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress" + String(address) + "/header/" + headerStatusKeys[i]).c_str(),
                             sd[headerStatusKeys[i]].as<bool>() ? "on" : "off");
            }
          }

          // Header autodiscovery
          if(userData.haAutodisc == true && adMbusMessageCounter == 3){
            haHandoverHeader(address);
          }

          for (uint8_t i=0; i<fields; i++) {
            uint8_t code = recordsArr[i]["code"].as<int>();
            const char* name = recordsArr[i]["name"];
            const char* units = recordsArr[i]["units"];           
            double value = recordsArr[i]["value_scaled"].as<double>();
            const char* valueString = recordsArr[i]["value_string"];   
            bool telegramFollow = recordsArr[i]["telegramFollow"].as<int>();    

            if(userData.haAutodisc == true && adMbusMessageCounter == 3){  //every 254 message is a HA autoconfig message
              strcpy(adVariables.haName,name);
              if(units != NULL){
                strcpy(adVariables.haUnits,units);
              }else{
                strcpy(adVariables.haUnits,""); 
              }
              strcpy(adVariables.stateClass,payload.getStateClass(code));
              strcpy(adVariables.deviceClass,payload.getDeviceClass(code, (uint8_t)headerObj["medium_code"].as<int>()));     
              haHandoverMbus(recordCounter[currentAddressCounter]+i+1, engelmann, address);
            }else{               

              //two messages per value, values comes as number or as ASCII string or both
              if(valueString != NULL && strlen(valueString) > 0){
                client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/"+String(recordCounter[currentAddressCounter]+i+1)+"_vs_"+String(name)).c_str(), valueString); // send the value if a ascii value is aviable (variable length)
              }
              client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/"+String(recordCounter[currentAddressCounter]+i+1)+"_"+String(name)).c_str(), String(value,3).c_str()); // send the value if a real value is aviable (standard)
              if(units != NULL && strlen(units) > 0){
                client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/"+String(recordCounter[currentAddressCounter]+i+1)+"_"+String(name)+"_unit").c_str(), units);
              }
              //or only one message
              //client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/MBus/"+String(i+1)+"_"+String(name)+"_in_"+String(units)), String(value,3).c_str());

              if (i == 3 && engelmann == true){  // Sensostar Bugfix --> comment it out if you use not a Sensostar
                float flow = recordsArr[5]["value_scaled"].as<float>();
                float delta = recordsArr[9]["value_scaled"].as<float>();
                float calc_power = delta * flow * 1163;          
                client.publish(String(String(userData.mbusinoName) +"/MBus/SlaveAddress"+String(address)+ "/4_power_calc").c_str(), String(calc_power).c_str());                    
              }           
            }  

            if(fields == i+1){ // ...last Field
              if(telegramFollow == 1){ // if onother telegram with more records Aviable 
                client.publish(String(String(userData.mbusinoName) + "/MBus/SlaveAddress"+String(address)+ "/"+String(recordCounter[currentAddressCounter]+i+1)).c_str(),"--> More records follow in next telegram");              
                recordCounter[currentAddressCounter] = recordCounter[currentAddressCounter] + fields; // add the current number of recieved records to the previusly sended numbers of records 
                mtPolling = true;
                client.publish(String(String(userData.mbusinoName) + "/MBus/debug/TF/true").c_str(),String(currentAddress).c_str());                                                                                                                      
              }else{
                recordCounter[currentAddressCounter] = 0;
                mtPolling = false;
                if(addressCounter == 1){
                  adMbusMessageCounter++;
                }
                client.publish(String(String(userData.mbusinoName) + "/MBus/debug/TF/false").c_str(),String(currentAddress).c_str());                                                                                                       
              }
            }        
          }
          address = 0; 
        }   
        break;
    }

    heapCalc();
  }
}

void heapCalc(){
  if(minFreeHeap > ESP.getFreeHeap()){
    minFreeHeap = ESP.getFreeHeap();
  }
}
