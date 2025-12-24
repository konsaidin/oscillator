#include "VoltageSensor.h"

VoltageSensor::VoltageSensor(int pin, float sensitivity) {
    _pin = pin;
    _sensitivity = sensitivity;
    _offset = 0; // Will be calibrated or assumed VCC/2
    _voltageRMS = 0;
}

void VoltageSensor::begin() {
    pinMode(_pin, INPUT);
    // Simple DC offset calibration on startup
    long sum = 0;
    for(int i=0; i<1000; i++) {
        sum += analogRead(_pin);
        delayMicroseconds(100);
    }
    _offset = sum / 1000.0;
}

void VoltageSensor::setSensitivity(float sensitivity) {
    _sensitivity = sensitivity;
}

float VoltageSensor::readRMS(int samples) {
    long sumSq = 0;
    unsigned long startMillis = millis();
    
    // Sampling for a fixed duration (e.g., 200ms = 10 cycles at 50Hz)
    // Or fixed number of samples. Let's do fixed samples for now, 
    // but in real world, time-based is better for AC.
    
    int count = 0;
    while (count < samples) {
        int raw = analogRead(_pin);
        float val = raw - _offset;
        sumSq += val * val;
        count++;
        delayMicroseconds(200); // Adjust sampling rate
    }
    
    float meanSq = (float)sumSq / count;
    float rmsADC = sqrt(meanSq);
    
    // Convert ADC RMS to Voltage
    // This formula depends heavily on the ZMPT module circuit.
    // Usually: V_out_rms = V_in_rms * Ratio
    // We use a simplified linear coefficient here.
    _voltageRMS = rmsADC * _sensitivity; 
    
    // Noise filter
    if (_voltageRMS < 5.0) _voltageRMS = 0.0;
    
    return _voltageRMS;
}

float VoltageSensor::getLastRMS() {
    return _voltageRMS;
}
