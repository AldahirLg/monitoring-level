#include "claim.h"
#include <ArduinoJson.h>

Claim *Claim::_instance = nullptr;

Claim::Claim(WiFiManager &wifiManager, MqttManager &mqttManager)
    : _wifiManager(wifiManager),
      _mqttManager(mqttManager),
      _step(ClaimStep::IDLE),
      _claimSentAt(0)
{
    _instance = this;
}

void Claim::onMqttMessage(String &topic, String &payload)
{
    if (_instance)
    {
        _instance->handleMqttMessage(topic, payload);
    }
}

void Claim::buildPayload()
{
    String deviceId = _wifiManager.getDeviceUid();
    String claimToken = _wifiManager.getClaimToken();

    _mqttManager.setDeviceId(deviceId);
    _mqttManager.setMessageCallback(&Claim::onMqttMessage);
    _mqttManager.begin();

    JsonDocument doc;
    doc["claim_token"] = claimToken;
    doc["device_uid"] = deviceId;
    doc["device_type"] = "Medidor";
    doc["device_version"] = 1;
    doc["firmware_version"] = 1;
    doc["hardware_version"] = 1;
    serializeJson(doc, _pendingPayload);

    _pendingDeviceId = deviceId;
}

void Claim::handleMqttMessage(String &topic, String &payload)
{
    Serial.printf("[MQTT] Mensaje en [%s]: %s\n", topic.c_str(), payload.c_str());

    if (!topic.endsWith("/result"))
    {
        return;
    }

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
        _wifiManager.saveSesion();
        _step = ClaimStep::DONE;
    }
    else if (!success)
    {
        _wifiManager.resetSettings();
        ESP.restart();
    }
    else if (String(status) == "EXPIRED")
    {
        Serial.println("El token de vinculación ha expirado (EXPIRED).");
        _wifiManager.resetSettings();
        delay(1000);
        ESP.restart();
    }
    else
    {
        Serial.printf("Proceso de claim rechazado o con estado desconocido: %s\n", status);
    }
}

void Claim::loop()
{
    if (!_wifiManager.isConnected())
        return;

    if (_step == ClaimStep::IDLE)
    {
        buildPayload();
        _step = ClaimStep::WAIT_MQTT;
    }

    if (_step == ClaimStep::WAIT_MQTT || _step == ClaimStep::PUBLISHING)
    {
        if (!_mqttManager.isConnected())
        {
            return;
        }

        bool sent = _mqttManager.publishClaim(_pendingPayload.c_str(), _pendingDeviceId.c_str());

        if (sent)
        {
            Serial.println("[CLAIM] Payload enviado, esperando confirmacion del servidor...");
            _claimSentAt = millis();
            _step = ClaimStep::WAIT_ACK;
        }
        else
        {
            Serial.println("[CLAIM] publish() fallo, reintentando en el siguiente tick.");
            _step = ClaimStep::WAIT_MQTT;
        }
    }

    if (_step == ClaimStep::WAIT_ACK)
    {
        if (millis() - _claimSentAt > CLAIM_ACK_TIMEOUT_MS)
        {
            Serial.println("[CLAIM] Sin respuesta del servidor, reintentando publish...");
            _step = ClaimStep::WAIT_MQTT;
        }
    }
}

ClaimStep Claim::getStep() const
{
    return _step;
}

bool Claim::isDone() const
{
    return _step == ClaimStep::DONE;
}

void Claim::reset()
{
    _step = ClaimStep::IDLE;
    _pendingPayload = "";
    _pendingDeviceId = "";
    _claimSentAt = 0;
}