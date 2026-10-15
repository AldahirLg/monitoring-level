#include <Arduino.h>
#include "sensor/sensor.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"
#include "ble/ble_manager.h"
#include "claim/claim.h"
#include <ArduinoJson.h>
#include "esp_wifi.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include <math.h>

enum State
{
    PROVISIONING,
    CLAIM,
    CONNECTION,
    AUTO,
    SLEEP,
    MEASURE
};

const uint64_t SLEEP_HALF_US = 12500000ULL; // 12.5 s x 2 = 25 s
const unsigned long PROVISIONING_TIMEOUT_MS = 3UL * 60UL * 1000UL;

Sensor sensor(14, 13);
WiFiManager wifiManager;
BleManager bleManager(wifiManager);
MqttManager mqttManager;
Claim claimHandler(wifiManager, mqttManager);

State state;
bool psConfigured = false;
uint8_t resetPin = 25;
uint8_t cycleStep = 1; // 0 = próximo despertar solo mide, 1 = mide y envía
unsigned long provisioningStart = 0;

String deviceId;

// Batería
#define BATTERY_PIN 32
const float VREF = 3.3f;
const int ADC_MAX = 4095;

int calculateBatteryPercentage()
{
    int readADC = analogRead(BATTERY_PIN);

    float volt = (readADC * VREF) / ADC_MAX;
    float mV = volt * 1000.0f;

    float norm = (mV - 2400.0f) / (3200.0f - 2400.0f);
    norm = constrain(norm, 0.0f, 1.0f);

    int percentage = (int)(pow(norm, 1.7f) * 100.0f);

    /*Serial.printf(
        "[BATTERY] ADC: %d | Voltaje: %.2f V | Batería: %d%%\n",
        readADC, volt, percentage
    );*/

    return percentage;
}

void resetCredentials()
{
    if (digitalRead(resetPin) == LOW)
    {
        wifiManager.resetSettings();
        delay(1000);
        ESP.restart();
    }
}

void provisioningTimeoutSleep()
{
    Serial.println("[PROV] Timeout, entrando a deep sleep");
    // bleManager.end();   // apaga BLE si tu clase lo permite
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // El INPUT_PULLUP digital se pierde en deep sleep: hay que usar el pull-up RTC
    rtc_gpio_pullup_en((gpio_num_t)resetPin);
    rtc_gpio_pulldown_dis((gpio_num_t)resetPin);

    esp_sleep_enable_ext0_wakeup((gpio_num_t)resetPin, 0);
    esp_deep_sleep_start();
}

void handleMqttMessage(String &topic, String &payload)
{
    Serial.printf("[MQTT] Mensaje en [%s]: %s\n", topic.c_str(), payload.c_str());
}

// Medición intermedia: sin WiFi ni MQTT, no se publica nada
void measureOnly()
{
    sensor.loop();
    if (sensor.state() == SensorMode::DONE)
    {
        sensor.reset();
        cycleStep = 1;
        state = State::SLEEP;
    }
}

void autoMode()
{
    sensor.loop();
    mqttManager.loop(wifiManager.isConnected());

    if (sensor.state() == SensorMode::DONE)
    {
        JsonDocument doc;

        doc["status"]["level"] = sensor.getDistance();
        doc["status"]["sensorState"] = sensor.sensorState();
        doc["status"]["battery"] = calculateBatteryPercentage();

        String payload;
        serializeJson(doc, payload);
        if (mqttManager.isConnected())
        {
            mqttManager.publishState(payload.c_str(), deviceId.c_str());
        }
        sensor.reset();
        cycleStep = 0;
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
        deviceId = wifiManager.getDeviceUid();

        mqttManager.setDeviceId(deviceId);
        mqttManager.setMessageCallback(handleMqttMessage);
        mqttManager.begin();
    }

    state = State::AUTO;
}

void modeSleep()
{
    // Solo apagamos el radio si venimos de AUTO (WiFi encendido)
    if (cycleStep == 0)
    {
        if (mqttManager.isConnected())
            mqttManager.disconnect();
        WiFi.disconnect();
        delay(50);
        esp_wifi_stop();
        delay(50);
    }

    esp_sleep_enable_ext0_wakeup((gpio_num_t)resetPin, 0);
    esp_sleep_enable_timer_wakeup(SLEEP_HALF_US);
    esp_light_sleep_start();

    resetCredentials();

    if (cycleStep == 0)
    {
        state = State::MEASURE; // medir sin encender WiFi
    }
    else
    {
        esp_wifi_start();
        delay(50);
        state = State::CONNECTION; // conectar y enviar
    }
}

void setup()
{
    Serial.begin(115200);
    analogReadResolution(12);
    pinMode(BATTERY_PIN, INPUT);
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
        provisioningStart = millis();
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
    case State::MEASURE:
        measureOnly();
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
        else if (millis() - provisioningStart > PROVISIONING_TIMEOUT_MS)
        {
            provisioningTimeoutSleep();
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