#include "mqtt_manager.h"

MqttManager::MqttManager()
{
}

void MqttManager::begin()
{
    if (_netClient.connected())
    {
        _netClient.stop();
    }

    _mqttClient.begin(mqtt_broker, mqtt_port, _netClient);

    if (_messageCallback != nullptr)
    {
        _mqttClient.onMessage(_messageCallback);
    }

    reconnect();
    Serial.println("[MQTT] Iniciado");
}

void MqttManager::reconnect()
{
    Serial.print("[MQTT] Conectando...");

    if (_mqttClient.connect(client_id, mqtt_username, mqtt_password))
    {
        Serial.println(" conectado");
        onConnected();
    }
    else
    {
        _attempConnection++;
        Serial.println(" fallo");
    }
}

void MqttManager::disconnect()
{
    _mqttClient.disconnect();
}

bool MqttManager::isConnected()
{
    return _mqttClient.connected();
}

void MqttManager::loop(bool wifiConnected)
{

    _mqttClient.loop();

    if (wifiConnected && !isConnected())
    {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 5000)
        {
            _lastReconnectAttempt = now;
            reconnect();
        }
    }
}

void MqttManager::onConnected()
{
    _mqttClient.subscribe("claim/result", 1);
    _mqttClient.subscribe("monitoring_level/response", 1);
}

void MqttManager::publishClaim(const char *payload)
{
    const char *topic = "claim/";
    _mqttClient.publish(topic, payload, false, 0);
}

void MqttManager::publishState(const char *payload)
{
    const char *topic = "level_monitoring/";
    Serial.println(payload);
    _mqttClient.publish(topic, payload, false, 1);
}

void MqttManager::publishChangeApply(const char *payload)
{
    const char *topic = "level_monitoring/apply";
    _mqttClient.publish(topic, payload, false, 0);
}

void MqttManager::setMessageCallback(MessageCallback callback)
{
    _messageCallback = callback;
    _mqttClient.onMessage(_messageCallback);
}