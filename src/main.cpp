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
    CLAIM,
    CONNECTION,
    AUTO,
    SLEEP,
};
enum class ClaimStep
{
    IDLE,
    WAIT_MQTT,
    PUBLISHING,
    WAIT_ACK,
    DONE
};

ClaimStep claimStep = ClaimStep::IDLE;
String pendingPayload;
String pendingDeviceId;
unsigned long claimSentAt = 0;
const unsigned long CLAIM_ACK_TIMEOUT_MS = 6000;

Sensor sensor(14, 13);
WiFiManager wifiManager;
BleManager bleManager(wifiManager);
MqttManager mqttManager;
Monitoring monitoring(sensor);

State state;
bool psConfigured = false;
uint8_t resetPin = 4;

void resetCredentials()
{
    if (digitalRead(resetPin) == LOW)
    {
        wifiManager.resetSettings();
        delay(1000);
        ESP.restart();
    }
}

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

void handleMqttMessage(String &topic, String &payload)
{
    Serial.printf("[MQTT] Mensaje en [%s]: %s\n", topic.c_str(), payload.c_str());

    if (topic.endsWith("/result"))
    {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error)
        {
            Serial.printf("[MQTT] Error al parsear JSON: %s\n", error.c_str());
            return;
        }
        bool success = doc["success"] | false;
        const char *status = doc["status"] | "UNKNOWN";

        if (success && String(status) == "CLAIMED")
        {
            Serial.println("¡Dispositivo vinculado con éxito (CLAIMED)!");
            wifiManager.saveSesion();
        }
        else if (!success)
        {
            wifiManager.resetSettings();
            ESP.restart();
        }
        else if (String(status) == "EXPIRED")
        {
            Serial.println("El token de vinculación ha expirado (EXPIRED).");
            wifiManager.resetSettings();
            delay(1000);
            ESP.restart();
        }
        else
        {
            Serial.printf("Proceso de claim rechazado o con estado desconocido: %s\n", status);
        }
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
        String deviceId = wifiManager.getDeviceUid(); // antes: "Monitor-" + mac
        String claimToken = wifiManager.getClaimToken();

        mqttManager.setDeviceId(deviceId);
        mqttManager.setMessageCallback(handleMqttMessage);
        mqttManager.begin();
    }

    state = State::AUTO;
}

void claim()
{
    if (!wifiManager.isConnected())
        return;

    if (claimStep == ClaimStep::IDLE)
    {
        String deviceId = wifiManager.getDeviceUid();
        String claimToken = wifiManager.getClaimToken();

        mqttManager.setDeviceId(deviceId);
        mqttManager.setMessageCallback(handleMqttMessage);
        mqttManager.begin();

        JsonDocument doc;
        doc["claim_token"] = claimToken;
        doc["device_uid"] = deviceId;
        doc["device_type"] = "Monitor";
        doc["device_version"] = 1;
        doc["firmware_version"] = 1;
        doc["hardware_version"] = 1;
        serializeJson(doc, pendingPayload);

        pendingDeviceId = deviceId;

        claimStep = ClaimStep::WAIT_MQTT;
    }

    if (claimStep == ClaimStep::WAIT_MQTT || claimStep == ClaimStep::PUBLISHING)
    {
        if (!mqttManager.isConnected())
        {
            return;
        }

        bool sent = mqttManager.publishClaim(pendingPayload.c_str(), pendingDeviceId.c_str());

        if (sent)
        {
            Serial.println("[CLAIM] Payload enviado, esperando confirmacion del servidor...");
            claimSentAt = millis();
            claimStep = ClaimStep::WAIT_ACK;
        }
        else
        {
            Serial.println("[CLAIM] publish() fallo, reintentando en el siguiente tick.");
            claimStep = ClaimStep::WAIT_MQTT;
        }
    }

    if (claimStep == ClaimStep::WAIT_ACK)
    {
        if (millis() - claimSentAt > CLAIM_ACK_TIMEOUT_MS)
        {
            Serial.println("[CLAIM] Sin respuesta del servidor, reintentando publish...");
            claimStep = ClaimStep::WAIT_MQTT; // vuelve a publicar
        }
    }
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
        claim();
        mqttManager.loop(wifiManager.isConnected());
        break;
    default:
        break;
    }
}
