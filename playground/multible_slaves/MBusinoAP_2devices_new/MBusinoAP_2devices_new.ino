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


#include <PubSubClient.h>
#include <OneWire.h>            // Library for OneWire Bus
#include <DallasTemperature.h>  //Library for DS18B20 Sensors
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h> 
#include <ESPAsyncTCP.h>
#include <DNSServer.h>
#include <ArduinoOTA.h>

#include <MBusinoLib.h>  // Library for decode M-Bus
#include <ArduinoJson.h>
#include <EEPROM.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include <SoftwareSerial.h>

SoftwareSerial *customSerial;
EspSoftwareSerial::UART ESP8266Serial;

#define MBUSINO_VERSION "0.7.1"

#define MBUS_BAUD_RATE 2400
#define MBUS_ADDRESS 0xFE  // brodcast
#define MBUS_TIMEOUT 1000  // milliseconds
#define MBUS_DATA_SIZE 255
#define MBUS_GOOD_FRAME true
#define MBUS_BAD_FRAME false

#define MBUS_DEVICES 0x02
#define MBUS_ADDRESS_1 0x01
#define MBUS_ADDRESS_2 0x02

#define ONE_WIRE_BUS1 2   //D4
#define ONE_WIRE_BUS2 13  //D7
#define ONE_WIRE_BUS3 12  //D6
#define ONE_WIRE_BUS4 14  //D5
#define ONE_WIRE_BUS5 0   //D3
#define ONE_WIRE_BUS6 5   //D1
#define ONE_WIRE_BUS7 4   //D2

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;  // I2C

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
  uint8_t mbusSlaves;
  uint8_t mbusAddress1;
  uint8_t mbusAddress2;
  uint8_t mbusAddress3;
} userData = {"SSID","Password","MBusino","192.168.1.7",1883,5,"mqttUser","mqttPasword",5000,120000,1,0xFE,0,0};

uint8_t mbusAddress[3] = {0};

bool mqttcon = false;
bool apMode = false;
bool credentialsReceived = false;
uint16_t conCounter = 0;


uint8_t currentAddress = 0;
uint8_t addressCounter = 0;
bool mbusCleared = false;

int Startadd = 0x13;  // Start address for decoding

float OW[7] = {0};         // variables for DS18B20 Onewire sensors 
float temperatur = 0;      // Variablen für den BLE280 am I2C
float druck = 0;
float hoehe = 0;
float feuchte = 0;
bool bmeStatus;

bool mbusReq = false;
bool waitForRestart = false;

unsigned long timerMQTT = 15000;
unsigned long timerSensorRefresh1 = 0;
unsigned long timerSensorRefresh2 = 0;
unsigned long timerMbus1 = 0;
unsigned long timerMbus2 = 0;
unsigned long timerDebug = 0;
unsigned long timerReconnect = 0;
unsigned long timerRestart = 0;

