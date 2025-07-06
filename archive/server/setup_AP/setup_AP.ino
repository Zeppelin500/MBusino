/*  ============================================================================================================
MBusino Test code for a configuration server

  ============================================================================================================== */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <EspMQTTClient.h>
ESP8266WebServer    server(80);

struct settings {
  char ssid[30];
  char password[30];
  char broker[20];
  uint16_t mqttPort;
  char mqttUser[30];
  char mqttPswrd[30];
} userData = {};

EspMQTTClient client;
/*
EspMQTTClient client(
  userData.broker,     // MQTT Broker IP address in quote marks "192.168.x.x"
  userData.mqttPort,             // The MQTT port, default to 1883. this line can be omitted
  userData.mqttUser,  // Can be omitted if not needed
  userData.mqttPswrd,  // "MQTTPassword" also password for OTA Update with Arduino IDE 2
  "servertest"    // Client name that uniquely identify your device
);
*/

unsigned long timer = 0;

void setup() {
  
  EEPROM.begin(512);
  EEPROM.get( 40, userData );
  EEPROM.commit();
  EEPROM.end();
   
  client.setMqttClientName("servertest");
  client.setMqttServer(userData.broker, userData.mqttUser, userData.mqttPswrd, userData.mqttPort);

  WiFi.mode(WIFI_STA);
  WiFi.begin(userData.ssid, userData.password);
  
  byte tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    if (tries++ > 30) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("MBusino Setup Portal"); //, "secret");
      break;
    }
  }
  server.on("/",  handlePortal);
  server.begin();

    // Optional functionalities of EspMQTTClient
  client.enableHTTPWebUpdater();                                           // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  client.enableOTA("mbusino");                                                      // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  client.enableLastWillMessage("servertest/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
  client.setMaxPacketSize(6000);
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished() {  // send a message to MQTT broker if connected.
  client.publish("servertest/start", "bin hoch gefahren, WLAN und MQTT seht ");  
}

void loop() {

  server.handleClient();
  client.loop();  //MQTT Funktion

  if(timer+10000<millis()){
    timer = millis();
    client.publish("server/test", "hallo"); 
    client.publish("server/ssid", String(userData.ssid)); 
    client.publish("server/password", String(userData.password)); 
    client.publish("server/broker", String(userData.broker)); 
    client.publish("server/port", String(userData.mqttPort)); 
    client.publish("server/user", String(userData.mqttUser)); 
    client.publish("server/pswd", String(userData.mqttPswrd)); 
  }


}

void handlePortal() {

  
  if (server.method() == HTTP_POST) {
    char bufferStr[6] = {0};
    strncpy(userData.ssid,     server.arg("ssid").c_str(),     sizeof(userData.ssid) );
    strncpy(userData.password, server.arg("password").c_str(), sizeof(userData.password) );
    strncpy(userData.broker, server.arg("broker").c_str(), sizeof(userData.broker) );
    strncpy(bufferStr, server.arg("mqttPort").c_str(), sizeof(bufferStr) );
    strncpy(userData.mqttUser, server.arg("mqttUser").c_str(), sizeof(userData.mqttUser) );
    strncpy(userData.mqttPswrd, server.arg("mqttPswrd").c_str(), sizeof(userData.mqttPswrd) );
    userData.mqttPort = atoi(bufferStr);
    userData.ssid[server.arg("ssid").length()] = '\0';
    userData.password[server.arg("password").length()] = '\0';
    userData.broker[server.arg("broker").length()] = '\0';
    userData.mqttUser[server.arg("mqttUser").length()] = '\0';
    userData.mqttPswrd[server.arg("mqttPswrd").length()] = '\0';
    EEPROM.begin(512);
    EEPROM.put(40, userData);
    EEPROM.commit();
    EEPROM.end();

    server.send(200,   "text/html",  "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>Wifi Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1>Wifi Setup</h1> <br/> <p>Your settings have been saved successfully!<br />Please restart the device.<br />MQTT should now work. <br /> If you find the Acces Point network again, your credentials were wrong.</p></main></body></html>" );
  } else {

    server.send(200,   "text/html", "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>MBusino Setup</title><style>*,::after,::before{box-sizing:border-box}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#212529;background-color:#f5f5f5}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da}button{cursor:pointer;border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto}h1{text-align:center}</style></head><body><main class='form-signin'><form action='/' method='post'><h1 class=''>MBusino Setup</h1><br><div class='form-floating'><label>SSID</label><input type='text' class='form-control' name='ssid'></div><div class='form-floating'><label>Password</label><input type='password' class='form-control' name='password'></div><div class='form-floating'><label>MQTT Broker</label><input type='text' class='form-control' name='broker'></div><div class='form-floating'><label>MQTT Port</label><input type='text' value='1883' class='form-control' name='mqttPort'></div><div class='form-floating'><label>MQTT User (optional)</label><input type='text' class='form-control' name='mqttUser'></div><div class='form-floating'><label>MQTT Passwort (optional)</label><input type='password' class='form-control' name='mqttPswrd'></div><br><br><button type='submit'>Save</button><p style='text-align:right'><a href='https://www.github.com/zeppelin500/mbusino' style='color:#32c5ff'>MBusino</a></p></form></main></body></html>" );
  }
}