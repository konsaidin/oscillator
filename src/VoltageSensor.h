#pragma once
#include <Arduino.h>

class VoltageSensor {
private:
    int _pin;
    float _sensitivity;
    float _offset;
    float _voltageRMS;
    
public:
    VoltageSensor(int pin, float sensitivity);
    void begin();
    void setSensitivity(float sensitivity);
    float readRMS(int samples = 1000);
    float getLastRMS();
};
