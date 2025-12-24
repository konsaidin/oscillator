#pragma once

// =============================================================================
// WiFi Configuration
// =============================================================================
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define WIFI_CONNECT_TIMEOUT_SEC 30

// =============================================================================
// InfluxDB Configuration
// =============================================================================
#define INFLUXDB_URL "http://192.168.1.100:8086"
#define INFLUXDB_ORG "home"
#define INFLUXDB_BUCKET "power_monitoring"
#define INFLUXDB_TOKEN "my-super-secret-token"

// =============================================================================
// Device Configuration
// =============================================================================
#define DEVICE_ID "esp32-001"

// =============================================================================
// ADC Pins for ZMPT101B sensors (use ADC1 only - ADC2 conflicts with WiFi)
// =============================================================================
#define PIN_PHASE_A 1   // GPIO1 - ADC1_CH0
#define PIN_PHASE_B 2   // GPIO2 - ADC1_CH1
#define PIN_PHASE_C 3   // GPIO3 - ADC1_CH2

// Status LED (built-in on ESP32-S3-DevKitC)
#define PIN_LED 48

// =============================================================================
// Calibration Coefficients
// Each ZMPT101B module needs individual calibration
// Formula: V_actual = ADC_RMS * CALIBRATION_COEFF
// Adjust these values after calibration with a multimeter
// =============================================================================
#define CALIBRATION_COEFF_A 0.23    // Calibration coefficient for Phase A
#define CALIBRATION_COEFF_B 0.23    // Calibration coefficient for Phase B
#define CALIBRATION_COEFF_C 0.23    // Calibration coefficient for Phase C

// =============================================================================
// Grid Parameters
// =============================================================================
#define NOMINAL_FREQUENCY 50.0f     // Hz (50 for Europe/Asia, 60 for Americas)
#define NOMINAL_VOLTAGE 220.0f      // Nominal phase voltage
#define VOLTAGE_MIN 198.0f          // -10% threshold
#define VOLTAGE_MAX 242.0f          // +10% threshold
#define PHASE_LOSS_THRESHOLD 50.0f  // Voltage below this = phase loss
#define UNBALANCE_THRESHOLD 4.0f    // Max allowed unbalance %
#define FREQUENCY_DEVIATION_THRESHOLD 0.4f  // Max freq deviation from nominal

// =============================================================================
// Sampling Configuration
// =============================================================================
#define SAMPLES_PER_READING 2000    // Number of ADC samples for one RMS calculation
#define SAMPLE_INTERVAL_US 100      // Microseconds between samples (10kHz = 100us)
#define READINGS_PER_SECOND 1       // How often to calculate and send data

// =============================================================================
// ADC Configuration
// =============================================================================
#define ADC_RESOLUTION 12           // 12-bit ADC
#define ADC_MAX_VALUE 4095          // 2^12 - 1
#define ADC_VREF 3.3                // Reference voltage
#define ADC_OFFSET 2048             // Zero offset (VCC/2 = 1.65V ≈ 2048)

// =============================================================================
// Timing Configuration
// =============================================================================
#define SEND_INTERVAL_MS 1000       // How often to send data (1 second)
#define HTTP_TIMEOUT_MS 5000        // HTTP request timeout
