/**
 * ESP32-S3 Three-Phase Power Monitor
 * 
 * Мониторинг трёхфазной сети с помощью 3x ZMPT101B датчиков напряжения.
 * Данные отправляются в InfluxDB, визуализация через Grafana.
 * 
 * Функционал:
 * - Измерение RMS напряжения каждой фазы
 * - Определение частоты сети
 * - Расчёт межфазных (линейных) напряжений
 * - Определение перекоса фаз
 * - Отправка данных в InfluxDB каждую секунду
 * - Индикация состояния через встроенный LED
 * 
 * Аппаратное обеспечение:
 * - ESP32-S3-DevKitC-1
 * - 3x ZMPT101B (GPIO 1, 2, 3)
 * - Встроенный RGB LED (GPIO 48) или внешний LED (GPIO 2)
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "PowerAnalyzer.h"
#include "InfluxClient.h"

// Глобальные объекты
PowerAnalyzer analyzer;
InfluxClient influxClient;

// Тайминги
unsigned long lastMeasurement = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastStatusPrint = 0;

// Счётчики
unsigned long measurementCount = 0;
unsigned long wifiReconnects = 0;

// LED пин для индикации (встроенный на ESP32-S3-DevKitC-1)
#ifndef LED_BUILTIN
#define LED_BUILTIN 48  // RGB LED на ESP32-S3-DevKitC-1 (или 2 для обычного LED)
#endif

/**
 * Подключение к WiFi с таймаутом
 */
bool connectWiFi() {
    Serial.println();
    Serial.print("[WiFi] Connecting to ");
    Serial.println(WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_TIMEOUT_SEC * 2) {
        delay(500);
        Serial.print(".");
        attempts++;
        
        // Мигаем LED
        digitalWrite(LED_BUILTIN, attempts % 2);
    }
    
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Connected!");
        Serial.print("[WiFi] IP address: ");
        Serial.println(WiFi.localIP());
        Serial.print("[WiFi] RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        return true;
    } else {
        Serial.println("[WiFi] Connection failed!");
        return false;
    }
}

/**
 * Проверка и переподключение WiFi
 */
void checkWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Connection lost, reconnecting...");
        wifiReconnects++;
        
        WiFi.disconnect();
        delay(1000);
        connectWiFi();
    }
}

/**
 * Мигание LED для индикации
 * @param count Количество миганий
 * @param onTime Время включения (мс)
 * @param offTime Время выключения (мс)
 */
void blinkLED(int count, int onTime = 100, int offTime = 100) {
    for (int i = 0; i < count; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(onTime);
        digitalWrite(LED_BUILTIN, LOW);
        if (i < count - 1) {
            delay(offTime);
        }
    }
}

/**
 * Вывод статуса в Serial
 */
void printStatus(const PowerData& data) {
    Serial.println("----------------------------------------");
    Serial.printf("Measurement #%lu\n", measurementCount);
    Serial.println("----------------------------------------");
    
    Serial.printf("Phase A: %.1f V @ %.2f Hz\n", data.voltageA, data.frequencyA);
    Serial.printf("Phase B: %.1f V @ %.2f Hz\n", data.voltageB, data.frequencyB);
    Serial.printf("Phase C: %.1f V @ %.2f Hz\n", data.voltageC, data.frequencyC);
    Serial.printf("Average: %.1f V @ %.2f Hz\n", data.voltageAvg, data.frequencyAvg);
    
    Serial.println();
    Serial.printf("Line AB: %.1f V\n", data.voltageAB);
    Serial.printf("Line BC: %.1f V\n", data.voltageBC);
    Serial.printf("Line CA: %.1f V\n", data.voltageCA);
    
    Serial.println();
    Serial.printf("Unbalance: %.2f %%\n", data.unbalance);
    
    if (analyzer.hasProblems()) {
        Serial.printf("⚠️  Problems: %s\n", analyzer.getProblemsDescription().c_str());
    } else {
        Serial.println("✓ All parameters OK");
    }
    
    Serial.println();
    Serial.printf("InfluxDB: sent=%lu, failed=%lu\n", 
                  influxClient.getSuccessCount(), 
                  influxClient.getFailCount());
    Serial.printf("WiFi reconnects: %lu, RSSI: %d dBm\n", 
                  wifiReconnects, WiFi.RSSI());
    Serial.println("----------------------------------------");
    Serial.println();
}

void setup() {
    // Инициализация Serial
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        delay(10);
    }
    
    Serial.println();
    Serial.println("========================================");
    Serial.println("  ESP32-S3 Three-Phase Power Monitor");
    Serial.println("========================================");
    Serial.println();
    
    // Инициализация LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    
    // Подключение к WiFi
    if (!connectWiFi()) {
        Serial.println("[ERROR] WiFi connection failed. Restarting in 10 seconds...");
        delay(10000);
        ESP.restart();
    }
    
    // Быстрая проверка доступности InfluxDB
    influxClient.begin(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
    
    Serial.println("[InfluxDB] Checking connection...");
    if (influxClient.ping()) {
        Serial.println("[InfluxDB] Server is reachable!");
        blinkLED(2, 200, 200);
    } else {
        Serial.println("[InfluxDB] Warning: Server not responding to ping");
        Serial.println("[InfluxDB] Will retry on data send...");
        blinkLED(5, 100, 100);
    }
    
    // Инициализация анализатора напряжения
    Serial.println();
    analyzer.begin();
    
    Serial.println();
    Serial.println("[READY] Starting measurements...");
    Serial.printf("[CONFIG] Interval: %d ms, Device ID: %s\n", 
                  SEND_INTERVAL_MS, DEVICE_ID);
    Serial.println();
    
    // Сигнал готовности
    blinkLED(3, 300, 200);
    
    // Первое измерение
    lastMeasurement = millis() - SEND_INTERVAL_MS;
}

void loop() {
    unsigned long currentTime = millis();
    
    // Периодическая проверка WiFi (каждые 30 секунд)
    if (currentTime - lastWifiCheck >= 30000) {
        lastWifiCheck = currentTime;
        checkWiFi();
    }
    
    // Основной цикл измерений
    if (currentTime - lastMeasurement >= SEND_INTERVAL_MS) {
        lastMeasurement = currentTime;
        measurementCount++;
        
        // Индикация начала измерения
        digitalWrite(LED_BUILTIN, HIGH);
        
        // Измерение
        PowerData data = analyzer.measure();
        
        // Формирование и отправка данных
        String lineProtocol = analyzer.toLineProtocol(DEVICE_ID);
        
        SendStatus status = influxClient.send(lineProtocol);
        
        // Индикация результата
        if (status == SendStatus::SUCCESS) {
            // Успех - короткое мигание
            digitalWrite(LED_BUILTIN, LOW);
        } else {
            // Ошибка - длинное мигание
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
        }
        
        // Вывод статуса каждые 10 секунд
        if (currentTime - lastStatusPrint >= 10000) {
            lastStatusPrint = currentTime;
            printStatus(data);
        }
        
        // При проблемах выводим сразу
        if (analyzer.hasProblems()) {
            Serial.printf("⚠️  [ALERT] %s | A=%.1fV B=%.1fV C=%.1fV | Unb=%.1f%% | F=%.2fHz\n",
                         analyzer.getProblemsDescription().c_str(),
                         data.voltageA, data.voltageB, data.voltageC,
                         data.unbalance, data.frequencyAvg);
        }
    }
    
    // Небольшая задержка для стабильности
    delay(10);
}
