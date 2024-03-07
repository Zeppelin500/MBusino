struct autodiscover {
  char bufferValue[500] = {0};
  char bufferTopic[100] = {0};
  char haUnits[20] = {0};
  char stateClass[30] = {0};
  char deviceClass[40] = {0};
  char valueString[4] = {0};

} adVariables; // home assistand auto discover

const char bmeValue[4][20] = {"Temperatur","Druck","Hoehe","Feuchte"};
const char bmeDeviceClass[4][20] = {"temperature","pressure","distance","humidity"};
const char bmeUnits[4][11] = {"°C","mbar","m","%"};

//const char vs[4] = "_vs"; //placeholder for insert "_vs" for valuestring is used instead of value
//const char[4] noVs = {0}; //empty, no Valuestring

const char adValueMbus[] PROGMEM = R"rawliteral({"unique_id":"%s_%u_%s","object_id":"%s_%u_%s","state_topic":"%s/MBus/%u%s_%s","name":"%u_%s","value_template":"{{value_json if value_json is defined else 0}}","unit_of_meas":"%s","state_class":"%s","device":{"ids": ["%s"],"name":"%s","manufacturer": "MBusino","mdl":"V%s"},%s"availability_mode":"all"})rawliteral";
const char adTopicMbus[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/%u_%s/config)rawliteral";

const char adValueSensor[] PROGMEM = R"rawliteral({"unique_id":"%s_Sensor%u","object_id":"%s_Sensor%u","state_topic":"%s/OneWire/S%u","name":"Sensor%u","value_template":"{{value_json if value_json is defined else 0}}","unit_of_meas":"°C","state_class":"measurement","device":{"ids": ["%s"],"name":"%s","manufacturer": "MBusino","mdl":"V%s"},"device_class":"temperature","availability_mode":"all"})rawliteral";
const char adTopicSensor[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/Sensor%u/config)rawliteral";

const char adValueBME[] PROGMEM = R"rawliteral({"unique_id":"%s__BME_%s","object_id":"%s_BME_%s","state_topic":"%s/bme/%s","name":"%s","value_template":"{{value_json if value_json is defined else 0}}","unit_of_meas":"%s","state_class":"measurement","device":{"ids": ["%s"],"name":"%s","manufacturer": "MBusino","mdl":"V%s"},"device_class":"%s","availability_mode":"all"})rawliteral";
const char adTopicBME[] PROGMEM = R"rawliteral(homeassistant/sensor/%s/%s/config)rawliteral";

void haHandoverMbus(uint8_t haCounter, const char* haName, const char* units, const char* valueString, const char* deviceClass, const char* stateClass, bool engelmann){ // haCounter is the "i+1" at the for() in main

  if(units != NULL){
    strcpy(adVariables.haUnits,units);//getUnitOfMeassuremetn(units));
  }else{
    strcpy(adVariables.haUnits,""); 
  }
    strcpy(adVariables.stateClass,stateClass);

    if(deviceClass[0] != 0){
      strcpy(adVariables.deviceClass,String("\"device_class\": \"" + String(deviceClass) + "\",").c_str());
    }
    //if(units[1]==89){ //if the secon caracter "Y" no unit will be submitted.
      //strcpy(adVariables.haUnits,"");
      //strcpy(adVariables.valueString,"_vs"); //set "_vs" befor the value name     
    //}
  //}
  /*else{
  strcpy(adVariables.haUnits,"");
  strcpy(adVariables.stateClass,"total");
  strcpy(adVariables.deviceClass,"");  
  }*/

  if(units[1]==89){

  }

  sprintf(adVariables.bufferValue,adValueMbus,userData.mbusinoName,haCounter,haName,userData.mbusinoName,haCounter,haName,userData.mbusinoName,haCounter,adVariables.valueString,haName,haCounter,haName,adVariables.haUnits,adVariables.stateClass,userData.mbusinoName,userData.mbusinoName,MBUSINO_VERSION,adVariables.deviceClass);
  sprintf(adVariables.bufferTopic,adTopicMbus,userData.mbusinoName,haCounter,haName);
  client.publish(adVariables.bufferTopic, adVariables.bufferValue); 

  if (haCounter == 4 && engelmann == true){  // Sensostar Bugfix --> comment it out if you use not a Sensostar   
    sprintf(adVariables.bufferValue,adValueMbus,userData.mbusinoName,haCounter,"power_calc",userData.mbusinoName,haCounter,"power_calc",userData.mbusinoName,haCounter,adVariables.valueString,"power_calc",haCounter,"power_calc",adVariables.haUnits,adVariables.stateClass,userData.mbusinoName,userData.mbusinoName,MBUSINO_VERSION,adVariables.deviceClass);
    sprintf(adVariables.bufferTopic,adTopicMbus,userData.mbusinoName,haCounter,"power_calc");
    client.publish(adVariables.bufferTopic, adVariables.bufferValue);                   
  } 

  adVariables.bufferTopic[0] = 0;
  adVariables.bufferValue[0] = 0;
  adVariables.deviceClass[0] = 0;
  adVariables.stateClass[0] = 0;
  adVariables.haUnits[0] = 0;
  adVariables.valueString[0] = 0;
}

