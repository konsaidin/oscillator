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
 * - Встроенный RGB LED (GPIO 48)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "PowerAnalyzer.h"
#include "InfluxClient.h"
#include "Oscilloscope.h"
#include "WebConfig.h"

// NTP Configuration
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 0        // UTC time
#define DAYLIGHT_OFFSET_SEC 0   // No DST

// Глобальные объекты
PowerAnalyzer analyzer;
InfluxClient influxClient;
Oscilloscope oscilloscope(PIN_PHASE_A, PIN_PHASE_B, PIN_PHASE_C);

// Тайминги
unsigned long lastMeasurement = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastStatusPrint = 0;
unsigned long lastWaveform = 0;

// Интервал отправки waveform (2 секунды)
#define WAVEFORM_SEND_INTERVAL_MS 2000

// Счётчики
unsigned long measurementCount = 0;
unsigned long wifiReconnects = 0;

// Последние измерения для веб-интерфейса
PowerData lastPowerData;

// LED пин для индикации (встроенный на ESP32-S3-DevKitC-1)
#ifndef LED_BUILTIN
#define LED_BUILTIN 48  // RGB LED на ESP32-S3-DevKitC-1
#endif

// WiFi события для автопереподключения
void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("[WiFi] Disconnected, will auto-reconnect...");
            wifiReconnects++;
            // ESP32 автоматически переподключается если setAutoReconnect(true)
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("[WiFi] Connected to AP");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf("[WiFi] Got IP: %s\n", WiFi.localIP().toString().c_str());
            break;
        default:
            break;
    }
}

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
 * Синхронизация времени через NTP
 */
bool syncTime() {
    Serial.println("[NTP] Syncing time...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    // Ждём синхронизации (максимум 10 секунд)
    int attempts = 0;
    time_t now = time(nullptr);
    while (now < 1700000000 && attempts < 20) {  // 1700000000 = ~2023 год
        delay(500);
        Serial.print(".");
        now = time(nullptr);
        attempts++;
    }
    Serial.println();
    
    if (now > 1700000000) {
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        Serial.printf("[NTP] Time synced: %04d-%02d-%02d %02d:%02d:%02d UTC\n",
                      timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                      timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        return true;
    } else {
        Serial.println("[NTP] Time sync failed!");
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
        if (connectWiFi()) {
            syncTime();  // Пересинхронизируем время после реконнекта
        }
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
    
    // ADC Diagnostics - показывает реальный диапазон ADC для проверки клиппинга
    Serial.println();
    Serial.println("[ADC DIAGNOSTICS]");
    float offA, offB, offC;
    analyzer.getOffsets(offA, offB, offC);
    int minA, maxA, minB, maxB, minC, maxC;
    analyzer.getAdcRanges(minA, maxA, minB, maxB, minC, maxC);
    Serial.printf("  Phase A: offset=%.0f, ADC range=[%d - %d], swing=%d\n", offA, minA, maxA, maxA - minA);
    Serial.printf("  Phase B: offset=%.0f, ADC range=[%d - %d], swing=%d\n", offB, minB, maxB, maxB - minB);
    Serial.printf("  Phase C: offset=%.0f, ADC range=[%d - %d], swing=%d\n", offC, minC, maxC, maxC - minC);
    
    // Проверка на клиппинг
    if (minA < 50 || maxA > 4045) Serial.println("  ⚠️  Phase A: possible CLIPPING!");
    if (minB < 50 || maxB > 4045) Serial.println("  ⚠️  Phase B: possible CLIPPING!");
    if (minC < 50 || maxC > 4045) Serial.println("  ⚠️  Phase C: possible CLIPPING!");
    
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
    
    // ВАЖНО: Сначала инициализируем WiFi стек
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(WiFiEvent);  // Регистрируем обработчик событий WiFi
    delay(100);  // Даём время на инициализацию lwIP стека

    // NOTE: Web configuration panel temporarily disabled — use hardcoded config
    // Попробуем подключиться к жёстко заданной сети из config.h
    if (!connectWiFi()) {
        Serial.println("[WiFi] Connection failed (hardcoded). Continuing without AP or web panel.");
        // Do not start AP mode — web panel is disabled for now
    }
    
    // Синхронизация времени через NTP
    if (!syncTime()) {
        Serial.println("[WARNING] Time sync failed, timestamps may be incorrect!");
        // Продолжаем работу, но данные могут не записаться в InfluxDB
    }
    
    // Быстрая проверка доступности InfluxDB (используем настройки из WebConfig)
    influxClient.begin(
        INFLUXDB_URL, 
        INFLUXDB_ORG, 
        INFLUXDB_BUCKET, 
        INFLUXDB_TOKEN
    );
    
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
    
    // Инициализация осциллографа
    oscilloscope.begin();
    Serial.println("[Oscilloscope] Initialized");
    
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
    
    // Web panel disabled — no webConfig handling
    
    // Периодическая проверка WiFi (каждые 60 секунд)
    // WiFi автоматически переподключается, но проверяем для лога
    if (currentTime - lastWifiCheck >= 60000) {
        lastWifiCheck = currentTime;
        if (!webConfig.isAPMode()) {
            if (webConfig.isConnected()) {
                Serial.printf("[WiFi] Status: Connected, RSSI: %d dBm\n", WiFi.RSSI());
            } else {
                Serial.println("[WiFi] Status: Disconnected (auto-reconnecting...)");
            }
        }
    }
    
    // Основной цикл измерений
    if (currentTime - lastMeasurement >= SEND_INTERVAL_MS) {
        lastMeasurement = currentTime;
        measurementCount++;
        
        // Индикация начала измерения
        digitalWrite(LED_BUILTIN, HIGH);
        
        // Измерение
        PowerData data = analyzer.measure();
        lastPowerData = data;  // Сохраняем для веб-интерфейса
        
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
    
    // Отправка waveform для осциллографа (раз в 5 секунд)
    if (currentTime - lastWaveform >= WAVEFORM_SEND_INTERVAL_MS) {
        lastWaveform = currentTime;
        
        // Захватываем waveform (~20ms блокировка)
        oscilloscope.capture();
        
        // Получаем offset'ы от анализатора для центрирования
        float offsetA = ADC_OFFSET;
        float offsetB = ADC_OFFSET;
        float offsetC = ADC_OFFSET;
        analyzer.getOffsets(offsetA, offsetB, offsetC);

        // sanity check (на случай, если калибровка ещё не прошла или вход "висит")
        if (offsetA < 0 || offsetA > ADC_MAX_VALUE) offsetA = ADC_OFFSET;
        if (offsetB < 0 || offsetB > ADC_MAX_VALUE) offsetB = ADC_OFFSET;
        if (offsetC < 0 || offsetC > ADC_MAX_VALUE) offsetC = ADC_OFFSET;

        String waveformData = oscilloscope.toLineProtocol(
            DEVICE_ID, 
            offsetA, offsetB, offsetC
        );
        
        // Отправляем в InfluxDB
        SendStatus wfStatus = influxClient.send(waveformData);
        if (wfStatus == SendStatus::SUCCESS) {
            Serial.println("[Oscilloscope] Waveform sent");
        } else {
            Serial.println("[Oscilloscope] Waveform send failed");
        }
    }
    
    // Небольшая задержка для стабильности
    delay(10);
}
