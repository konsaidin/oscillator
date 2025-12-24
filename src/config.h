#pragma once

// WiFi Configuration
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// ADC Pins for ZMPT101B
#define PIN_PHASE_A 1
#define PIN_PHASE_B 2
#define PIN_PHASE_C 3

// Calibration Constants (Initial values, should be tunable via Web UI)
// ZMPT101B usually needs calibration. 
// V_ACTUAL = (ADC_VALUE - OFFSET) * SENSITIVITY
#define SENSITIVITY_A 0.55
#define SENSITIVITY_B 0.55
#define SENSITIVITY_C 0.55

// Grid Parameters
#define GRID_FREQ 50
#define NOMINAL_VOLTAGE 220
#define VOLTAGE_THRESHOLD_LOW 198  // -10%
#define VOLTAGE_THRESHOLD_HIGH 242 // +10%

// Sampling
#define SAMPLES_PER_CYCLE 100 // Number of samples per mains cycle
#define SAMPLING_FREQUENCY (GRID_FREQ * SAMPLES_PER_CYCLE)
