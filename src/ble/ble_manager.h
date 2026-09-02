#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <ArduinoJson.h>
#include <BLE2902.h>

#include "wifi/wifi_manager.h"

namespace
{
    constexpr char BLE_SERVICE_UUID[] = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr char BLE_RX_UUID[] = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr char BLE_TX_UUID[] = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";
    constexpr unsigned long STATUS_NOTIFY_INTERVAL_MS = 1200;
}

class BleManager : public BLEServerCallbacks,
                   public BLECharacteristicCallbacks
{
public:
    BleManager(WiFiManager &wifiManager);
    void begin();
    void loop();

    void handleRxChunk(const std::string &value);
    void handleCommandLine(const String &line);

    void onConnect(BLEServer *pServer) override;
    void onDisconnect(BLEServer *pServer) override;

    void onWrite(BLECharacteristic *characteristic) override;

    void enqueueRx(const std::string &value);

private:
    bool _initialized = false;
    WiFiManager &_wifiManager;

    BLEServer *_server = nullptr;
    BLECharacteristic *_rxCharacteristic = nullptr;
    BLECharacteristic *_txCharacteristic = nullptr;

    bool _clientConnected = false;
    bool _awaitingProvisionResult = false;
    bool _sentConnecting = false;
    unsigned long _lastProgressNotifyMs = 0;

    String _rxBuffer;

    std::string _pendingData;
    bool _hasNewData = false;

    void notifyJsonLine(const JsonDocument &doc);
    void notifyStatus(const char *status, const char *message = nullptr);

    void handleGetDeviceId();
    void handleProvision(JsonDocument &doc);
};