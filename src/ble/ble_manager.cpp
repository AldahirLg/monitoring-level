#include "ble_manager.h"

BleManager::BleManager(WiFiManager &wifiManager)
    : _wifiManager(wifiManager)
{
}

void BleManager::begin()
{
    Serial.println("[BLE] Inicializando provisionamiento por BLE...");
    BLEDevice::init("Monitor de Nivel");
    BLEDevice::setMTU(185);
    _server = BLEDevice::createServer();
    _server->setCallbacks(this);
    BLEService *service = _server->createService(BLE_SERVICE_UUID);
    _rxCharacteristic = service->createCharacteristic(
        BLE_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR);

    _rxCharacteristic->setCallbacks(this);
    _txCharacteristic = service->createCharacteristic(
        BLE_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY);
    _txCharacteristic->addDescriptor(new BLE2902());
    service->start();
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(BLE_SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06);
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.printf("[BLE] Servicio listo. UUID=%s\n", BLE_SERVICE_UUID);
}

void BleManager::loop()
{

    if (_hasNewData)
    {
        _hasNewData = false;
        handleRxChunk(_pendingData);
        _pendingData.clear();
    }

    if (!_awaitingProvisionResult)
    {
        return;
    }

    const ConnectionState state = _wifiManager.getConnectionState();

    if (state == ConnectionState::TESTING_WIFI)
    {
        if (!_sentConnecting)
        {
            notifyStatus("connecting");
            _sentConnecting = true;
            _lastProgressNotifyMs = millis();
        }
        else if (millis() - _lastProgressNotifyMs > STATUS_NOTIFY_INTERVAL_MS)
        {
            notifyStatus("testing");
            _lastProgressNotifyMs = millis();
        }
        return;
    }

    if (state == ConnectionState::TEST_SUCCESS)
    {
        JsonDocument doc;
        doc["status"] = "success";
        doc["mac"] = _wifiManager.getConnectedMac();
        notifyJsonLine(doc);
        _awaitingProvisionResult = false;
        _sentConnecting = false;
        Serial.println("[BLE] Provisioning completado con éxito.");
        return;
    }

    if (state == ConnectionState::TEST_FAILED)
    {
        const String reason = _wifiManager.getLastFailureReason();
        notifyStatus("failed", reason.isEmpty() ? "wifi_connection_failed" : reason.c_str());
        _awaitingProvisionResult = false;
        _sentConnecting = false;
        Serial.printf("[BLE] Provisioning fallido: %s\n", reason.c_str());
    }
}

void BleManager::handleRxChunk(const std::string &value)
{
    if (value.empty())
    {
        return;
    }

    _rxBuffer.reserve(_rxBuffer.length() + value.size());
    for (char c : value)
    {
        _rxBuffer += c;
    }

    int newlineIndex = _rxBuffer.indexOf('\n');
    while (newlineIndex >= 0)
    {
        String line = _rxBuffer.substring(0, newlineIndex);
        _rxBuffer.remove(0, newlineIndex + 1);
        line.trim();
        if (!line.isEmpty())
        {
            handleCommandLine(line);
        }
        newlineIndex = _rxBuffer.indexOf('\n');
    }

    if (_rxBuffer.length() > 512)
    {
        _rxBuffer = "";
        notifyStatus("error", "rx_buffer_overflow");
        Serial.println("[BLE] Error: buffer RX excedido, limpiando.");
    }
}

void BleManager::handleCommandLine(const String &line)
{
    Serial.printf("[BLE] RX line: %s\n", line.c_str());

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, line);
    if (err)
    {
        notifyStatus("error", "invalid_json");
        Serial.printf("[BLE] JSON invalido: %s\n", err.c_str());
        return;
    }

    const char *type = doc["type"] | "";
    if (String(type) != "provision")
    {
        notifyStatus("error", "unsupported_type");
        Serial.printf("[BLE] Tipo de comando no soportado: %s\n", type);
        return;
    }

    const char *ssid = doc["ssid"] | "";
    const char *password = doc["password"] | "";
    const char *tokenClain = doc["token_claim"] | "";

    if (String(ssid).length() == 0)
    {
        notifyStatus("error", "ssid_empty");
        Serial.println("[BLE] ssid vacío en comando provision.");
        return;
    }

    if (String(tokenClain).length() == 0)
    {
        notifyStatus("error", "token_claim_empty");
        Serial.println("[BLE] token_claim vacío en comando provision.");
        return;
    }

    if (_wifiManager.getConnectionState() == ConnectionState::TESTING_WIFI)
    {
        notifyStatus("error", "provisioning_in_progress");
        Serial.println("[BLE] Ignorado: provisioning ya está en curso.");
        return;
    }

    if (!_wifiManager.startProvisioningTest(String(ssid), String(password)))
    {
        notifyStatus("failed", "cannot_start_wifi_test");
        Serial.println("[BLE] No se pudo iniciar la prueba WiFi.");
        return;
    }

    _awaitingProvisionResult = true;
    _sentConnecting = false;

    notifyStatus("working");
    Serial.printf("[BLE] Provisioning iniciado para SSID=%s\n", ssid);
}

void BleManager::onConnect(BLEServer *pServer)
{
    (void)pServer;

    _clientConnected = true;

    Serial.println("[BLE] Cliente conectado.");
}

void BleManager::onWrite(BLECharacteristic *characteristic)
{
    enqueueRx(characteristic->getValue());
}

void BleManager::onDisconnect(BLEServer *pServer)
{
    (void)pServer;

    _clientConnected = false;
    _rxBuffer = "";

    Serial.println("[BLE] Cliente desconectado. Reanudando advertising...");

    BLEDevice::startAdvertising();
}

void BleManager::enqueueRx(const std::string &value)
{
    _pendingData += value;
    _hasNewData = true;
}

void BleManager::notifyJsonLine(const JsonDocument &doc)
{
    if (!_txCharacteristic || !_clientConnected)
    {
        return;
    }

    String payload;
    serializeJson(doc, payload);
    payload += "\n";

    _txCharacteristic->setValue(payload.c_str());
    _txCharacteristic->notify();
    Serial.printf("[BLE] TX notify: %s", payload.c_str());
}

void BleManager::notifyStatus(const char *status, const char *message)
{
    JsonDocument doc;
    doc["status"] = status;
    if (message != nullptr)
    {
        doc["message"] = message;
    }
    notifyJsonLine(doc);
}