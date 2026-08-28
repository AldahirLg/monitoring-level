#pragma once
#include <Arduino.h>
#include "sensor/sensor.h"
#include "mqtt/mqtt_manager.h"

enum MonitorSate {
    IDLE,
    CONNECTION,
    MEASURING,
    PROCESSING,
    PUBLISHING,
};

class Monitoring {
public:
    Monitoring(Sensor &sensor, MqttManager &mqtt);
    MonitorSate state();
    void loop();
private:
    Sensor _sensor;
    MqttManager _mqtt;
    MonitorSate _state;
    void idle();
    void connection();
    void measuring();
    void processing();
    void publishing();
    unsigned long _lastTime;
    unsigned long _interval;
};