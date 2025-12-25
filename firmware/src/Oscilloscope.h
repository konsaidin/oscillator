#ifndef OSCILLOSCOPE_H
#define OSCILLOSCOPE_H

#include <Arduino.h>
#include "config.h"

// Параметры захвата waveform
#define WAVEFORM_SAMPLES 100      // Точек на фазу (2-3 периода при 50Hz)
#define WAVEFORM_INTERVAL_US 200  // Интервал между отсчётами (200us = 5kHz)

/**
 * Структура для хранения waveform данных трёх фаз
 */
struct WaveformData {
    int16_t phaseA[WAVEFORM_SAMPLES];
    int16_t phaseB[WAVEFORM_SAMPLES];
    int16_t phaseC[WAVEFORM_SAMPLES];
    uint32_t sampleCount;
    unsigned long captureTime;  // millis() когда захвачено
};

/**
 * Класс для захвата осциллограмм трёх фаз
 * Захватывает "сырые" данные ADC для отображения синусоид
 */
class Oscilloscope {
public:
    Oscilloscope(int pinA, int pinB, int pinC);
    
    /**
     * Инициализация
     */
    void begin();
    
    /**
     * Захватить waveform всех трёх фаз одновременно
     * Блокирующий вызов ~20-40ms
     */
    void capture();
    
    /**
     * Получить последние захваченные данные
     */
    const WaveformData& getData() const;
    
    /**
     * Сформировать Line Protocol для отправки в InfluxDB
     * @param deviceId ID устройства
     * @param offset смещение ADC (для центрирования)
     * @return Строка Line Protocol (много строк через \n)
     */
    String toLineProtocol(const char* deviceId, float offsetA, float offsetB, float offsetC) const;

private:
    int _pinA, _pinB, _pinC;
    WaveformData _data;
};

#endif // OSCILLOSCOPE_H
