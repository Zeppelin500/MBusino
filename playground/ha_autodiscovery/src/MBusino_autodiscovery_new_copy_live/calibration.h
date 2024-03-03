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