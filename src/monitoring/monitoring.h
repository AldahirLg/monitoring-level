#pragma once
#include <Arduino.h>
#include "sensor/sensor.h"
#include "mqtt/mqtt_manager.h"

enum MonitorSate
{
    IDLE,
    MEASURING,
    PROCESSING,
    END,
};

enum EventState
{
    MIN_LEVEL,
    NONE,
    MAX_LEVEL
};

class Monitoring
{
public:
    Monitoring(Sensor &sensor);
    MonitorSate state();
    void setValues(int height, int minLevel, int maxLevel, bool notification);
    int getPercent();
    bool getSensorState();
    void loop();
    void reset();

private:
    Sensor _sensor;
    MqttManager _mqtt;
    MonitorSate _state;
    EventState _eventState;
    void idle();
    void measuring();
    void processing();
    bool _sensorState = false;
    unsigned long _lastTime;
    unsigned long _interval;
    int _percent = 0;
    int convertToPercent();
    EventState handlerEvent(int percent);
    // Parametros de usuario
    int _height = 0;
    int _minLevel = 0;
    int _maxLevel = 0;
    bool _notification = false;
};