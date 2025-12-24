#include "VoltageSensor.h"

VoltageSensor::VoltageSensor(int pin, float sensitivity) {
    _pin = pin;
    _sensitivity = sensitivity;
    _offset = ADC_OFFSET;  // Default offset (VCC/2)
    _voltageRMS = 0;
    _frequency = GRID_FREQUENCY;
    _lastCrossingTime = 0;
    _crossingCount = 0;
}

void VoltageSensor::begin() {
    pinMode(_pin, INPUT);
    analogReadResolution(ADC_RESOLUTION);
    analogSetAttenuation(ADC_11db);  // Full range 0-3.3V
    
    // Allow ADC to stabilize
    delay(100);
    
    // Calibrate offset
    calibrateOffset();
}

void VoltageSensor::calibrateOffset() {
    // Sample ADC multiple times to find the DC offset
    // This should ideally be done with no AC signal, but works reasonably
    // well with AC too as we're averaging over many cycles
    
    long sum = 0;
    const int calibrationSamples = 5000;
    
    for (int i = 0; i < calibrationSamples; i++) {
        sum += analogRead(_pin);
        delayMicroseconds(50);
    }
    
    _offset = (float)sum / calibrationSamples;
    
    Serial.printf("[VoltageSensor] Pin %d offset calibrated: %.1f\n", _pin, _offset);
}

void VoltageSensor::setSensitivity(float sensitivity) {
    _sensitivity = sensitivity;
}

float VoltageSensor::getSensitivity() const {
    return _sensitivity;
}

float VoltageSensor::readRMS(int samples) {
    long sumSquares = 0;
    int lastValue = analogRead(_pin);
    bool wasAboveZero = lastValue > _offset;
    
    unsigned long startTime = micros();
    int zeroCrossings = 0;
    unsigned long firstCrossingTime = 0;
    unsigned long lastCrossingTime = 0;
    
    for (int i = 0; i < samples; i++) {
        int raw = analogRead(_pin);
        
        // Calculate deviation from offset
        float value = raw - _offset;
        sumSquares += (long)(value * value);
        
        // Zero-crossing detection for frequency measurement
        bool isAboveZero = raw > _offset;
        if (isAboveZero != wasAboveZero) {
            // Zero crossing detected
            unsigned long now = micros();
            if (zeroCrossings == 0) {
                firstCrossingTime = now;
            }
            lastCrossingTime = now;
            zeroCrossings++;
            wasAboveZero = isAboveZero;
        }
        
        // Maintain consistent sampling rate
        while (micros() - startTime < (i + 1) * SAMPLE_INTERVAL_US) {
            // Busy wait for precise timing
        }
    }
    
    // Calculate RMS from ADC values
    float meanSquare = (float)sumSquares / samples;
    float rmsADC = sqrt(meanSquare);
    
    // Convert ADC RMS to voltage using calibration coefficient
    _voltageRMS = rmsADC * _sensitivity;
    
    // Filter out noise (anything below 5V is probably noise)
    if (_voltageRMS < 5.0) {
        _voltageRMS = 0.0;
    }
    
    // Calculate frequency from zero crossings
    // Each full cycle has 2 zero crossings
    if (zeroCrossings >= 4 && lastCrossingTime > firstCrossingTime) {
        float timePeriod = (lastCrossingTime - firstCrossingTime) / 1000000.0; // Convert to seconds
        int fullCycles = (zeroCrossings - 1) / 2;
        if (fullCycles > 0) {
            _frequency = fullCycles / timePeriod;
            
            // Sanity check - frequency should be around 50Hz (40-60Hz range)
            if (_frequency < 40 || _frequency > 60) {
                _frequency = GRID_FREQUENCY;
            }
        }
    }
    
    return _voltageRMS;
}

float VoltageSensor::getLastRMS() const {
    return _voltageRMS;
}

float VoltageSensor::getFrequency() const {
    return _frequency;
}

int VoltageSensor::readRaw() {
    return analogRead(_pin);
}

float VoltageSensor::getOffset() const {
    return _offset;
}
