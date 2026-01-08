struct autodiscover {
  char bufferValue[500] = {0};
  char bufferTopic[100] = {0};
  char haName[30] = {0};
  char haUnits[30] = {0};
  char stateClass[30] = {0};
  char deviceClass[30] = {0};
  char deviceClassString[50] = {0};
} adVariables; // home assistand auto discover

const char bmeValue[4][12] = {"temperature","pressure","altitude","humidity"};
const char bmeDeviceClass[4][12] = {"temperature","pressure","distance","humidity"};
const char bmeUnits[4][5] = {"°C","mbar","m","%"};

const char adValueMbus[] PROGMEM = R"rawliteral({"unique_id":"%s_%u_%u_%s","default_entity_id":"sensor.%s_%u_%u_%s","state_topic":"%s/MBus/SlaveAddress%u/%u_%s","name":"Addr%u_%u_%s","value_template":"{{value_json if value_json is defined else 0}}","unit_of_meas":"%s","state_class":"%s","device":{"ids": ["%s"],"name":"%s","manufacturer": "MBusino","mdl":"V%s"},%s"availability_mode":"all"})rawliteral";
const char adTopicMbus[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/%u_%u_%s/config)rawliteral";

const char adValueSensor[] PROGMEM = R"rawliteral({"unique_id":"%s_Sensor%u","default_entity_id":"sensor.%s_Sensor%u","state_topic":"%s/OneWire/S%u","name":"Sensor%u","value_template":"{{value_json if value_json is defined else 0}}","unit_of_meas":"°C","state_class":"measurement","device":{"ids": ["%s"],"name":"%s","manufacturer": "MBusino","mdl":"V%s"},"device_class":"temperature","availability_mode":"all"})rawliteral";
const char adTopicSensor[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/Sensor%u/config)rawliteral";

const char adValueBME[] PROGMEM = R"rawliteral({"unique_id":"%s__BME_%s","default_entity_id":"sensor.%s_BME_%s","state_topic":"%s/bme/%s","name":"%s","value_template":"{{value_json if value_json is defined else 0}}","unit_of_meas":"%s","state_class":"measurement","device":{"ids": ["%s"],"name":"%s","manufacturer": "MBusino","mdl":"V%s"},"device_class":"%s","availability_mode":"all"})rawliteral";
const char adTopicBME[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/%s/config)rawliteral";

void haHandoverMbus(uint8_t haCounter, bool engelmann, uint8_t address){ // haCounter is the "i+1" at the for() in main

  if(adVariables.deviceClass[0] != 0){
    strcpy(adVariables.deviceClassString,String("\"device_class\": \"" + String(adVariables.deviceClass) + "\",").c_str());
  }
  sprintf(adVariables.bufferValue,adValueMbus,userData.mbusinoName,address,haCounter,adVariables.haName,userData.mbusinoName,address,haCounter,adVariables.haName,userData.mbusinoName,address,haCounter,adVariables.haName,address,haCounter,adVariables.haName,adVariables.haUnits,adVariables.stateClass,userData.mbusinoName,userData.mbusinoName,MBUSINO_VERSION,adVariables.deviceClassString);
  sprintf(adVariables.bufferTopic,adTopicMbus,userData.mbusinoName,address,haCounter,adVariables.haName);
  client.publish(adVariables.bufferTopic, adVariables.bufferValue, true); 

  if(haCounter == 4 && engelmann == true){  // Sensostar Bugfix --> comment it out if you use not a Sensostar   
    sprintf(adVariables.bufferValue,adValueMbus,userData.mbusinoName,address,haCounter,"power_calc",userData.mbusinoName,address,haCounter,"power_calc",userData.mbusinoName,address,haCounter,"power_calc",address,haCounter,"power_calc",adVariables.haUnits,adVariables.stateClass,userData.mbusinoName,userData.mbusinoName,MBUSINO_VERSION,adVariables.deviceClassString);
    sprintf(adVariables.bufferTopic,adTopicMbus,userData.mbusinoName,address,haCounter,"power_calc");
    client.publish(adVariables.bufferTopic, adVariables.bufferValue, true);                   
  } 

  adVariables.bufferTopic[0] = 0;
  adVariables.bufferValue[0] = 0;
  adVariables.deviceClass[0] = 0;
  adVariables.deviceClassString[0] = 0;
  adVariables.stateClass[0] = 0;
  adVariables.haUnits[0] = 0;
}
