#pragma once
#include <Arduino.h>
#include <NewPing.h>

class Sensor
{
public:
    Sensor(uint8_t triggerPin, uint8_t echoPin);
    bool measure();
    void start();
    bool measuring = false;
    bool sensorState();
    void setValues(int height, int minValue, int maxValue, bool notification);
    int getPercent();

private:
    NewPing _sonar;
    int _maxDistance = 400;
    unsigned long _pingSpeed = 50;
    unsigned long _pingTimer;
    unsigned long _sum = 0;
    int _attempts = 0;
    float _distance = 0;
    bool _sensorState = false;
    float getDistance();

    int _height = 0;
};