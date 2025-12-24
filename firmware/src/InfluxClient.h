#ifndef INFLUX_CLIENT_H
#define INFLUX_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"

/**
 * Статус последней отправки
 */
enum class SendStatus {
    SUCCESS,
    WIFI_DISCONNECTED,
    CONNECTION_FAILED,
    HTTP_ERROR,
    TIMEOUT
};

/**
 * Класс для отправки данных в InfluxDB 2.x через HTTP API
 */
class InfluxClient {
public:
    InfluxClient();
    
    /**
     * Инициализация клиента с параметрами подключения
     * @param url URL сервера InfluxDB (например, "http://192.168.1.100:8086")
     * @param org Организация в InfluxDB
     * @param bucket Bucket для записи данных
     * @param token Токен авторизации
     */
    void begin(const char* url, const char* org, const char* bucket, const char* token);
    
    /**
     * Отправить данные в формате Line Protocol
     * @param lineProtocol Строка в формате InfluxDB Line Protocol
     * @return Статус отправки
     */
    SendStatus send(const String& lineProtocol);
    
    /**
     * Отправить несколько строк данных (batch)
     * @param lines Массив строк Line Protocol
     * @param count Количество строк
     * @return Статус отправки
     */
    SendStatus sendBatch(const String* lines, size_t count);
    
    /**
     * Получить текстовое описание последнего статуса
     */
    String getLastStatusString() const;
    
    /**
     * Получить HTTP код последнего ответа
     */
    int getLastHttpCode() const;
    
    /**
     * Получить количество успешных отправок
     */
    unsigned long getSuccessCount() const;
    
    /**
     * Получить количество неудачных отправок
     */
    unsigned long getFailCount() const;
    
    /**
     * Сбросить счётчики
     */
    void resetCounters();
    
    /**
     * Проверить доступность сервера InfluxDB
     * @return true если сервер отвечает
     */
    bool ping();

private:
    String serverUrl;
    String organization;
    String bucketName;
    String authToken;
    String writeUrl;  // Полный URL для записи
    
    SendStatus lastStatus;
    int lastHttpCode;
    unsigned long successCount;
    unsigned long failCount;
    
    /**
     * Построить URL для API записи
     */
    void buildWriteUrl();
    
    /**
     * Выполнить HTTP POST запрос
     * @param payload Тело запроса
     * @return HTTP код ответа или отрицательное значение при ошибке
     */
    int httpPost(const String& payload);
};

#endif // INFLUX_CLIENT_H
