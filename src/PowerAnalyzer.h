#pragma once
#include <Arduino.h>
#include "VoltageSensor.h"

struct GridStatus {
    float voltageA;
    float voltageB;
    float voltageC;
    float voltageAB;
    float voltageBC;
    float voltageCA;
    float unbalancePercent;
    bool phaseLoss;
    bool phaseUnbalance;
    float frequency; // Placeholder for now
};

class PowerAnalyzer {
private:
    VoltageSensor* _sensorA;
    VoltageSensor* _sensorB;
    VoltageSensor* _sensorC;
    GridStatus _status;

public:
    PowerAnalyzer(VoltageSensor* sA, VoltageSensor* sB, VoltageSensor* sC);
    void update();
    GridStatus getStatus();
    String getJsonStatus();
};
