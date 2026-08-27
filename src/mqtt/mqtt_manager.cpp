#include "mqtt_manager.h"

MqttManager::MqttManager()
    : _mqttClient(MqttClient::getInstance())
{
}

void MqttManager::begin()
{
    _mqttClient->begin(mqtt_broker);
    _mqttClient->connect(client_id);
    _mqttClient->onConnect([this]()
                           { onConnected(); });
    Serial.println("[MQTT] Iniciado");
}

void MqttManager::loop()
{
}

void MqttManager::onConnected()
{
    _mqttClient->subscribe("claim/result", 1);
    _mqttClient->subscribe("monitoring_level/response", 1);
}

void MqttManager::publishClaim(const char *payload)
{
    const char *topic = "claim/";
    _mqttClient->publish(topic, payload, false);
}

void MqttManager::publishState(const char *payload)
{
    const char *topic = "level_monitoring/";
    _mqttClient->publish(topic, payload, false);
}

void MqttManager::publishChangeApply(const char *payload)
{
    const char *topic = "level_monitoring/apply";
    _mqttClient->publish(topic, payload, false);
}

void MqttManager::setMessageCallback(MessageCallback callback)
{
    _messageCallback = callback;
    _mqttClient->onMessage(_messageCallback);
}