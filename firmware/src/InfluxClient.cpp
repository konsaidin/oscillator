#include "InfluxClient.h"

InfluxClient::InfluxClient() 
    : lastStatus(SendStatus::SUCCESS),
      lastHttpCode(0),
      successCount(0),
      failCount(0) {
}

void InfluxClient::begin(const char* url, const char* org, const char* bucket, const char* token) {
    serverUrl = url;
    organization = org;
    bucketName = bucket;
    authToken = token;
    
    buildWriteUrl();
    
    Serial.println("[InfluxClient] Initialized");
    Serial.print("[InfluxClient] Write URL: ");
    Serial.println(writeUrl);
}

void InfluxClient::buildWriteUrl() {
    // InfluxDB 2.x API endpoint для записи:
    // POST /api/v2/write?org=<org>&bucket=<bucket>&precision=ms
    
    writeUrl = serverUrl;
    if (!writeUrl.endsWith("/")) {
        writeUrl += "/";
    }
    writeUrl += "api/v2/write?org=";
    writeUrl += organization;
    writeUrl += "&bucket=";
    writeUrl += bucketName;
    writeUrl += "&precision=ms";
}

SendStatus InfluxClient::send(const String& lineProtocol) {
    // Проверяем подключение к WiFi
    if (WiFi.status() != WL_CONNECTED) {
        lastStatus = SendStatus::WIFI_DISCONNECTED;
        failCount++;
        Serial.println("[InfluxClient] Error: WiFi disconnected");
        return lastStatus;
    }
    
    // Добавляем timestamp в миллисекундах если не указан
    String payload = lineProtocol;
    if (payload.indexOf(' ', payload.lastIndexOf('=')) == -1) {
        // Нет пробела после последнего значения - нет timestamp
        payload += " ";
        payload += String(millis());  // Для precision=ms
    }
    
    int httpCode = httpPost(payload);
    
    if (httpCode == 204) {
        // 204 No Content - успешная запись
        lastStatus = SendStatus::SUCCESS;
        successCount++;
        return lastStatus;
    } else if (httpCode < 0) {
        lastStatus = SendStatus::CONNECTION_FAILED;
        failCount++;
        Serial.printf("[InfluxClient] Connection failed: %d\n", httpCode);
        return lastStatus;
    } else {
        lastStatus = SendStatus::HTTP_ERROR;
        failCount++;
        Serial.printf("[InfluxClient] HTTP error: %d\n", httpCode);
        return lastStatus;
    }
}

SendStatus InfluxClient::sendBatch(const String* lines, size_t count) {
    if (count == 0) {
        return SendStatus::SUCCESS;
    }
    
    // Объединяем все строки через перенос строки
    String payload;
    for (size_t i = 0; i < count; i++) {
        payload += lines[i];
        if (i < count - 1) {
            payload += "\n";
        }
    }
    
    return send(payload);
}

int InfluxClient::httpPost(const String& payload) {
    HTTPClient http;
    
    // Настройка таймаутов
    http.setTimeout(HTTP_TIMEOUT_MS);
    http.setConnectTimeout(HTTP_TIMEOUT_MS);
    
    if (!http.begin(writeUrl)) {
        Serial.println("[InfluxClient] Failed to begin HTTP");
        return -1;
    }
    
    // Заголовки
    http.addHeader("Content-Type", "text/plain");
    http.addHeader("Authorization", "Token " + authToken);
    
    // Отправка
    lastHttpCode = http.POST(payload);
    
    // Если ошибка, выводим тело ответа для отладки
    if (lastHttpCode != 204 && lastHttpCode > 0) {
        String response = http.getString();
        Serial.printf("[InfluxClient] Response (%d): %s\n", lastHttpCode, response.c_str());
    }
    
    http.end();
    
    return lastHttpCode;
}

String InfluxClient::getLastStatusString() const {
    switch (lastStatus) {
        case SendStatus::SUCCESS:
            return "SUCCESS";
        case SendStatus::WIFI_DISCONNECTED:
            return "WIFI_DISCONNECTED";
        case SendStatus::CONNECTION_FAILED:
            return "CONNECTION_FAILED";
        case SendStatus::HTTP_ERROR:
            return "HTTP_ERROR";
        case SendStatus::TIMEOUT:
            return "TIMEOUT";
        default:
            return "UNKNOWN";
    }
}

int InfluxClient::getLastHttpCode() const {
    return lastHttpCode;
}

unsigned long InfluxClient::getSuccessCount() const {
    return successCount;
}

unsigned long InfluxClient::getFailCount() const {
    return failCount;
}

void InfluxClient::resetCounters() {
    successCount = 0;
    failCount = 0;
}

bool InfluxClient::ping() {
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }
    
    HTTPClient http;
    http.setTimeout(5000);
    
    String pingUrl = serverUrl;
    if (!pingUrl.endsWith("/")) {
        pingUrl += "/";
    }
    pingUrl += "ping";
    
    if (!http.begin(pingUrl)) {
        return false;
    }
    
    int httpCode = http.GET();
    http.end();
    
    // InfluxDB возвращает 204 на /ping
    return (httpCode == 204);
}
