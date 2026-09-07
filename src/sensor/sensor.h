#pragma once
#include <Arduino.h>
#include <NewPing.h>

enum class SensorMode
{
    IDLE,
    MEASURING,
    DONE
};

class Sensor
{
public:
    Sensor(uint8_t triggerPin, uint8_t echoPin);
    void loop();
    SensorMode state();
    void reset();
    float getDistance();
    bool sensorState();

private:
    void idle();
    void measuring();
    void start();
    bool measure();

    NewPing _sonar;
    SensorMode _state = SensorMode::IDLE;
    int _maxDistance = 400;
    unsigned long _pingSpeed = 100;
    unsigned long _pingTimer = 0;
    unsigned long _sum = 0;
    int _attempts = 0;
    float _distance = 0;
    bool _sensorState = false;
};