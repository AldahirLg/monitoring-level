#include "wifi_manager.h"

WiFiManager::WiFiManager(uint16_t port) : _server(port) {}

void WiFiManager::begin()
{
    delay(1000);
    String ssid = getSavedSSID();
    String pass = getSavedPassword();
    delay(100);
    // CASO 1: No hay credenciales guardadas -> No conectar
    if (!hasSavedCredentials())
    {
        Serial.println("[WiFi] Sin credenciales guardadas. Esperando provisioning por BLE.");
        _modoConfig = true;
        return;
    }

    // CASO 2: Hay credenciales -> Intentar conectar
    WiFi.mode(WIFI_STA);
    // WiFi.setSleep(false);
    // WiFi.setTxPower(WIFI_POWER_19_5dBm);
    Serial.printf("[WiFi] Intentando conectar a: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    const unsigned long TIMEOUT_MS = 10000;
    unsigned long start = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - start > TIMEOUT_MS)
        {
            Serial.println("\n[WiFi] Timeout.");
            break;
        }
        delay(100);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("[WiFi] Conectado en %lu ms | IP: %s\n",
                      millis() - start,
                      WiFi.localIP().toString().c_str());
    }
    else
    {
        Serial.println("[WiFi] No se puedo conectar");
    }
}

void WiFiManager::loop()
{
    if (_testState == ConnectionState::TESTING_WIFI)
    {

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println("¡Prueba de conexión exitosa!");
            saveCredentials(_tempSSID, _tempPass);
            _modoConfig = false;

            _testState = ConnectionState::TEST_SUCCESS;
            _shouldRestart = true;
            _restartTimer = millis();
        }
        else if (millis() - _testStartTime > _wifiTestTimeOutMs)
        {
            if (_testRetryCount < _wifiTestMaxRetries)
            {
                _testRetryCount++;
                Serial.printf("Timeout al conectar. Reintentando (%u/%u)...\n", _testRetryCount, _wifiTestMaxRetries);
                WiFi.disconnect();
                delay(10);
                WiFi.begin(_tempSSID.c_str(), _tempPass.c_str());
                _testStartTime = millis();
            }
            else
            {
                _lastFailureReason = "timeout_or_auth";
                Serial.println("Prueba de conexión fallida (timeout o contraseña incorrecta).");
                WiFi.disconnect();
                _testState = ConnectionState::TEST_FAILED;
            }
        }
    }
    /*if (_shouldRestart && millis() - _restartTimer > _restartDelayMs)
    {
        Serial.println("Reiniciando el dispositivo...");
        saveSesion();
        delay(1000);
        ESP.restart();
    }*/
}

WiFiManagerStatus WiFiManager::getStatus()
{
    if (_modoConfig)
    {
        return WiFiManagerStatus::PROVISIONING;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        return WiFiManagerStatus::CONNECTED;
    }
    return _modoConfig ? WiFiManagerStatus::PROVISIONING : WiFiManagerStatus::DISCONNECTED;
}

bool WiFiManager::isConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}

void WiFiManager::resetSettings()
{
    _preferences.begin("wifi", false);
    _preferences.clear();
    _preferences.end();
    Serial.println("Credenciales WiFi borradas.");
    _preferences.begin("sesion", false);
    _preferences.clear();
    _preferences.end();
    Serial.println("Sesion Borrada.");
}

void WiFiManager::resetSettingsAndRestart(uint32_t delayMs)
{
    resetSettings();
    _shouldRestart = true;
    _restartDelayMs = delayMs;
    _restartTimer = millis();
}

bool WiFiManager::startProvisioningTest(const String &ssid, const String &pass, String claimToken)
{
    String ssidSanitized = ssid;
    ssidSanitized.trim();

    if (ssidSanitized.isEmpty())
    {
        _lastFailureReason = "ssid_empty";
        Serial.println("No se puede iniciar provisioning: SSID vacío.");
        return false;
    }

    _tempSSID = ssidSanitized;
    _tempPass = pass;
    _testRetryCount = 0;
    _lastFailureReason = "";
    _claimToken = claimToken;
    Serial.printf("Probando nuevas credenciales: SSID=%s\n", _tempSSID.c_str());
    _modoConfig = true;
    WiFi.mode(WIFI_STA);
    WiFi.softAPdisconnect(true);
    WiFi.disconnect();
    delay(10);
    WiFi.begin(_tempSSID.c_str(), _tempPass.c_str());

    _testState = ConnectionState::TESTING_WIFI;
    _testStartTime = millis();
    return true;
}

ConnectionState WiFiManager::getConnectionState() const
{
    return _testState;
}

String WiFiManager::getLastFailureReason() const
{
    return _lastFailureReason;
}

String WiFiManager::getConnectedMac() const
{
    String mac = WiFi.macAddress();
    return mac;
}
String WiFiManager::getDeviceUid() const
{
    return "Monitor-" + getConnectedMac();
}

String WiFiManager::getLocalIP() const
{
    return WiFi.localIP().toString();
}

String WiFiManager::getSavedSSID() const
{
    Preferences preferences;
    preferences.begin("wifi", true);
    if (!preferences.isKey("ssid"))
    {
        preferences.end();
        return "";
    }

    String ssid = preferences.getString("ssid", "");
    preferences.end();
    return ssid;
}

bool WiFiManager::hasSavedCredentials() const
{
    Preferences preferences;
    preferences.begin("wifi", true);
    bool exists = preferences.isKey("ssid") && preferences.isKey("pass");

    preferences.end();
    return exists;
}

String WiFiManager::getSavedPassword()
{
    Preferences preferences;
    preferences.begin("wifi", true);
    if (!preferences.isKey("pass"))
    {
        preferences.end();
        return "";
    }
    String pass = preferences.getString("pass", "");
    preferences.end();
    return pass;
}

void WiFiManager::saveCredentials(String ssid, String pass)
{
    _preferences.begin("wifi", false);
    _preferences.putString("ssid", ssid);
    _preferences.putString("pass", pass);
    _preferences.end();
    Serial.println("Credenciales guardadas en NVS.");
}

void WiFiManager::saveSesion()
{
    _preferences.begin("sesion", false);
    _preferences.putBool("claimed", true);
    _preferences.end();
    Serial.println("Sesion guardadas en NVS.");
    delay(1000);
    ESP.restart();
}

void WiFiManager::setClaimToken(const String claimToken)
{
    _claimToken = claimToken;
}

String WiFiManager::getClaimToken()
{
    return _claimToken;
}

String WiFiManager::getInfoWiFi()
{
    String info = String("SSID: ") + getSavedSSID() + ", IP: " + getLocalIP() + ", MAC: " + getConnectedMac();
    return info;
}