void mbus_request_data(byte address);
void mbus_short_frame(byte address, byte C_field);
bool mbus_get_response(byte *pdata, unsigned char len_pdata);
void mbus_normalize(byte address);
void mbus_clearRXbuffer();
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

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>MBusino Setup</title>
    <style>
      *,
      ::after,
      ::before {
        box-sizing: border-box
      }

      body {
        margin: 0;
        font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans';
        font-size: 1rem;
        font-weight: 400;
        line-height: 1.5;
        color: #fff;
        background-color: #438287
      }

      .form-control {
        display: block;
        width: 100%%;
        height: calc(1.5em + .75rem + 2px);
        border: 1px solid #ced4da
      }

      button {
        cursor: pointer;
        border: 1px solid transparent;
        color: #fff;
        background-color: #304648;
        border-color: #304648;
        padding: .5rem 1rem;
        font-size: 1.25rem;
        line-height: 1.5;
        border-radius: .3rem;
        width: 100%%
      }

      .form-signin {
        width: 100%%;
        max-width: 400px;
        padding: 15px;
        margin: auto
      }

      h1 {
        text-align: center
      }
    </style>
  </head>
  <body>
    <main class='form-signin'>
      <form action='/get'>
        <h1 class=''><i>MBusino</i> Setup</h1><br>
        <div class='form-floating'><label>SSID</label><input type='text' value='%s' class='form-control' name='ssid'></div>
        <div class='form-floating'><label>Password</label><input type='password' class='form-control' name='password'></div>
        <div class='form-floating'><label>Device Name</label><input type='text' value='%s' class='form-control' name='name'>
        </div><br><label for='extension'>Stage of Extension:</label><br><select name='extension' id='extension'>
          <option value='5'>(5) 5x DS18B20 + BME</option>
          <option value='7'>(7) 7x DS18B20 no BME</option>
          <option value='0'>(0) only M-Bus</option>
          <option value='' selected>choose option</option>
        </select>  stored: %u <br><br>
        <div class='form-floating'><label>Sensor publish interval sec.</label><input type='text' value='%u' class='form-control' name='sensorInterval'></div>
        <div class='form-floating'><label>M-Bus publish interval sec.</label><input type='text' value='%u' class='form-control' name='mbusInterval'></div>
        <div class='form-floating'><label>MQTT Broker</label><input type='text' value='%s' class='form-control' name='broker'></div>
        <div class='form-floating'><label>MQTT Port</label><input type='text' value='%u' class='form-control' name='mqttPort'></div>
        <div class='form-floating'><label>MQTT User (optional)</label><input type='text' value='%s' class='form-control' name='mqttUser'></div>
        <div class='form-floating'><label>MQTT Password (optional)</label><input type='password' class='form-control' name='mqttPswrd'></div>
        <div class='form-floating'><label>number of M-Bus Slaves</label><input type='text' value='%u' class='form-control' name='mbusSlaves'></div>
        <div class='form-floating'><label>M-Bus address 1</label><input type='text' value='%u' class='form-control' name='mbusAddress1'></div>
        <div class='form-floating'><label>M-Bus address 2</label><input type='text' value='%u' class='form-control' name='mbusAddress2'></div>
        <div class='form-floating'><label>M-Bus address 3</label><input type='text' value='%u' class='form-control' name='mbusAddress3'></div>
        <br>
        <button type='submit'>Save</button>
        <p style='text-align:right'><a href='/update' style='color:#3F4CFB'>update</a></p>
        <p style='text-align:right'><a href='https://www.github.com/zeppelin500/mbusino' style='color:#3F4CFB'>MBusino</a></p>
      </form>
    </main>
  </body>
</html>)rawliteral";

char html_buffer[sizeof(index_html)+200] = {0};

const char update_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='en'>
  <head>
    <meta charset='utf-8'>
    <meta name='viewport' content='width=device-width,initial-scale=1'>
    <title>MBusino update</title>
    <style>
      *,
      ::after,
      ::before {
        box-sizing: border-box
      }

      body {
        margin: 0;
        font-family: 'Segoe UI', Roboto, 'Helvetica Neue', Arial, 'Noto Sans', 'Liberation Sans';
        font-size: 1rem;
        font-weight: 400;
        line-height: 1.5;
        color: #fff;
        background-color: #438287
      }

      h1 {
        text-align: center
      }
    </style>
  </head>
  <body>
    <main class='form-signin'>
       	<h1 class=''><i>MBusino</i> update</h1><br>
          <p style='text-align:center'> select a xxx.bin file </p><br>
			    <form style='text-align:center' method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'>
         	</form><br/>
          <p style='text-align:center'> MBusino will restart after update </p><br>
          <p style='text-align:center'><a href='/' style='color:#3F4CFB'>home</a></p>
    </main>
  </body>
</html>)rawliteral";


class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", html_buffer); 
  }
};