void haHandoverOw(uint8_t haCounter){
  sprintf(adVariables.bufferValue,adValueSensor,userData.mbusinoName,haCounter,userData.mbusinoName,haCounter,userData.mbusinoName,haCounter,haCounter,userData.mbusinoName,userData.mbusinoName,MBUSINO_VERSION);
  sprintf(adVariables.bufferTopic,adTopicSensor,userData.mbusinoName,haCounter);
  client.publish(adVariables.bufferTopic, adVariables.bufferValue); 
  adVariables.bufferTopic[0] = 0;
  adVariables.bufferValue[0] = 0;
}

void haHandoverBME(){
  for(uint8_t i=0; i<4;i++){  
    sprintf(adVariables.bufferValue,adValueBME,userData.mbusinoName,bmeValue[i],userData.mbusinoName,bmeValue[i],userData.mbusinoName,bmeValue[i],bmeValue[i],bmeUnits[i],userData.mbusinoName,userData.mbusinoName,MBUSINO_VERSION,bmeDeviceClass[i]);
    sprintf(adVariables.bufferTopic,adTopicBME,userData.mbusinoName,bmeValue[i]);
    client.publish(adVariables.bufferTopic, adVariables.bufferValue); 
    adVariables.bufferTopic[0] = 0;
    adVariables.bufferValue[0] = 0;
  }
}



