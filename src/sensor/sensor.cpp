
#include "sensor.h"
#include <math.h>

Sensor::Sensor(uint8_t triggerPin, uint8_t echoPin)
    : _sonar(triggerPin, echoPin, 400), _pingTimer(0)
{
}

void Sensor::start()
{
    _attempts = 0;
    _validAttempts = 0;
    _sum = 0;
    _pingTimer = millis();
}

bool Sensor::measure()
{
    unsigned long currentTime = millis();

    if (currentTime - _pingTimer < _pingSpeed)
        return false;

    _pingTimer = currentTime;

    unsigned int distance = _sonar.ping_cm();
    _attempts++;

    if (distance > 0)
    {
        _sum += distance;
        _validAttempts++;
    }

    Serial.print("Lectura: ");
    Serial.print(distance);
    Serial.print(" cm | Validas: ");
    Serial.print(_validAttempts);
    Serial.print("/");
    Serial.println(_attempts);

    if (_attempts >= SAMPLE_COUNT)
    {
        processSamples();
        return true;
    }

    return false;
}

void Sensor::processSamples()
{
    if (_validAttempts == 0)
    {
        _sensorState = false;
        Serial.println("Ventana invalida: sin lecturas validas");
        return;
    }

    float raw = _sum / (float)_validAttempts;
    float corrected = fmaxf(0.0f, raw - _offset);

    _sensorState = true;

    // Inicializar el filtro con la primera medición válida.
    if (!_filterInitialized)
    {
        _filteredDistance = corrected;
        _filterInitialized = true;
    }

    // Actualizar la mediana móvil.
    updateHistory(roundf(corrected));
    float median = getMedian();

    // Acercar gradualmente la distancia a la mediana.
    _filteredDistance +=
        _filterAlpha * (median - _filteredDistance);

    _distance = roundf(_filteredDistance);

    Serial.print("Promedio ventana: ");
    Serial.print(corrected);
    Serial.print(" cm | Mediana móvil: ");
    Serial.print(median);
    Serial.print(" cm | Distancia filtrada: ");
    Serial.print(_distance);
    Serial.println(" cm");
}

void Sensor::updateHistory(float distance)
{
    _history[_historyIndex] = distance;

    _historyIndex = (_historyIndex + 1) % HISTORY_SIZE;

    if (_historyCount < HISTORY_SIZE)
        _historyCount++;
}

float Sensor::getMedian()
{
    float sorted[HISTORY_SIZE];

    for (uint8_t i = 0; i < _historyCount; i++)
        sorted[i] = _history[i];

    // Ordenar las ventanas disponibles.
    for (uint8_t i = 0; i < _historyCount - 1; i++)
    {
        for (uint8_t j = i + 1; j < _historyCount; j++)
        {
            if (sorted[j] < sorted[i])
            {
                float temp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temp;
            }
        }
    }

    uint8_t middle = _historyCount / 2;

    if (_historyCount % 2 == 0)
        return (sorted[middle - 1] + sorted[middle]) / 2.0f;

    return sorted[middle];
}

void Sensor::idle()
{
    start();
    _state = SensorMode::MEASURING;
}

void Sensor::measuring()
{
    if (measure())
        _state = SensorMode::DONE;
}

void Sensor::loop()
{
    switch (_state)
    {
    case SensorMode::IDLE:
        idle();
        break;

    case SensorMode::MEASURING:
        measuring();
        break;

    case SensorMode::DONE:
        break;
    }
}

void Sensor::reset()
{
    _state = SensorMode::IDLE;
}

float Sensor::getDistance()
{
    return _distance;
}

bool Sensor::sensorState()
{
    return _sensorState;
}

SensorMode Sensor::state()
{
    return _state;
}
