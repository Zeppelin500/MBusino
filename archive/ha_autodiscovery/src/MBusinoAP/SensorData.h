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