/*
char* getStateClass(const char* unit) {
  if (this->unit == "C") {
      return "measurement";
  }else if (this->unit == "K") {
      return "measurement";
  }else if (this->unit == "W") {
      return "measurement";
  }else if(this->unit =="Wh") {
      return "total";
  }else if(this->unit=="kWh"){
      return "total";
  }else if(this->unit=="m3") {
      return "total";
  }else if(this->unit =="m3/h"){
      return "measurement";
  } else {
      return String("");
  }
}

String getDeviceClass() {
    if (this->unit == "C") {
        return "temperature";
    }else if (this->unit == "K") {
        return "temperature";
    }else if (this->unit == "W") {
        return "power";
    }else if(this->unit =="Wh") {
        return "energy";
    }else if(this->unit=="kWh"){
        return "energy";
    }else if(this->unit=="m3") {
        return "water";
    }else if(this->unit =="m3/h"){
        return String("");
    } else {
        return String("");
    }
}


///######## Device.h #####################################
#include <vector>
class Device {
public:

    Device() = default;
    String toJsonString(String mbusinoName) const {
        String oss;
        oss +=  "{";
        oss +=   "\"ids\":  [";
        oss +=    "\""+mbusinoName+"\"";
        oss +=    "],";
        oss +=    "\"name\": \""+mbusinoName+"\",";
        oss +=    "\"mdl\": \"MBusino v.070\"";
        oss += "}";
        return oss;
    }
};

///######## MBusTopic.h #####################################
#include "SensorData.h"

class MBusTopic {
public:
    String topic;
    String value;
    String unit;
    String name;

    MBusTopic(String t) : topic(t), value(""), unit("") {
        name = getLastTopicPart(t);
    }

    void setValue(String v) {
        value = v;
    }

    void setUnit(String u) {
        unit = u;
    }

private:
    const char *getLastTopicPart(String curTopic) {
        const char *lastSlash = strrchr(curTopic.c_str(), '/'); // Suche den letzten Slash

        if (lastSlash != nullptr) {
            // Rückgabe des Zeigers auf den Anfang des letzten Teils des Topics
            return lastSlash + 1;
        } else {
            // Wenn kein Slash gefunden wurde, bedeutet das, dass der ganze String das Topic ist
            return curTopic.c_str();
        }
    }

    bool isValid() {
        if (this->value != nullptr && this->unit != nullptr) {
            return true;
        } else {
            return false;
        }
    }

private:
    String getUnitOfMeassuremetn() {
        if (this->unit == "C") {
            return "°C";
        }else if (this->unit == "K") {
            return "K";
        }else if (this->unit == "W") {
            return "W";
        }else if(this->unit =="Wh") {
            return "Wh";
        }else if(this->unit=="kWh"){
            return "kWh";
        }else if(this->unit=="m3") {
            return "m³";
        }else if(this->unit =="m3/h"){
            return "m³/h";
        } else {
            return String("");
        }
    }
private:
    String getStateClass() {
        if (this->unit == "C") {
            return "measurement";
        }else if (this->unit == "K") {
            return "measurement";
        }else if (this->unit == "W") {
            return "measurement";
        }else if(this->unit =="Wh") {
            return "total";
        }else if(this->unit=="kWh"){
            return "total";
        }else if(this->unit=="m3") {
            return "total";
        }else if(this->unit =="m3/h"){
            return "measurement";
        } else {
            return String("");
        }
    }
private:
    String getDeviceClass() {
        if (this->unit == "C") {
            return "temperature";
        }else if (this->unit == "K") {
            return "temperature";
        }else if (this->unit == "W") {
            return "power";
        }else if(this->unit =="Wh") {
            return "energy";
        }else if(this->unit=="kWh"){
            return "energy";
        }else if(this->unit=="m3") {
            return "water";
        }else if(this->unit =="m3/h"){
            return String("");
        } else {
            return String("");
        }
    }


public:
    SensorData toSensorData(String basic) {
        SensorData sensorData;
        sensorData.unique_id = basic + "_" + this->name;
        sensorData.object_id = basic + "_" + this->name;
        sensorData.state_topic = this->topic;
        sensorData.name = this->name;
        sensorData.value_template = "{{value_json if value_json is defined else 0}}";
        sensorData.unit_of_meas = getUnitOfMeassuremetn();
        sensorData.state_class = getStateClass();
        sensorData.device_class = getDeviceClass();
        sensorData.availability_mode = "all";
        return sensorData;
    }
};

///######## MBusTopicService.h #####################################
#include "SensorData.h"

class MBusTopic {
public:
    String topic;
    String value;
    String unit;
    String name;

    MBusTopic(String t) : topic(t), value(""), unit("") {
        name = getLastTopicPart(t);
    }

    void setValue(String v) {
        value = v;
    }

    void setUnit(String u) {
        unit = u;
    }

private:
    const char *getLastTopicPart(String curTopic) {
        const char *lastSlash = strrchr(curTopic.c_str(), '/'); // Suche den letzten Slash

        if (lastSlash != nullptr) {
            // Rückgabe des Zeigers auf den Anfang des letzten Teils des Topics
            return lastSlash + 1;
        } else {
            // Wenn kein Slash gefunden wurde, bedeutet das, dass der ganze String das Topic ist
            return curTopic.c_str();
        }
    }

    bool isValid() {
        if (this->value != nullptr && this->unit != nullptr) {
            return true;
        } else {
            return false;
        }
    }

private:
    String getUnitOfMeassuremetn() {
        if (this->unit == "C") {
            return "°C";
        }else if (this->unit == "K") {
            return "K";
        }else if (this->unit == "W") {
            return "W";
        }else if(this->unit =="Wh") {
            return "Wh";
        }else if(this->unit=="kWh"){
            return "kWh";
        }else if(this->unit=="m3") {
            return "m³";
        }else if(this->unit =="m3/h"){
            return "m³/h";
        } else {
            return String("");
        }
    }
private:
    String getStateClass() {
        if (this->unit == "C") {
            return "measurement";
        }else if (this->unit == "K") {
            return "measurement";
        }else if (this->unit == "W") {
            return "measurement";
        }else if(this->unit =="Wh") {
            return "total";
        }else if(this->unit=="kWh"){
            return "total";
        }else if(this->unit=="m3") {
            return "total";
        }else if(this->unit =="m3/h"){
            return "measurement";
        } else {
            return String("");
        }
    }
private:
    String getDeviceClass() {
        if (this->unit == "C") {
            return "temperature";
        }else if (this->unit == "K") {
            return "temperature";
        }else if (this->unit == "W") {
            return "power";
        }else if(this->unit =="Wh") {
            return "energy";
        }else if(this->unit=="kWh"){
            return "energy";
        }else if(this->unit=="m3") {
            return "water";
        }else if(this->unit =="m3/h"){
            return String("");
        } else {
            return String("");
        }
    }


public:
    SensorData toSensorData(String basic) {
        SensorData sensorData;
        sensorData.unique_id = basic + "_" + this->name;
        sensorData.object_id = basic + "_" + this->name;
        sensorData.state_topic = this->topic;
        sensorData.name = this->name;
        sensorData.value_template = "{{value_json if value_json is defined else 0}}";
        sensorData.unit_of_meas = getUnitOfMeassuremetn();
        sensorData.state_class = getStateClass();
        sensorData.device_class = getDeviceClass();
        sensorData.availability_mode = "all";
        return sensorData;
    }
};
//######################### SensorData.h ###############################################################
#include <vector>
#include "Device.h"

class SensorData {
public:
    String unique_id;
    String object_id;
    String state_topic;
    String name;
    String value_template;
    String unit_of_meas;
    String state_class;
    Device device;
    String availability_mode;
    String device_class;

    SensorData() = default;

public:
    String toJsonString(String mbusinoName) const {
        String oss;
        oss += "{";
        oss += "\"unique_id\":\"" + unique_id + "\",";
        oss += "\"object_id\":\"" + object_id + "\",";
        oss += "\"state_topic\":\"" + state_topic + "\",";
        oss += "\"name\":\"" + name + "\",";
        oss += "\"value_template\":\"" + value_template + "\",";
        if (!unit_of_meas.isEmpty()) {
            oss += "\"unit_of_meas\":\"" + unit_of_meas + "\",";
        }
        // checking state_class for null and empty, if nothing is present then no entry is made in JSON
        if (!state_class.isEmpty()) {
            oss += "\"state_class\":\"" + state_class + "\",";
        }
        oss += "\"device\":" + device.toJsonString(mbusinoName) + ",";
        if (!device_class.isEmpty()) {
            oss += "\"device_class\":\"" + device_class+ "\",";
        }
        oss += "\"availability_mode\":\"" + availability_mode + "\"";

        oss += "}";
        return oss;
    }

};
*/
