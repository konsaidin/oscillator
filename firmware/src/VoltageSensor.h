#pragma once
#include <Arduino.h>
#include "config.h"

/**
 * VoltageSensor - Class for reading AC voltage from ZMPT101B sensor
 * 
 * Uses RMS (Root Mean Square) calculation for accurate AC voltage measurement.
 * Includes automatic offset calibration and zero-crossing frequency detection.
 */
class VoltageSensor {
private:
    int _pin;
    float _sensitivity;
    float _offset;
    float _voltageRMS;
    float _frequency;
    
    // Zero-crossing detection
    unsigned long _lastCrossingTime;
    int _crossingCount;
    
public:
    /**
     * Constructor
     * @param pin GPIO pin connected to ZMPT101B output
     * @param sensitivity Calibration coefficient (V_actual = ADC_RMS * sensitivity)
     */
    VoltageSensor(int pin, float sensitivity);
    
    /**
     * Initialize the sensor and calibrate offset
     */
    void begin();
    
    /**
     * Calibrate the DC offset (should be called when no AC is connected or at startup)
     * Samples the ADC and calculates the average as the zero point
     */
    void calibrateOffset();
    
    /**
     * Set sensitivity coefficient for calibration
     * @param sensitivity New sensitivity value
     */
    void setSensitivity(float sensitivity);
    
    /**
     * Get current sensitivity coefficient
     */
    float getSensitivity() const;
    
    /**
     * Read RMS voltage from the sensor
     * @param samples Number of ADC samples to take (default: SAMPLES_PER_READING)
     * @return RMS voltage in Volts
     */
    float readRMS(int samples = SAMPLES_PER_READING);
    
    /**
     * Get the last calculated RMS voltage without new measurement
     */
    float getLastRMS() const;
    
    /**
     * Get the detected frequency from zero-crossing
     */
    float getFrequency() const;
    
    /**
     * Read raw ADC value
     */
    int readRaw();
    
    /**
     * Get the calibrated offset value
     */
    float getOffset() const;
};
