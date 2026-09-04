#pragma once
#include <Arduino.h>
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"

enum class ClaimStep
{
    IDLE,
    WAIT_MQTT,
    PUBLISHING,
    WAIT_ACK,
    DONE
};

class Claim
{
public:
    Claim(WiFiManager &wifiManager, MqttManager &mqttManager);

    void loop();

    ClaimStep getStep() const;
    bool isDone() const;

    void reset();

private:
    void handleMqttMessage(String &topic, String &payload);
    void buildPayload();
    static Claim *_instance;
    static void onMqttMessage(String &topic, String &payload);

    WiFiManager &_wifiManager;
    MqttManager &_mqttManager;

    ClaimStep _step;
    String _pendingPayload;
    String _pendingDeviceId;
    unsigned long _claimSentAt;

    static const unsigned long CLAIM_ACK_TIMEOUT_MS = 6000;
};