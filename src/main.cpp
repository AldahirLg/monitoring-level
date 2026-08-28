#include <Arduino.h>
#include "sensor/sensor.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"
#include "ble/ble_manager.h"
#include "monitoring/monitoring.h"
#include <ArduinoJson.h>
#include "esp_wifi.h"
#include "esp_sleep.h"

enum State
{
    PROVISIONING,
    CONNECTION,
    AUTO,
    SLEEP,
};

Sensor sensor(14, 13);
WiFiManager wifiManager;
BleManager bleManager(wifiManager);
MqttManager mqttManager;
Monitoring monitoring(sensor);

State state;
bool psConfigured = false;

void autoMode()
{
    monitoring.loop();
    mqttManager.loop(wifiManager.isConnected());
    if (monitoring.state() == MonitorSate::END)
    {
        JsonDocument doc;

        doc["level"] = monitoring.getPercent();
        doc["sensor_state"] = monitoring.getSensorState();
        doc["batery"] = 100;

        String payload;

        serializeJson(doc, payload);
        if (mqttManager.isConnected())
        {
            mqttManager.publishState(payload.c_str());
        }
        monitoring.reset();
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
        // esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        psConfigured = true;
    }

    if (wifiManager.isConnected() && !mqttManager.isConnected())
    {
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
    esp_sleep_enable_timer_wakeup(5 * 1000000);
    esp_light_sleep_start();
    esp_wifi_start();
    delay(50);
    state = State::CONNECTION;
}

void setup()
{
    Serial.begin(115200);
    wifiManager.begin();

    if (wifiManager.getStatus() == WiFiManagerStatus::CONNECTED)
    {
        state = State::CONNECTION;
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
    default:
        break;
    }
}
