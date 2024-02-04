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