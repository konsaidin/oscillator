#include "PowerAnalyzer.h"
#include <ArduinoJson.h>
#include <math.h>

PowerAnalyzer::PowerAnalyzer(VoltageSensor* sA, VoltageSensor* sB, VoltageSensor* sC) {
    _sensorA = sA;
    _sensorB = sB;
    _sensorC = sC;
}

void PowerAnalyzer::update() {
    // Read voltages
    _status.voltageA = _sensorA->readRMS();
    _status.voltageB = _sensorB->readRMS();
    _status.voltageC = _sensorC->readRMS();

    // Calculate Line Voltages (Approximate assuming 120 deg shift)
    // U_line = U_phase * sqrt(3)
    // For more accuracy, we would need zero-crossing detection to measure phase angles.
    _status.voltageAB = _status.voltageA * 1.732;
    _status.voltageBC = _status.voltageB * 1.732;
    _status.voltageCA = _status.voltageC * 1.732;

    // Calculate Unbalance (NEMA definition)
    float avgVoltage = (_status.voltageA + _status.voltageB + _status.voltageC) / 3.0;
    
    if (avgVoltage > 0) {
        float maxDiff = 0;
        maxDiff = max(maxDiff, abs(_status.voltageA - avgVoltage));
        maxDiff = max(maxDiff, abs(_status.voltageB - avgVoltage));
        maxDiff = max(maxDiff, abs(_status.voltageC - avgVoltage));
        
        _status.unbalancePercent = (maxDiff / avgVoltage) * 100.0;
    } else {
        _status.unbalancePercent = 0;
    }

    // Check for issues
    _status.phaseLoss = (_status.voltageA < 50 || _status.voltageB < 50 || _status.voltageC < 50);
    _status.phaseUnbalance = (_status.unbalancePercent > 4.0); // Example threshold
    
    // Frequency is hard to measure with just RMS sampling. 
    // Needs interrupt on zero-crossing. 
    // Setting dummy value or implementing ZC later.
    _status.frequency = 50.0; 
}

GridStatus PowerAnalyzer::getStatus() {
    return _status;
}

String PowerAnalyzer::getJsonStatus() {
    StaticJsonDocument<512> doc;
    
    JsonObject phaseA = doc.createNestedObject("phaseA");
    phaseA["voltage"] = _status.voltageA;
    
    JsonObject phaseB = doc.createNestedObject("phaseB");
    phaseB["voltage"] = _status.voltageB;
    
    JsonObject phaseC = doc.createNestedObject("phaseC");
    phaseC["voltage"] = _status.voltageC;
    
    JsonObject line = doc.createNestedObject("line");
    line["AB"] = _status.voltageAB;
    line["BC"] = _status.voltageBC;
    line["CA"] = _status.voltageCA;
    
    doc["unbalance"] = _status.unbalancePercent;
    doc["frequency"] = _status.frequency;
    
    JsonObject alerts = doc.createNestedObject("alerts");
    alerts["phaseLoss"] = _status.phaseLoss;
    alerts["unbalance"] = _status.phaseUnbalance;

    String output;
    serializeJson(doc, output);
    return output;
}
