#include <Arduino.h>
#include "sensor/sensor.h"
#include "wifi/wifi_manager.h"
#include "mqtt/mqtt_manager.h"
#include "ble/ble_manager.h"
#include "monitoring/monitoring.h"

enum State {
    PROVISIONING,
    CONNECTION,
    AUTO,
    
};


Sensor sensor(14, 13);

WiFiManager wifiManager;
BleManager bleManager(wifiManager);
MqttManager mqttManager;

Monitoring monitoring(sensor, mqttManager);

void setup()
{
    Serial.begin(115200);
}


void loop()
{
    monitoring.loop();
}
