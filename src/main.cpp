#include <Arduino.h>
#include "sensor/sensor.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"
#include "ble/ble_manager.h"

Sensor sensor(14, 13);
WiFiManager wifiManager();
BleManager bleManager();
MqttManager mqttManager();

void setup()
{
}

void loop()
{
}
