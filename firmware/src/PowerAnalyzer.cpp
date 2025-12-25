#include "PowerAnalyzer.h"
#include <algorithm>
#include <cmath>

PowerAnalyzer::PowerAnalyzer() 
    : sensorA(PIN_PHASE_A, CALIBRATION_COEFF_A),
      sensorB(PIN_PHASE_B, CALIBRATION_COEFF_B),
      sensorC(PIN_PHASE_C, CALIBRATION_COEFF_C) {
    memset(&lastData, 0, sizeof(lastData));
}

void PowerAnalyzer::begin() {
    Serial.println("[PowerAnalyzer] Initializing sensors...");
    
    sensorA.begin();
    sensorB.begin();
    sensorC.begin();
    
    // Небольшая задержка для стабилизации ADC
    delay(100);
    
    // Автокалибровка смещения
    calibrate();
    
    Serial.println("[PowerAnalyzer] Initialization complete");
}

void PowerAnalyzer::calibrate() {
    Serial.println("[PowerAnalyzer] Calibrating offset...");
    
    sensorA.calibrateOffset();
    sensorB.calibrateOffset();
    sensorC.calibrateOffset();
    
    Serial.println("[PowerAnalyzer] Calibration complete");
}

PowerData PowerAnalyzer::measure() {
    PowerData data;
    
    // Читаем RMS напряжения с каждой фазы
    // Измеряем последовательно для точности (не параллельно)
    data.voltageA = sensorA.readRMS();
    data.frequencyA = sensorA.getFrequency();
    
    data.voltageB = sensorB.readRMS();
    data.frequencyB = sensorB.getFrequency();
    
    data.voltageC = sensorC.readRMS();
    data.frequencyC = sensorC.getFrequency();
    
    // Среднее напряжение
    data.voltageAvg = (data.voltageA + data.voltageB + data.voltageC) / 3.0f;
    
    // Средняя частота
    data.frequencyAvg = (data.frequencyA + data.frequencyB + data.frequencyC) / 3.0f;
    
    // Межфазные (линейные) напряжения
    // Для симметричной трёхфазной системы: Uл = Uф * √3
    // Для реальной системы с учётом сдвига фаз на 120°:
    // Uab = √(Ua² + Ub² - 2*Ua*Ub*cos(120°)) = √(Ua² + Ub² + Ua*Ub)
    // Упрощённо (для почти симметричной системы): Uл ≈ Uф * 1.732
    data.voltageAB = sqrt(data.voltageA * data.voltageA + 
                         data.voltageB * data.voltageB + 
                         data.voltageA * data.voltageB);
    
    data.voltageBC = sqrt(data.voltageB * data.voltageB + 
                         data.voltageC * data.voltageC + 
                         data.voltageB * data.voltageC);
    
    data.voltageCA = sqrt(data.voltageC * data.voltageC + 
                         data.voltageA * data.voltageA + 
                         data.voltageC * data.voltageA);
    
    // Перекос фаз
    data.unbalance = calculateUnbalance(data.voltageA, data.voltageB, data.voltageC);
    
    // Метка времени
    data.timestamp = millis();
    
    // Проверка пороговых значений
    checkThresholds(data);
    
    // Сохраняем последние данные
    lastData = data;
    
    return data;
}

PowerData PowerAnalyzer::getLastData() const {
    return lastData;
}

float PowerAnalyzer::calculateUnbalance(float ua, float ub, float uc) {
    // Коэффициент несимметрии напряжений по обратной последовательности
    // Упрощённый расчёт по ГОСТ 13109-97:
    // K2U = (Umax - Umin) / Uavg * 100%
    
    float maxV = std::max({ua, ub, uc});
    float minV = std::min({ua, ub, uc});
    float avgV = (ua + ub + uc) / 3.0f;
    
    if (avgV < 1.0f) {
        return 0.0f;  // Избегаем деления на ноль при отсутствии напряжения
    }
    
    return ((maxV - minV) / avgV) * 100.0f;
}

void PowerAnalyzer::checkThresholds(PowerData& data) {
    // Проверка низкого напряжения (< 198В для 220В сети, -10%)
    float lowThreshold = NOMINAL_VOLTAGE * 0.9f;
    data.lowVoltage = (data.voltageA < lowThreshold || 
                       data.voltageB < lowThreshold || 
                       data.voltageC < lowThreshold);
    
    // Проверка высокого напряжения (> 242В для 220В сети, +10%)
    float highThreshold = NOMINAL_VOLTAGE * 1.1f;
    data.highVoltage = (data.voltageA > highThreshold || 
                        data.voltageB > highThreshold || 
                        data.voltageC > highThreshold);
    
    // Перекос фаз > 2% - норма по ГОСТ 13109-97
    // > 4% - предельно допустимое
    data.highUnbalance = (data.unbalance > UNBALANCE_THRESHOLD);
    
    // Отклонение частоты > 0.4 Гц от 50 Гц
    data.frequencyDeviation = (fabs(data.frequencyAvg - NOMINAL_FREQUENCY) > FREQUENCY_DEVIATION_THRESHOLD);
}

String PowerAnalyzer::toLineProtocol(const char* deviceId) const {
    // Формат InfluxDB Line Protocol (без timestamp — InfluxDB выставит server time):
    // voltage,device=...,phase=A value=221.5
    // frequency,device=... value=50.02
    // unbalance,device=... value=1.23
    // line_voltage,device=...,phases=AB value=383.5

    String lines;

    // Фазные напряжения
    lines += "voltage,device=";
    lines += deviceId;
    lines += ",phase=A value=";
    lines += String(lastData.voltageA, 2);
    lines += "\n";

    lines += "voltage,device=";
    lines += deviceId;
    lines += ",phase=B value=";
    lines += String(lastData.voltageB, 2);
    lines += "\n";

    lines += "voltage,device=";
    lines += deviceId;
    lines += ",phase=C value=";
    lines += String(lastData.voltageC, 2);
    lines += "\n";

    // Межфазные напряжения
    lines += "line_voltage,device=";
    lines += deviceId;
    lines += ",phases=AB value=";
    lines += String(lastData.voltageAB, 2);
    lines += "\n";

    lines += "line_voltage,device=";
    lines += deviceId;
    lines += ",phases=BC value=";
    lines += String(lastData.voltageBC, 2);
    lines += "\n";

    lines += "line_voltage,device=";
    lines += deviceId;
    lines += ",phases=CA value=";
    lines += String(lastData.voltageCA, 2);
    lines += "\n";

    // Частота и перекос
    lines += "frequency,device=";
    lines += deviceId;
    lines += " value=";
    lines += String(lastData.frequencyAvg, 2);
    lines += "\n";

    lines += "unbalance,device=";
    lines += deviceId;
    lines += " value=";
    lines += String(lastData.unbalance, 2);

    return lines;
}

bool PowerAnalyzer::hasProblems() const {
    return lastData.lowVoltage || 
           lastData.highVoltage || 
           lastData.highUnbalance || 
           lastData.frequencyDeviation;
}

String PowerAnalyzer::getProblemsDescription() const {
    if (!hasProblems()) {
        return "OK";
    }
    
    String desc = "";
    
    if (lastData.lowVoltage) {
        desc += "LOW_V ";
    }
    if (lastData.highVoltage) {
        desc += "HIGH_V ";
    }
    if (lastData.highUnbalance) {
        desc += "UNBALANCE ";
    }
    if (lastData.frequencyDeviation) {
        desc += "FREQ_DEV ";
    }
    
    desc.trim();
    return desc;
}
