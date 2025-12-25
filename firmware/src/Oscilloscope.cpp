#include "Oscilloscope.h"
#include <time.h>

Oscilloscope::Oscilloscope(int pinA, int pinB, int pinC)
    : _pinA(pinA), _pinB(pinB), _pinC(pinC) {
    memset(&_data, 0, sizeof(_data));
}

void Oscilloscope::begin() {
    pinMode(_pinA, INPUT);
    pinMode(_pinB, INPUT);
    pinMode(_pinC, INPUT);
    analogReadResolution(ADC_RESOLUTION);
    analogSetAttenuation(ADC_11db);
}

void Oscilloscope::capture() {
    // Захватываем все три фазы "почти одновременно"
    // ESP32 ADC работает последовательно, но при 5kHz это достаточно быстро
    
    unsigned long startTime = micros();
    _data.captureTime = millis();
    
    for (int i = 0; i < WAVEFORM_SAMPLES; i++) {
        // Читаем все три канала подряд (минимальная задержка между ними)
        _data.phaseA[i] = analogRead(_pinA);
        _data.phaseB[i] = analogRead(_pinB);
        _data.phaseC[i] = analogRead(_pinC);
        
        // Ждём до следующего отсчёта для точного timing
        while (micros() - startTime < (unsigned long)(i + 1) * WAVEFORM_INTERVAL_US) {
            // busy wait
        }
    }
    
    _data.sampleCount = WAVEFORM_SAMPLES;
}

const WaveformData& Oscilloscope::getData() const {
    return _data;
}

String Oscilloscope::toLineProtocol(const char* deviceId, float offsetA, float offsetB, float offsetC) const {
    // Формат: waveform,device=xxx,phase=A,idx=0 value=123.45
    // idx как TAG для уникальности записи в InfluxDB
    // Без timestamp - InfluxDB назначит сам
    
    String lines;
    lines.reserve(WAVEFORM_SAMPLES * 3 * 70);
    
    for (int i = 0; i < (int)_data.sampleCount; i++) {
        // Нормализуем значения относительно offset (центрируем около 0)
        float valA = (_data.phaseA[i] - offsetA);
        float valB = (_data.phaseB[i] - offsetB);
        float valC = (_data.phaseC[i] - offsetC);
        
        // Phase A - idx как TAG
        lines += "waveform,device=";
        lines += deviceId;
        lines += ",phase=A,idx=";
        lines += String(i);
        lines += " value=";
        lines += String(valA, 1);
        lines += "\n";
        
        // Phase B
        lines += "waveform,device=";
        lines += deviceId;
        lines += ",phase=B,idx=";
        lines += String(i);
        lines += " value=";
        lines += String(valB, 1);
        lines += "\n";
        
        // Phase C
        lines += "waveform,device=";
        lines += deviceId;
        lines += ",phase=C,idx=";
        lines += String(i);
        lines += " value=";
        lines += String(valC, 1);
        if (i < (int)_data.sampleCount - 1) {
            lines += "\n";
        }
    }
    
    return lines;
}
