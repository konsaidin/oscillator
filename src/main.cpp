#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "config.h"
#include "VoltageSensor.h"
#include "PowerAnalyzer.h"

// Objects
VoltageSensor sensorA(PIN_PHASE_A, SENSITIVITY_A);
VoltageSensor sensorB(PIN_PHASE_B, SENSITIVITY_B);
VoltageSensor sensorC(PIN_PHASE_C, SENSITIVITY_C);
PowerAnalyzer analyzer(&sensorA, &sensorB, &sensorC);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Timer for updates
unsigned long lastUpdate = 0;
const long updateInterval = 1000; // 1 sec

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("Websocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("Websocket client disconnected");
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialize Sensors
    sensorA.begin();
    sensorB.begin();
    sensorC.begin();

    // Initialize LittleFS
    if(!LittleFS.begin(true)){
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Web Server Routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", "text/html");
    });
    
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/style.css", "text/css");
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/script.js", "text/javascript");
    });

    // WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.begin();
}

void loop() {
    // Non-blocking update loop
    unsigned long currentMillis = millis();
    
    // Continuous analysis (blocking inside readRMS for short periods is okay for this simple example)
    // For production, use FreeRTOS tasks.
    analyzer.update();

    if (currentMillis - lastUpdate >= updateInterval) {
        lastUpdate = currentMillis;
        
        String json = analyzer.getJsonStatus();
        ws.textAll(json);
        
        Serial.println(json);
    }
    
    ws.cleanupClients();
}
