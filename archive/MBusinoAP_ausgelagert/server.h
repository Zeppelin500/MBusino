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

      request->send(200, "text/html", "<!doctype html><html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>MBusino Setup</title><style>*,::after,::before{box-sizing:border-box;}body{margin:0;font-family:'Segoe UI',Roboto,'Helvetica Neue',Arial,'Noto Sans','Liberation Sans';font-size:1rem;font-weight:400;line-height:1.5;color:#FFF;background-color:#438287;}.form-control{display:block;width:100%;height:calc(1.5em + .75rem + 2px);border:1px solid #ced4da;}button{border:1px solid transparent;color:#fff;background-color:#007bff;border-color:#007bff;padding:.5rem 1rem;font-size:1.25rem;line-height:1.5;border-radius:.3rem;width:100%}.form-signin{width:100%;max-width:400px;padding:15px;margin:auto;}h1,p{text-align: center}</style> </head> <body><main class='form-signin'> <h1><i>MBusino</i> Setup</h1> <br/> <p>Your settings have been saved successfully!<br />MBusino restart now!<br />MQTT should now work. <br /> If you find the Acces Point network again, your credentials were wrong.</p></main></body></html>");
      //request->send(200, "text/html", "The values entered by you have been successfully sent to the device <br><a href=\"/\">Return to Home Page</a>");
  });
}

void onRequest(AsyncWebServerRequest *request){
  //Handle Unknown Request
  request->send(404);
}

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