void setup() {
  Serial.setRxBufferSize(256);
  Serial.begin(MBUS_BAUD_RATE, SERIAL_8E1);
  //customSerial = &ESP8266Serial;
  //customSerial->begin(MBUS_BAUD_RATE, SWSERIAL_8E1,5,4,false,254); // mbus uses 8E1 encoding

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

  sprintf(html_buffer, index_html,userData.ssid,userData.mbusinoName,userData.extension,userData.sensorInterval/1000,userData.mbusInterval/1000,userData.broker,userData.mqttPort,userData.mqttUser,userData.mbusSlaves,userData.mbusAddress1,userData.mbusAddress2,userData.mbusAddress3);

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
      timerRestart = millis();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", waitForRestart?"success, restart now":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index){
      //Serial.printf("Update Start: %s\n", filename.c_str());
      Update.runAsync(true);
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

  //AsyncElegantOTA.begin(&server);
  ArduinoOTA.begin(&server);
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
    timerRestart = millis();
    waitForRestart = true;
  }

  if(waitForRestart==true && (millis() - timerRestart) > 1000){
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
    client.publish(String(String(userData.mbusinoName) + "/settings/pswd").c_str(), userData.mqttPswrd); 
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
    for(uint8_t i = 0; i < userData.extension; i++){
      if(OW[i] != -127){
        client.publish(String(String(userData.mbusinoName) + "/OneWire/S" + String(i+1)).c_str(), String(OWwO[i]).c_str());
        client.publish(String(String(userData.mbusinoName) + "/OneWire/offset" + String(i+1)).c_str(), String(offset[i]).c_str());
      }      
    }
  
    if(userData.extension == 5){
      client.publish(String(String(userData.mbusinoName) + "/bme/Temperatur").c_str(), String(temperatur).c_str());
      client.publish(String(String(userData.mbusinoName) + "/bme/Druck").c_str(), String(druck).c_str());
      client.publish(String(String(userData.mbusinoName) + "/bme/Hoehe").c_str(), String(hoehe).c_str());
      client.publish(String(String(userData.mbusinoName) + "/bme/Feuchte").c_str(), String(feuchte).c_str());
    }
     timerMQTT = millis();
  }

  if(millis() - timerMbus1 > userData.mbusInterval){
    timerMbus1 = millis();
    timerMbus2 = millis();    
    if(addressCounter >= userData.mbusSlaves){
      addressCounter = 0;
    }
    currentAddress = mbusAddress[addressCounter];
    addressCounter++;
    mbus_clearRXbuffer();
    mbusCleared = true;
  }

  if(millis() - timerMbus1 > 1000 && mbusCleared == true){
    mbusReq = true;
    mbusCleared = false;
    mbus_request_data(currentAddress);
  }
  if(millis() - timerMbus2 > 2000 && mbusReq == true){
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
      MBusinoLib payload(254);  
      JsonDocument jsonBuffer;
      JsonArray root = jsonBuffer.add<JsonArray>();  
      uint8_t fields = payload.decode(&mbus_data[Startadd], packet_size - Startadd - 2, root); 
      char jsonstring[2048] = { 0 };
      yield();
      serializeJson(root, jsonstring);
      client.publish(String(String(userData.mbusinoName) + "/Slave"+String(addressCounter)+ "/MBus/error").c_str(), String(payload.getError()).c_str());  // kann auskommentiert werden wenn es läuft
      yield();
      client.publish(String(String(userData.mbusinoName) + "/Slave"+String(addressCounter)+ "/MBus/jsonstring").c_str(), jsonstring);

      for (uint8_t i=0; i<fields; i++) {
        uint8_t code = root[i]["code"].as<int>();
        const char* name = root[i]["name"];
        const char* units = root[i]["units"];           
        double value = root[i]["value_scaled"].as<double>(); 
        const char* valueString = root[i]["value_string"];            

        //two messages per value, values comes as number or as ASCII string or both
        client.publish(String(String(userData.mbusinoName) +"/Slave"+String(addressCounter)+ "/MBus/"+String(i+1)+"_vs_"+String(name)).c_str(), valueString); // send the value if a ascii value is aviable (variable length)
        client.publish(String(String(userData.mbusinoName) +"/Slave"+String(addressCounter)+ "/MBus/"+String(i+1)+"_"+String(name)).c_str(), String(value,3).c_str()); // send the value if a real value is aviable (standard)
        client.publish(String(String(userData.mbusinoName) +"/Slave"+String(addressCounter)+ "/MBus/"+String(i+1)+"_"+String(name)+"_unit").c_str(), units);
        //or only one message
        //client.publish(String(String(userData.mbusinoName) +"/Slave"+String(addressCounter)+ "/MBus/"+String(i+1)+"_"+String(name)+"_in_"+String(units)), String(value,3).c_str());

        if (i == 3){  // Sensostar Bugfix --> comment it out if you use not a Sensostar
          float flow = root[5]["value_scaled"].as<float>();
          float delta = root[9]["value_scaled"].as<float>();
          float calc_power = delta * flow * 1163;          
          client.publish(String(String(userData.mbusinoName) +"/Slave"+String(addressCounter)+ "/MBus/4_power_calc").c_str(), String(calc_power).c_str());                    
        }   
        yield();      
      }
    } 
    else {
  //Fehlermeldung
        client.publish(String(String(userData.mbusinoName)  +"/Slave"+String(addressCounter)+ "/MBUSerror").c_str(), "no_good_telegram");
    }
    mbus_normalize(currentAddress);
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

