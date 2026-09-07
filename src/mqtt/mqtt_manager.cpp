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

    _netClient.setInsecure();

    _mqttClient.begin(
        mqtt_broker,
        mqtt_port,
        _netClient);

    if (_messageCallback != nullptr)
    {
        _mqttClient.onMessage(_messageCallback);
    }

    _hasBegun = true;

    reconnect();

    Serial.println("[MQTT] Iniciado");
}

void MqttManager::loop(bool wifiConnected)
{
    if (!_hasBegun)
        return;

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

void MqttManager::reconnect()
{
    if (!_hasBegun)
        return;

    Serial.print("[MQTT] Conectando...");

    if (_mqttClient.connect(
            client_id,
            mqtt_username,
            mqtt_password))
    {
        Serial.println(" conectado");

        _attempConnection = 0;

        onConnected();
    }
    else
    {
        _attempConnection++;

        Serial.print(" fallo. Error: ");
        Serial.println(_mqttClient.lastError());
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

void MqttManager::onConnected()
{
    String claimResultTopic =
        "claim/" + _deviceId + "/result";

    _mqttClient.subscribe(
        claimResultTopic.c_str(),
        1);

    Serial.printf(
        "[MQTT] Suscrito a: %s\n",
        claimResultTopic.c_str());

    String responseResultTopic =
        "medidor/" + _deviceId + "/response";

    _mqttClient.subscribe(
        responseResultTopic.c_str(),
        1);

    Serial.printf(
        "[MQTT] Suscrito a: %s\n",
        responseResultTopic.c_str());
}

bool MqttManager::publishClaim(
    const char *payload,
    const char *deviceId)
{
    String topic = "claim/";
    topic += deviceId;

    return _mqttClient.publish(
        topic.c_str(),
        payload,
        false,
        1);
}

void MqttManager::publishState(
    const char *payload,
    const char *deviceId)
{
    String topic = "medidor/";
    topic += deviceId;

    Serial.println(payload);

    _mqttClient.publish(
        topic,
        payload,
        false,
        1);
}

void MqttManager::publishChangeApply(
    const char *payload)
{
    const char *topic = "medidor/apply";

    _mqttClient.publish(
        topic,
        payload,
        false,
        0);
}

void MqttManager::setDeviceId(String deviceId)
{
    _deviceId = deviceId;
}

void MqttManager::setMessageCallback(
    MessageCallback callback)
{
    _messageCallback = callback;

    _mqttClient.onMessage(
        _messageCallback);
}