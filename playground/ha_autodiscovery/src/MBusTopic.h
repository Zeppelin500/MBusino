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
