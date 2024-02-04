#include <vector>
#include "MBusTopic.h"
#include "PubSubClient.h"
#include <regex>
#include <WString.h>
#include <Arduino.h>

class MBusTopicService {
private:
    std::vector<MBusTopic *> topics;
    PubSubClient *client;
    String mbusinoName;

public:
    MBusTopicService(PubSubClient &mqttClient, String mbusinoName) : client(&mqttClient), mbusinoName("mbusinoName") {}


    void subscribeToTopic(String mbusinoName) {
        this->mbusinoName = mbusinoName;
        // Abonniere das Basis-Topic
        String subscriptionPattern = this->mbusinoName + "/MBus/#";
        client->subscribe(subscriptionPattern.c_str());

        // Callback-Funktion für MQTT-Nachrichten
        client->setCallback([this](char *topic, byte *payload, unsigned int length) {
            callback(topic, payload, length);
        });
    }

    void callback(const char *topic, const byte *payload, unsigned int length) {
        String topicStr = String(topic);
        String payloadStr = String((char *) payload).substring(0, length);
        // Überprüfe, ob es sich um ein Einheit-Topic handelt
        if (topicStr.endsWith("_unit")) {
            // Finde das entsprechende Topic-Objekt und setze die Einheit
            String baseTopic = topicStr.substring(0, topicStr.lastIndexOf("_unit"));
            for (auto &t: topics) {
                if (t->topic == baseTopic) {
                    t->setUnit(payloadStr);
                    String haConfigTopic= "homeassistant/sensor/"+this->mbusinoName+"/"+t->name+"/config";
                    client->publish(haConfigTopic.c_str(),
                                    t->toSensorData(this->mbusinoName).toJsonString(this->mbusinoName).c_str());
                    return;
                }
            }
        } else {
            // Überprüfe, ob das Topic schon existiert, und aktualisiere es
            bool found = false;
            for (auto &t: topics) {
                if (t->topic == topicStr) {
                    t->setValue(payloadStr);
                    found = true;
                    break;
                }
            }
            // Wenn nicht, erstelle eine neue Instanz
            if (!found) {
                subscribeToUnitTopic(topicStr, payloadStr);
            }
        }
    }

    void subscribeToUnitTopic(const String &topicStr, const String &payloadStr) {
        MBusTopic *newTopic = new MBusTopic(topicStr);
        newTopic->setValue(payloadStr);
        topics.push_back(newTopic);
        // Abonnieren des zugehörigen Einheit-Topics
        String unitTopic = topicStr + "_unit";
        client->subscribe(unitTopic.c_str());
    }
};
