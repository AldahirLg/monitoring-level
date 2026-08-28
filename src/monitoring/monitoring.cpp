#include "monitoring.h"

Monitoring::Monitoring(Sensor &sensor)
    : _sensor(sensor), _state(MonitorSate::IDLE), _lastTime(0), _interval(4000)
{
}

MonitorSate Monitoring::state()
{
    return _state;
}

void Monitoring::idle()
{
    Serial.println("Inicio");
    _state = MonitorSate::MEASURING;
}

void Monitoring::measuring()
{
    unsigned long currentTime = millis();

    if (!_sensor.isMeasuring())
    {
        _lastTime = currentTime;
        _sensor.start();
        return;
    }

    if (_sensor.measure())
    {
        _state = MonitorSate::PROCESSING;
    }
}

void Monitoring::processing()
{
    // Porcentaje
    _percent = convertToPercent();
    Serial.print("Porcentaje: ");
    Serial.print(_percent);
    Serial.println("%");
    // Estado de sensor
    _sensorState = _sensor.sensorState();
    void reset();
    // Evento de usuario
    _eventState = handlerEvent(_percent);
    _lastTime = millis();
    _state = MonitorSate::END;
}

void Monitoring::reset()
{
    _state = MonitorSate::IDLE;
}

EventState Monitoring::handlerEvent(int percent)
{
    if (percent > _maxLevel && _eventState != EventState::MAX_LEVEL)
    {
        return EventState::MAX_LEVEL;
    }
    else if (percent < _minLevel && _eventState != EventState::MIN_LEVEL)
    {
        return EventState::MIN_LEVEL;
    }
    return EventState::NONE;
}

int Monitoring::convertToPercent()
{
    if (_height < 30)
        return 0;

    int value = _sensor.getDistance();

    value = constrain(value, 30, _height);

    int percent = ((_height - value) * 100) / (_height - 30);

    return constrain(percent, 0, 100);
}

void Monitoring::setValues(int height, int minLevel, int maxLevel, bool notification)
{
    _height = height;
    _minLevel = minLevel;
    _maxLevel = maxLevel;
    _notification = notification;
}

int Monitoring::getPercent()
{
    return _percent;
}

bool Monitoring::getSensorState()
{
    return _sensorState;
}

void Monitoring::loop()
{
    switch (_state)
    {
    case MonitorSate::IDLE:
        idle();
        break;
    case MonitorSate::MEASURING:
        measuring();
        break;
    case MonitorSate::PROCESSING:
        processing();
        break;
    case MonitorSate::END:
        break;
    default:
        break;
    }
}
