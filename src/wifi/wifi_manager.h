#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
enum class WiFiManagerStatus
{
    CONNECTED,
    DISCONNECTED,
    PROVISIONING,
};

enum class ConnectionState
{
    IDLE,
    TESTING_WIFI,
    TEST_SUCCESS,
    TEST_FAILED
};

class WiFiManager
{
public:
    WiFiManager(uint16_t port = 80);
    void begin();
    void loop();

    WiFiManagerStatus getStatus();
    bool isConnected();
    void resetSettings();
    void resetSettingsAndRestart(uint32_t delayMs = 500);

    bool startProvisioningTest(const String &ssid, const String &pass, String claimToken);
    ConnectionState getConnectionState() const;
    String getLastFailureReason() const;
    String getSavedSSID() const;
    bool hasSavedCredentials() const;
    String getConnectedMac() const;
    String getLocalIP() const;
    String getInfoWiFi();
    void saveSesion();
    void setClaimToken(const String claimToken);
    String getClaimToken();
    String getDeviceUid() const;
    AsyncWebServer &getServer() { return _server; }

private:
    Preferences _preferences;
    AsyncWebServer _server;

    ConnectionState _testState = ConnectionState::IDLE;
    unsigned long _testStartTime = 0;
    String _tempSSID;
    String _tempPass;
    uint8_t _testRetryCount = 0;
    String _lastFailureReason;

    bool _shouldRestart = false;
    unsigned long _restartTimer = 0;
    uint32_t _restartDelayMs = 0;

    bool _modoConfig = false;

    void saveCredentials(String ssid, String pass);
    String getSavedPassword();

    unsigned long _wifiTestTimeOutMs = 10000;
    uint8_t _wifiTestMaxRetries = 1;

    String _claimToken;
};
