
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
    void processSamples();
    void updateHistory(float distance);
    float getMedian();

    static constexpr uint8_t SAMPLE_COUNT = 10;
    static constexpr uint8_t HISTORY_SIZE = 5;

    NewPing _sonar;
    SensorMode _state = SensorMode::IDLE;

    unsigned long _pingSpeed = 100;
    unsigned long _pingTimer = 0;
    unsigned long _sum = 0;

    uint16_t _samples[SAMPLE_COUNT] = {};
    uint8_t _attempts = 0;
    uint8_t _validAttempts = 0;

    float _history[HISTORY_SIZE] = {};
    uint8_t _historyCount = 0;
    uint8_t _historyIndex = 0;

    float _distance = 0;
    bool _sensorState = false;

    int _offset = 30;

    float _filteredDistance = 0;
    float _filterAlpha = 0.5f;
    bool _filterInitialized = false;
};
