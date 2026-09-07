#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>

#include "secrets.h"

typedef void (*MessageCallback)(String &topic, String &payload);

class MqttManager
{
public:
    MqttManager();

    void begin();
    void loop(bool wifiConnected);

    void onConnected();
    void reconnect();
    void disconnect();

    bool publishClaim(const char *payload, const char *deviceId);
    void publishState(const char *payload, const char *deviceId);
    void publishChangeApply(const char *payload);

    bool isConnected();

    void setMessageCallback(MessageCallback callback);
    void setDeviceId(String deviceId);

private:
    WiFiClientSecure _netClient;
    MQTTClient _mqttClient;

    const char *mqtt_broker = MQTT_BROKER;
    const int mqtt_port = MQTT_PORT;
    const char *client_id = MQTT_CLIENT_ID;
    const char *mqtt_username = MQTT_USERNAME;
    const char *mqtt_password = MQTT_PASSWORD;

    MessageCallback _messageCallback = nullptr;

    uint8_t _attempConnection = 0;
    unsigned long _lastReconnectAttempt = 0;

    String _deviceId;
    bool _hasBegun = false;
};