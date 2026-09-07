#include <Arduino.h>
#include "sensor/sensor.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"
#include "ble/ble_manager.h"
#include "claim/claim.h"
#include <ArduinoJson.h>
#include "esp_wifi.h"
#include "esp_sleep.h"

enum State
{
    PROVISIONING,
    CLAIM,
    CONNECTION,
    AUTO,
    SLEEP,
};

Sensor sensor(14, 13);
WiFiManager wifiManager;
BleManager bleManager(wifiManager);
MqttManager mqttManager;
Claim claimHandler(wifiManager, mqttManager);

State state;
bool psConfigured = false;
uint8_t resetPin = 4;

String deviceId;

void resetCredentials()
{
    if (digitalRead(resetPin) == LOW)
    {
        wifiManager.resetSettings();
        delay(1000);
        ESP.restart();
    }
}

void handleMqttMessage(String &topic, String &payload)
{
    Serial.printf("[MQTT] Mensaje en [%s]: %s\n", topic.c_str(), payload.c_str());
}

void autoMode()
{
    sensor.loop();
    mqttManager.loop(wifiManager.isConnected());

    if (sensor.state() == SensorMode::DONE)
    {
        JsonDocument doc;

        doc["level"] = sensor.getDistance();
        doc["sensor_state"] = sensor.sensorState();
        doc["batery"] = 100;

        String payload;

        serializeJson(doc, payload);
        if (mqttManager.isConnected())
        {
            mqttManager.publishState(payload.c_str(), deviceId.c_str());
        }
        sensor.reset();
        state = State::SLEEP;
    }
}

void connection()
{
    if (!wifiManager.isConnected())
    {
        wifiManager.begin();
    }

    if (wifiManager.isConnected() && !psConfigured)
    {
        psConfigured = true;
    }

    if (wifiManager.isConnected() && !mqttManager.isConnected())
    {
        String deviceId = wifiManager.getDeviceUid();

        mqttManager.setDeviceId(deviceId);
        mqttManager.setMessageCallback(handleMqttMessage);
        mqttManager.begin();
    }

    state = State::AUTO;
}

void modeSleep()
{

    if (mqttManager.isConnected())
    {
        mqttManager.disconnect();
    }
    WiFi.disconnect();
    delay(50);
    esp_wifi_stop();
    delay(50);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)resetPin, 0);
    esp_sleep_enable_timer_wakeup(5 * 1000000);
    esp_light_sleep_start();
    esp_wifi_start();
    delay(50);
    resetCredentials();
    state = State::CONNECTION;
}

void setup()
{
    Serial.begin(115200);
    pinMode(resetPin, INPUT_PULLUP);
    wifiManager.begin();
    if (wifiManager.getStatus() == WiFiManagerStatus::CONNECTED)
    {
        state = State::CONNECTION;
        deviceId = wifiManager.getDeviceUid();
    }
    else if (wifiManager.getStatus() == WiFiManagerStatus::DISCONNECTED)
    {
        state = State::SLEEP;
    }
    else if (wifiManager.getStatus() == WiFiManagerStatus::PROVISIONING)
    {
        state = State::PROVISIONING;
        bleManager.begin();
    }
}

void loop()
{
    switch (state)
    {
    case State::CONNECTION:
        connection();
        break;
    case State::AUTO:
        autoMode();
        break;
    case State::SLEEP:
        modeSleep();
        break;
    case State::PROVISIONING:
        bleManager.loop();
        wifiManager.loop();
        if (wifiManager.getStatus() == WiFiManagerStatus::CONNECTED)
        {
            state = State::CLAIM;
        }
        break;
    case State::CLAIM:
        bleManager.loop();
        wifiManager.loop();
        claimHandler.loop();
        mqttManager.loop(wifiManager.isConnected());
        if (claimHandler.isDone())
        {
            claimHandler.reset();
            state = State::CONNECTION;
        }
        break;
    default:
        break;
    }
}