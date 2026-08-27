#pragma once

#include <Arduino.h>
#include <MqttClient.h>

class MqttManager
{
public:
    MqttManager();

    void begin();
    void loop();

    void onConnected();
    void onDisconnect();
    void publishClaim(const char *payload);
    void publishState(const char *payload);
    void publishChangeApply(const char *payload);

    void setMessageCallback(MessageCallback callback);

private:
    MqttClient *_mqttClient;
    const char *mqtt_broker = "mqtt.eclipse.org";
    const char *client_id = "monitoring123";
    const char *mqtt_username = "";
    const char *mqtt_password = "";
    MessageCallback _messageCallback;
};