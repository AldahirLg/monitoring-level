#include "sensor.h"

Sensor::Sensor(uint8_t triggerPin, uint8_t echoPin)
    : _sonar(triggerPin, echoPin, _maxDistance), _pingTimer(0)
{
}

void Sensor::start()
{
    _attempts = 0;
    _sum = 0;
    _pingTimer = millis();
    measuring = true;
}

bool Sensor::measure()
{
    unsigned long currentTime = millis();
    if (currentTime - _pingTimer >= _pingSpeed)
    {
        _pingTimer = currentTime;
        unsigned int distance = _sonar.ping_cm();
        _sum += distance;
        _attempts++;
        Serial.print("Distancia: ");
        Serial.print(distance);
        Serial.println(" cm");
        if (_attempts >= 10)
        {
            measuring = false;
            if (_sum != 0)
            {
                _distance = _sum / 10.0;
                _sensorState = true;
            }
            else
            {
                _sensorState = false;
            }
            return true;
        }
    }
    return false;
}

float Sensor::getDistance()
{
    return _distance;
}

bool Sensor::sensorState()
{
    return _sensorState;
}

int Sensor::getPercent()
{
    if (_height < 30)
        return 0;

    int value = getDistance();

    value = constrain(value, 30, _height);

    int percent = ((_height - value) * 100) / (_height - 30);

    return constrain(percent, 0, 100);
}