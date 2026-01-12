#include "VoltageSensor.h"
#include <cmath>

VoltageSensor::VoltageSensor(int pin, float sensitivity) {
    _pin = pin;
    _sensitivity = sensitivity;
    _offset = ADC_OFFSET;  // Default offset (VCC/2)
    _noiseRmsAdc = 0.0f;
    _voltageRMS = 0;
    _frequency = NOMINAL_FREQUENCY;
    _lastCrossingTime = 0;
    _crossingCount = 0;
    _lastMinAdc = 4095;
    _lastMaxAdc = 0;
    _freqHistoryIdx = 0;
    for (int i = 0; i < FREQ_AVG_SIZE; i++) {
        _freqHistory[i] = NOMINAL_FREQUENCY;
    }
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
    
    const int calibrationSamples = 5000;

    // Welford online algorithm for mean + variance
    double mean = 0.0;
    double m2 = 0.0;
    for (int i = 0; i < calibrationSamples; i++) {
        double x = (double)analogRead(_pin);
        double delta = x - mean;
        mean += delta / (double)(i + 1);
        double delta2 = x - mean;
        m2 += delta * delta2;
        delayMicroseconds(50);
    }

    _offset = (float)mean;
    double variance = (calibrationSamples > 1) ? (m2 / (double)(calibrationSamples - 1)) : 0.0;
    _noiseRmsAdc = (float)sqrt(variance);

    Serial.printf("[VoltageSensor] Pin %d offset calibrated: %.1f (noise RMS: %.2f ADC)\n",
                  _pin, _offset, _noiseRmsAdc);
}

void VoltageSensor::setSensitivity(float sensitivity) {
    _sensitivity = sensitivity;
}

float VoltageSensor::getSensitivity() const {
    return _sensitivity;
}

float VoltageSensor::readRMS(int samples) {
    uint64_t sumSquares = 0;
    int lastValue = analogRead(_pin);
    bool wasAboveZero = lastValue > _offset;
    
    unsigned long startTime = micros();
    int zeroCrossings = 0;
    unsigned long firstCrossingTime = 0;
    unsigned long lastCrossingTime = 0;
    
    // Reset ADC range tracking
    int minAdc = 4095;
    int maxAdc = 0;
    
    for (int i = 0; i < samples; i++) {
        int raw = analogRead(_pin);
        
        // Track min/max for diagnostics
        if (raw < minAdc) minAdc = raw;
        if (raw > maxAdc) maxAdc = raw;
        
        // Calculate deviation from offset
        int32_t deviation = (int32_t)raw - (int32_t)lroundf(_offset);
        sumSquares += (uint64_t)((int64_t)deviation * (int64_t)deviation);
        
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
    
    // Save ADC range for diagnostics
    _lastMinAdc = minAdc;
    _lastMaxAdc = maxAdc;
    
    // Calculate RMS from ADC values
    double meanSquare = (samples > 0) ? ((double)sumSquares / (double)samples) : 0.0;
    float rmsADC = (float)sqrt(meanSquare);
    
    // Dynamic noise gate based on calibrated noise level
    // If RMS is less than 6x the calibrated noise, consider it noise
    // This filters out random fluctuations while allowing real signals through
    float noiseThresholdAdc = _noiseRmsAdc * 6.0f;
    if (noiseThresholdAdc < 10.0f) noiseThresholdAdc = 10.0f;  // Minimum threshold
    
    if (rmsADC < noiseThresholdAdc) {
        _voltageRMS = 0.0;
    } else {
        // Convert ADC RMS to voltage using calibration coefficient
        _voltageRMS = rmsADC * _sensitivity;
    }
    
    // Additional voltage threshold - anything below 50V is noise without mains
    // Real mains voltage should be 180V+ even with significant sag
    if (_voltageRMS < 50.0f) {
        _voltageRMS = 0.0;
    }
    
    // Calculate frequency from zero crossings
    // We count zero crossings (both rising and falling). There are (zeroCrossings - 1)
    // intervals between crossings, each interval is half a cycle. Therefore
    // freq = (zeroCrossings - 1) / (2 * timePeriod)
    if (zeroCrossings >= 4 && lastCrossingTime > firstCrossingTime) {
        double timePeriod = (lastCrossingTime - firstCrossingTime) / 1000000.0; // seconds
        double halfIntervals = (double)(zeroCrossings - 1); // number of half-cycle intervals

        if (timePeriod > 0.0 && halfIntervals > 0.0) {
            double rawFreq = halfIntervals / (2.0 * timePeriod);

            // Sanity check - accept reasonable grid frequency range (adjustable)
            if (rawFreq >= 45.0 && rawFreq <= 55.0) {
                // Add to averaging buffer
                _freqHistory[_freqHistoryIdx] = (float)rawFreq;
                _freqHistoryIdx = (_freqHistoryIdx + 1) % FREQ_AVG_SIZE;

                // Calculate average
                float sum = 0;
                for (int i = 0; i < FREQ_AVG_SIZE; i++) {
                    sum += _freqHistory[i];
                }
                _frequency = sum / FREQ_AVG_SIZE;
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

void VoltageSensor::getAdcRange(int& minAdc, int& maxAdc) const {
    minAdc = _lastMinAdc;
    maxAdc = _lastMaxAdc;
}
