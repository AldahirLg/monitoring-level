#include "monitoring.h"

Monitoring::Monitoring(Sensor &sensor, MqttManager &mqtt)
: _sensor(sensor), _mqtt(mqtt), _state(MonitorSate::IDLE), _lastTime(0), _interval(4000)
{

}

MonitorSate Monitoring::state(){
    return _state;
}


void Monitoring::idle(){
    Serial.println("Inicio");
    _state = MonitorSate::CONNECTION;
}

void Monitoring::connection(){
    Serial.println("Conexion con mqtt");
    _state = MonitorSate::MEASURING;
}

void Monitoring::measuring(){
    unsigned long currentTime = millis();

    if (!_sensor.measuring) {

        if (currentTime - _lastTime >= _interval) {

            _lastTime = currentTime;
            _sensor.start();
        }

        return;
    }

    if (_sensor.measure()) {
        _state = MonitorSate::PROCESSING;
    }
}

void Monitoring::processing(){
    int percent = _sensor.getPercent();
    Serial.print("Porcentaje: ");
    Serial.print(percent);
    Serial.println("%");
    bool sensorState = _sensor.sensorState();
    _lastTime = millis();
    _state = MonitorSate::MEASURING;
}

void Monitoring::publishing(){
    Serial.println("Publicar mqtt");
    _state = MonitorSate::CONNECTION;
}


void Monitoring::loop() {
    switch (_state)
    {
    case MonitorSate::IDLE:
        idle();
        break;
    case MonitorSate::CONNECTION:
        connection();
        break;    
    case MonitorSate::MEASURING:
        measuring();
        break;
    case MonitorSate::PROCESSING:
        processing();
        break;
    case MonitorSate::PUBLISHING:
        publishing();
        break;
    default:
        break;
    }
}


