void reconnect() {
  // Loop until we're reconnected
  char lwBuffer[30] = {0};
  sprintf(lwBuffer, userData.mbusinoName, "/lastwill");
  if (client.connect(userData.mbusinoName,userData.mqttUser,userData.mqttPswrd,lwBuffer,0,false,"I am going offline")) {
    // Once connected, publish an announcement...
    Serial.println("MQTT connected to server");
    conCounter++;
    if(conCounter == 1){
      client.publish(String(String(userData.mbusinoName) + "/start").c_str(), "Bin hochgefahren, WLAN und MQTT steht");
    }
    else{
      client.publish(String(String(userData.mbusinoName) + "/reconnect").c_str(), "Online again!");
      adMbusMessageCounter = 2;
      adSensorMessageCounter = 2;
    }
    // ... and resubscribe
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateAverage").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateSensor").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateValue").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateBME").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/calibrateSet0").c_str());
    client.subscribe(String(String(userData.mbusinoName) + "/mbusPolling").c_str());
  }
  else{
    Serial.println("MQTT unable to connect server");
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
  if (strcmp(topic,String(String(userData.mbusinoName) + "/mbusPolling").c_str())==0){  
    polling = true;
  } 
}   