void mbus_normalize(byte address) {
  mbus_short_frame(address,0x40);
}

void mbus_clearRXbuffer(){
  while (Serial.available()) {
    byte received_byte = (byte)Serial.read();
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

  client.publish(String(String(userData.mbusinoName) + "/cal/started").c_str(), "buliding average");

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
  client.publish(String(String(userData.mbusinoName) + "/cal/connected").c_str(), String(connected).c_str());
  client.publish(String(String(userData.mbusinoName) + "/cal/sum").c_str(), String(sum).c_str());
  client.publish(String(String(userData.mbusinoName) + "/cal/average").c_str(), String(average).c_str()); 

  for(uint8_t i = 0; i < userData.extension; i++){
    if(!OWnotconnected[i]){
      offset[i] = average - OW[i];
    }

  }
  for(uint8_t i = 0; i < userData.extension; i++){
    if(!OWnotconnected[i]){
      client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(i+1)).c_str(), String(offset[i]).c_str());
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
      client.publish(String(String(userData.mbusinoName) + "/cal/offffsetS" + String(sensor+1)).c_str(), "No valid sensor");
    }

}

void calibrationValue(float value){
  client.publish(String(String(userData.mbusinoName) + "/cal/started").c_str(), "set new offset");
   
  if(OW[sensorToCalibrate] != -127){
    offset[sensorToCalibrate] += value;
    client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(sensorToCalibrate+1)).c_str(), String(offset[sensorToCalibrate]).c_str());
    EEPROM.begin(512);
    EEPROM.put(eeAddrOffset[sensorToCalibrate], offset[sensorToCalibrate]);  // copy offset to EEPROM
    EEPROM.commit();
    EEPROM.end();
  }
  else{
    client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(sensorToCalibrate)).c_str(), "No Sensor");
  }
}

void calibrationBME(){

  client.publish(String(String(userData.mbusinoName) + "/cal/started").c_str(), "set BME280 based offset");
  client.publish(String(String(userData.mbusinoName) + "/cal/BME").c_str(), String(temperatur).c_str());

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
      client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(i+1)).c_str(), String(offset[i]).c_str());
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

  client.publish(String(String(userData.mbusinoName) + "/cal/started").c_str(), "set all offsets 0");

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
      client.publish(String(String(userData.mbusinoName) + "/cal/offsetS" + String(i+1)).c_str(), String(offset[i]).c_str());
    }
  }
  EEPROM.begin(512);
  for(uint8_t i = 0; i < userData.extension; i++){
    EEPROM.put(eeAddrOffset[i], offset[i]);  // copy offset to EEPROM
  }
  EEPROM.commit();
  EEPROM.end();
}

void reconnect() {
  // Loop until we're reconnected
  char lwBuffer[30] = {0};
  sprintf(lwBuffer, userData.mbusinoName, "/lastwill");
  if (client.connect(userData.mbusinoName,userData.mqttUser,userData.mqttPswrd,lwBuffer,0,false,"I am going offline")) {
    // Once connected, publish an announcement...
    conCounter++;
    if(conCounter == 1){
      client.publish(String(String(userData.mbusinoName) + "/start").c_str(), "Bin hochgefahren, WLAN und MQTT steht");
    }
    else{
      client.publish(String(String(userData.mbusinoName) + "/reconnect").c_str(), "Online again!");
    }
    // ... and resubscribe
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateAverage").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateSensor").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateValue").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateBME").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateSet0").c_str());
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  if(userData.extension > 0){
    if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateAverage").c_str())==0){  
      calibrationAverage();
    }
    if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateSensor").c_str())==0){  
      calibrationSensor(atoi((char*)payload)-1);
    }
    if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateValue").c_str())==0){  
      calibrationValue(atof((char*)payload));
    }  
    if(userData.extension == 5){
      if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateBME").c_str())==0){  
        calibrationBME();
      }
    }
    if (strcmp(topic,String(String(userData.mbusinoName) + "/calibrateSet0").c_str())==0){  
      calibrationSet0();
    } 
  }
}   

