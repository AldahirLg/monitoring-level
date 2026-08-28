#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <MQTTClient.h>

typedef void (*MessageCallback)(String &topic, String &payload);

class MqttManager
{
public:
    MqttManager();

    void begin();
    void loop(bool wifiConncted);

    void onConnected();
    void reconnect();
    void disconnect();
    void publishClaim(const char *payload);
    void publishState(const char *payload);
    void publishChangeApply(const char *payload);
    bool isConnected();
    void setMessageCallback(MessageCallback callback);

private:
    WiFiClient _netClient;
    MQTTClient _mqttClient;

    const char *mqtt_broker = "test.mosquitto.org";
    const int mqtt_port = 1883;
    const char *client_id = "monitoring123";
    const char *mqtt_username = "";
    const char *mqtt_password = "";

    MessageCallback _messageCallback = nullptr;
    uint8_t _attempConnection = 0;
    unsigned long _lastReconnectAttempt = 0;
};