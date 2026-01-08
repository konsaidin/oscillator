#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "config.h"

#define DNS_PORT 53

class WebConfig {
public:
    WebConfig();
    
    // Load configuration from NVS (call before network init)
    void loadConfig();
    
    // Initialize web server (call after WiFi is ready)
    void begin();
    
    // Must be called in loop()
    void handle();
    
    // Start AP mode for configuration
    void startAPMode();
    
    // Try to connect to saved WiFi
    bool connectToSavedWiFi();
    
    // Check if WiFi is connected
    bool isConnected();
    
    // Check if AP mode is active
    bool isAPMode();
    
    // Get current IP address
    String getIPAddress();
    
    // Get saved WiFi SSID
    String getSavedSSID();
    
    // Calibration coefficients
    float getCalibCoeffA();
    float getCalibCoeffB();
    float getCalibCoeffC();
    
    // InfluxDB settings
    String getInfluxURL();
    String getInfluxOrg();
    String getInfluxBucket();
    String getInfluxToken();
    
    // Device ID
    String getDeviceID();

private:
    WebServer _server;
    DNSServer _dnsServer;
    Preferences _prefs;
    bool _apMode;
    unsigned long _lastScanTime;
    String _scanResults;
    bool _scanInProgress;
    String _cachedScanResults;
    
    // Saved configuration
    String _ssid;
    String _password;
    String _influxURL;
    String _influxOrg;
    String _influxBucket;
    String _influxToken;
    String _deviceID;
    float _calibA;
    float _calibB;
    float _calibC;
    
    // Save configuration
    void saveConfig();
    
    // HTTP handlers
    void handleRoot();
    void handleScan();
    void handleConnect();
    void handleConfig();
    void handleSaveConfig();
    void handleStatus();
    void handleReboot();
    void handleNotFound();
    
    // HTML generation
    String getHTMLHeader(const String& title);
    String getHTMLFooter();
    String getMainPage();
    String getConfigPage();
    String getStatusJSON();
};

extern WebConfig webConfig;