void setupServer(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", html_buffer); 
  //Serial.println("Client Connected");
  });
    
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String inputMessage;
      String inputParam;

      if (request->hasParam("ssid")) {
        inputMessage = request->getParam("ssid")->value();
        inputParam = "ssid";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.ssid, sizeof(userData.ssid));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("password")) {
        inputMessage = request->getParam("password")->value();
        inputParam = "password";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.password, sizeof(userData.password));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("name")) {
        inputMessage = request->getParam("name")->value();
        inputParam = "name";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.mbusinoName, sizeof(userData.mbusinoName));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("broker")) {
        inputMessage = request->getParam("broker")->value();
        inputParam = "broker";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.broker, sizeof(userData.broker));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("mqttPort")) {
        inputMessage = request->getParam("mqttPort")->value();
        inputParam = "mqttPort";
        if(inputMessage != NULL){
          userData.mqttPort = inputMessage.toInt();
          credentialsReceived = true;
        }
      }

      if (request->hasParam("extension")) {
        inputMessage = request->getParam("extension")->value();
        inputParam = "extension";
        if(inputMessage != NULL){
          userData.extension = inputMessage.toInt();
          credentialsReceived = true;
          }
      }

      if (request->hasParam("sensorInterval")) {
        inputMessage = request->getParam("sensorInterval")->value();
        inputParam = "sensorInterval";
        if(inputMessage != NULL){
          userData.sensorInterval = inputMessage.toInt()  * 1000;
          credentialsReceived = true;
          }
      }

      if (request->hasParam("mbusInterval")) {
        inputMessage = request->getParam("mbusInterval")->value();
        inputParam = "mbusInterval";
          if(inputMessage != NULL){
            userData.mbusInterval = inputMessage.toInt() * 1000;
            credentialsReceived = true;
          }
      }

      if (request->hasParam("mqttUser")) {
        inputMessage = request->getParam("mqttUser")->value();
        inputParam = "mqttUser";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.mqttUser, sizeof(userData.mqttUser));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("mqttPswrd")) {
        inputMessage = request->getParam("mqttPswrd")->value();
        inputParam = "mqttPswrd";
        if(inputMessage != NULL){
          inputMessage.toCharArray(userData.mqttPswrd, sizeof(userData.mqttPswrd));
          credentialsReceived = true;
        }
      }

      if (request->hasParam("mbusSlaves")) {
        inputMessage = request->getParam("mbusSlaves")->value();
        inputParam = "mbusSlaves";
        if(inputMessage != NULL){
          userData.mbusSlaves = inputMessage.toInt();
          credentialsReceived = true;
        }
      }   

      if (request->hasParam("mbusAddress1")) {
        inputMessage = request->getParam("mbusAddress1")->value();
        inputParam = "mbusAddress1";
        if(inputMessage != NULL){
          userData.mbusAddress1 = inputMessage.toInt();
          credentialsReceived = true;
        }
      }  

            if (request->hasParam("mbusAddress2")) {
        inputMessage = request->getParam("mbusAddress2")->value();
        inputParam = "mbusAddress2";
        if(inputMessage != NULL){
          userData.mbusAddress2 = inputMessage.toInt();
          credentialsReceived = true;
        }
      } 

            if (request->hasParam("mbusAddress3")) {
        inputMessage = request->getParam("mbusAddress3")->value();
        inputParam = "mbusAddress3";
        if(inputMessage != NULL){
          userData.mbusAddress3 = inputMessage.toInt();
          credentialsReceived = true;
        }
      }   

      request->send(200, "text/html", "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>MBusino Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#FFF;background-color:#438287;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1><i>MBusino</i> Setup</h1> <br/> <p>Your settings have been saved successfully!<br />MBusino restart now!<br />MQTT should now work. <br /> If you find the Acces Point network again, your credentials were wrong.</p></main></body></html>");
      //request->send(200, "text/html", "The values entered by you have been successfully sent to the device <br><a href=\"/\">Return to Home Page</a>");
  });
}

void onRequest(AsyncWebServerRequest *request){
  //Handle Unknown Request
  request->send(404);
}
