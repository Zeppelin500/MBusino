void reconnect() {
  // Loop until we're reconnected
  Serial.println("MQTT try to connect server");  
  char lwBuffer[40] = {0};
  snprintf(lwBuffer, sizeof(lwBuffer), "%s/status", userData.mbusinoName);
  if (client.connect(userData.mbusinoName,userData.mqttUser,userData.mqttPswrd,lwBuffer,1,true,"offline")) {
    // Once connected, publish an announcement...
    Serial.println("MQTT connected to server");
    digitalWrite(LED_BUILTIN,LOW);
    conCounter++;
    if(conCounter == 1){
      client.publish(String(String(userData.mbusinoName) + "/start").c_str(), "Bin hochgefahren, WLAN und MQTT steht");
    }
    else{
      client.publish(String(String(userData.mbusinoName) + "/reconnect").c_str(), "Online again!");
      adMbusMessageCounter = 2;
    }
    // publish online status for HA availability (retained)
    client.publish(lwBuffer, "online", true);
    // ... and resubscribe
    client.subscribe(String(String(userData.mbusinoName) + "/mbusPolling").c_str());
  }
  else{
    Serial.println("MQTT unable to connect server");
    digitalWrite(LED_BUILTIN,HIGH);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic,String(String(userData.mbusinoName) + "/mbusPolling").c_str())==0){  
    polling = true;
  } 
}   