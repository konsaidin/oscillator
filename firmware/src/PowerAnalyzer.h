#ifndef POWER_ANALYZER_H
#define POWER_ANALYZER_H

#include <Arduino.h>
#include "VoltageSensor.h"
#include "config.h"

/**
 * Структура для хранения результатов измерений трёхфазной сети
 */
struct PowerData {
    // Фазные напряжения (RMS)
    float voltageA;
    float voltageB;
    float voltageC;
    
    // Частоты фаз (Гц)
    float frequencyA;
    float frequencyB;
    float frequencyC;
    float frequencyAvg;
    
    // Межфазные (линейные) напряжения
    float voltageAB;  // Uab = Ua * √3 (для симметричной нагрузки)
    float voltageBC;
    float voltageCA;
    
    // Перекос фаз (%)
    float unbalance;
    
    // Среднее напряжение
    float voltageAvg;
    
    // Метка времени
    unsigned long timestamp;
    
    // Флаги проблем
    bool lowVoltage;
    bool highVoltage;
    bool highUnbalance;
    bool frequencyDeviation;
};

/**
 * Класс для анализа трёхфазной сети
 * Собирает данные с 3 датчиков ZMPT101B и вычисляет производные величины
 */
class PowerAnalyzer {
public:
    PowerAnalyzer();
    
    /**
     * Инициализация анализатора и всех датчиков
     */
    void begin();
    
    /**
     * Калибровка смещения всех датчиков (вызывать при отсутствии напряжения или после прогрева)
     */
    void calibrate();
    
    /**
     * Выполнить измерение и анализ всех параметров
     * @return Структура с результатами измерений
     */
    PowerData measure();
    
    /**
     * Получить последние измеренные данные без нового измерения
     */
    PowerData getLastData() const;
    
    /**
     * Форматировать данные в InfluxDB Line Protocol
     * @param deviceId Идентификатор устройства
     * @return Строка в формате Line Protocol
     */
    String toLineProtocol(const char* deviceId) const;
    
    /**
     * Проверить наличие проблем в последнем измерении
     * @return true если есть проблемы
     */
    bool hasProblems() const;
    
    /**
     * Получить текстовое описание проблем
     */
    String getProblemsDescription() const;

private:
    VoltageSensor sensorA;
    VoltageSensor sensorB;
    VoltageSensor sensorC;
    
    PowerData lastData;
    
    /**
     * Вычислить коэффициент перекоса фаз по ГОСТ 13109-97
     * K2U = (Umax - Umin) / Uavg * 100%
     */
    float calculateUnbalance(float ua, float ub, float uc);
    
    /**
     * Проверить пороговые значения и установить флаги проблем
     */
    void checkThresholds(PowerData& data);
};

#endif // POWER_ANALYZER_H
