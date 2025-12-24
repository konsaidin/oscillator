# Трёхфазный монитор напряжения на ESP32-S3

[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP32--S3-orange)](https://platformio.org/)
[![Grafana](https://img.shields.io/badge/Grafana-Dashboard-F46800)](https://grafana.com/)
[![InfluxDB](https://img.shields.io/badge/InfluxDB-Time--Series-22ADF6)](https://www.influxdata.com/)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

## Обзор проекта

Система мониторинга трёхфазной электросети (380В/220В) на базе микроконтроллера **ESP32-S3** и трёх датчиков напряжения **ZMPT101B**. Устройство измеряет параметры сети в реальном времени и отправляет данные в **InfluxDB** для визуализации в **Grafana**.

### Основные функции

| Функция | Описание |
|---------|----------|
| **📊 Измерение напряжения** | RMS напряжение каждой фазы (A, B, C) |
| **📈 Междуфазное напряжение** | Линейные напряжения AB, BC, CA |
| **🔄 Частота сети** | Измерение частоты 50 Гц методом Zero-Crossing |
| **⚖️ Перекос фаз** | Расчёт несимметрии напряжений (%) |
| **🚨 Детекция проблем** | Обрыв фазы, превышение/занижение напряжения |
| **📉 Grafana Dashboard** | Real-time графики, история, алерты |

---

## Технический стек (финальный)

### Аппаратное обеспечение

| Компонент | Модель | Назначение |
|-----------|--------|------------|
| **MCU** | ESP32-S3-DevKitC-1 | Двухъядерный процессор 240 МГц, WiFi, 12-бит ADC |
| **Датчики** | ZMPT101B × 3 | Трансформаторные датчики напряжения с гальванической развязкой |
| **Питание** | 5V DC (USB-C) | Питание контроллера и датчиков |

### Программный стек

```
┌─────────────────────────────────────────────────────────────┐
│                 1. FIRMWARE (ESP32-S3)                      │
├─────────────────────────────────────────────────────────────┤
│  Platform:  PlatformIO                                      │
│  Framework: Arduino                                         │
├──────────────────┬──────────────────────────────────────────┤
│ ArduinoJson      │ Формирование JSON/Line Protocol          │
│ WiFi             │ Подключение к сети                       │
│ HTTPClient       │ Отправка данных в InfluxDB               │
│ Preferences      │ Хранение настроек калибровки             │
└──────────────────┴──────────────────────────────────────────┘
                            │
                            ▼ HTTP POST (InfluxDB Line Protocol)
┌─────────────────────────────────────────────────────────────┐
│                 2. INFLUXDB 2.x                             │
├─────────────────────────────────────────────────────────────┤
│  • Time-series база данных                                  │
│  • Bucket: "power_monitoring"                               │
│  • Retention: 30 дней (raw), 1 год (downsampled)            │
│  • Встроенные Tasks для агрегации                           │
└──────────────────┬──────────────────────────────────────────┘
                   │
                   ▼ Flux queries
┌─────────────────────────────────────────────────────────────┐
│                 3. GRAFANA 10.x                             │
├─────────────────────────────────────────────────────────────┤
│  • Real-time дашборды                                       │
│  • Встроенная система алертов                               │
│  • Уведомления: Email, Telegram, Webhook                    │
│  • Аннотации для событий                                    │
└─────────────────────────────────────────────────────────────┘
```

### Почему этот стек?

| Компонент | Почему выбран |
|-----------|---------------|
| **InfluxDB 2.x** | Оптимизирована для time-series, встроенный HTTP API, Flux язык запросов |
| **Grafana** | Лучший инструмент визуализации, встроенные алерты, zero-code настройка |
| **HTTP POST** | Проще WebSocket для ESP32, надёжнее при потере связи |
| **Line Protocol** | Компактный формат, нативный для InfluxDB |

---

## Архитектура системы

### Полная схема

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ФИЗИЧЕСКИЙ УРОВЕНЬ                              │
└──────────────────────────────────────────────────────────────────────────────┘
       │              │              │
   Фаза A         Фаза B         Фаза C        (Сеть 380В)
       │              │              │
       ▼              ▼              ▼
┌──────────────┬──────────────┬──────────────┐
│  ZMPT101B #1 │  ZMPT101B #2 │  ZMPT101B #3 │  Гальваническая развязка
└──────┬───────┴──────┬───────┴──────┬───────┘
       │ 0-3.3V       │ 0-3.3V       │ 0-3.3V
       ▼              ▼              ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ESP32-S3 (Датчик)                                 │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  ADC1 (12-bit, 10kHz sampling)                                         │ │
│  │       ↓                                                                │ │
│  │  Signal Processing:                                                    │ │
│  │    • RMS Calculation                                                   │ │
│  │    • Zero-Crossing → Frequency                                         │ │
│  │    • Unbalance Detection                                               │ │
│  │       ↓                                                                │ │
│  │  InfluxDB Client (HTTP POST, Line Protocol)                            │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────┬─────────────────────────────┘
                                                │ WiFi
                                                │ HTTP POST каждые 1 сек
                                                ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    СЕРВЕР (VPS / Raspberry Pi / NAS / Docker)               │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │                         INFLUXDB 2.7                                   │ │
│  │   ─────────────────────────────────────────────────────────────────────│ │
│  │  Organization: home                                                    │ │
│  │  Bucket: power_monitoring                                              │ │
│  │                                                                        │ │
│  │  Measurements:                                                         │ │
│  │    • voltage (phase=A/B/C) — фазные напряжения                         │ │
│  │    • line_voltage (phases=AB/BC/CA) — междуфазные                      │ │
│  │    • frequency — частота сети                                          │ │
│  │    • unbalance — перекос фаз %                                         │ │
│  │                                                                        │ │
│  │  Retention: 30d (raw), 365d (1h aggregates)                            │ │
│  │  Port: 8086                                                            │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                        │
│                                    │ Flux Queries                           │
│                                    ▼                                        │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │                         GRAFANA 10.x                                   │ │
│  │   ─────────────────────────────────────────────────────────────────────│ │
│  │  Dashboards:                                                           │ │
│  │    • Real-time напряжения (Gauge + Time Series)                        │ │
│  │    • История за день/неделю/месяц                                      │ │
│  │    • Таблица алертов                                                   │ │
│  │                                                                        │ │
│  │  Alerting:                                                             │ │
│  │    • Phase Loss (U < 50V) → Telegram + Email                           │ │
│  │    • Over/Under Voltage (±10%) → Telegram                              │ │
│  │    • Unbalance > 4% → Email                                            │ │
│  │                                                                        │ │
│  │  Port: 3000                                                            │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────┬─────────────────────────────┘
                                                │ HTTP :3000
                                                ▼
                                    ┌──────────────────┐
                                    │     Browser      │
                                    │  Grafana UI      │
                                    └──────────────────┘
```

### Формат данных (InfluxDB Line Protocol)

ESP32 отправляет данные в формате Line Protocol:

```
voltage,device=esp32-001,phase=A value=221.5 1703419200000000000
voltage,device=esp32-001,phase=B value=219.8 1703419200000000000
voltage,device=esp32-001,phase=C value=223.1 1703419200000000000
line_voltage,device=esp32-001,phases=AB value=383.5 1703419200000000000
line_voltage,device=esp32-001,phases=BC value=380.2 1703419200000000000
line_voltage,device=esp32-001,phases=CA value=386.1 1703419200000000000
frequency,device=esp32-001 value=50.02 1703419200000000000
unbalance,device=esp32-001 value=1.23 1703419200000000000
```

### Grafana Alerting Rules

| Alert | Условие (Flux) | Severity | Уведомление |
|-------|----------------|----------|-------------|
| Phase Loss | `voltage < 50` | Critical | Telegram + Email |
| Under Voltage | `voltage < 198` | Warning | Telegram |
| Over Voltage | `voltage > 242` | Warning | Telegram |
| Unbalance | `unbalance > 4.0` | Warning | Email |
| Frequency Drift | `abs(frequency - 50) > 0.5` | Warning | Email |

### Схема подключения (Pinout)

| Компонент | Пин ESP32-S3 | Функция | Примечание |
|-----------|--------------|---------|------------|
| **ZMPT101B (Фаза A)** | GPIO 1 | ADC1_CH0 | Аттенюация 11dB |
| **ZMPT101B (Фаза B)** | GPIO 2 | ADC1_CH1 | Аттенюация 11dB |
| **ZMPT101B (Фаза C)** | GPIO 3 | ADC1_CH2 | Аттенюация 11dB |
| **OLED SDA (опц.)** | GPIO 8 | I2C SDA | SSD1306 128x64 |
| **OLED SCL (опц.)** | GPIO 9 | I2C SCL | — |
| **Status LED** | GPIO 48 | Встроенный LED | Индикация работы |

> ⚠️ **Важно**: Используйте только **ADC1** (GPIO 1-10). ADC2 конфликтует с WiFi!

---

## Математическая модель

### 1. RMS напряжение (действующее значение)

$$U_{RMS} = \sqrt{\frac{1}{N} \sum_{i=1}^{N} (u_i - u_{offset})^2} \times K_{cal}$$

| Параметр | Описание |
|----------|----------|
| $u_i$ | i-й отсчёт АЦП |
| $u_{offset}$ | Смещение нуля (≈ VCC/2 = 1.65V = ~2048 для 12-бит) |
| $N$ | Количество отсчётов (рекомендуется 200+ на период) |
| $K_{cal}$ | Калибровочный коэффициент (зависит от датчика) |

### 2. Междуфазное (линейное) напряжение

Для симметричной трёхфазной системы со сдвигом 120°:

$$U_{line} = U_{phase} \times \sqrt{3} \approx U_{phase} \times 1.732$$

Пример: $U_{phase} = 220V \Rightarrow U_{line} = 380V$

### 3. Частота сети (Zero-Crossing)

$$f = \frac{N_{crossings}}{2 \times T_{measurement}}$$

Алгоритм:
1. Детектируем переход сигнала через $u_{offset}$
2. Засекаем время между переходами
3. Усредняем за 10-20 периодов

### 4. Перекос фаз (Voltage Unbalance)

По стандарту **NEMA MG-1**:

$$Unbalance (\%) = \frac{\max(|U_A - U_{avg}|, |U_B - U_{avg}|, |U_C - U_{avg}|)}{U_{avg}} \times 100$$

| Значение | Состояние |
|----------|-----------|
| 0-2% | ✅ Норма |
| 2-4% | ⚠️ Допустимо |
| >4% | 🔴 Проблема |

---

## Протокол обмена данными

### ESP32 → InfluxDB (HTTP POST)

**Endpoint:** `POST http://<server>:8086/api/v2/write?org=home&bucket=power_monitoring`

**Headers:**
```
Authorization: Token <INFLUXDB_TOKEN>
Content-Type: text/plain
```

**Body (Line Protocol):**
```
voltage,device=esp32-001,phase=A value=221.5
voltage,device=esp32-001,phase=B value=219.8
voltage,device=esp32-001,phase=C value=223.1
line_voltage,device=esp32-001,phases=AB value=383.5
line_voltage,device=esp32-001,phases=BC value=380.2
line_voltage,device=esp32-001,phases=CA value=386.1
frequency,device=esp32-001 value=50.02
unbalance,device=esp32-001 value=1.23
```

### Flux запросы для Grafana

**Текущее напряжение фазы A:**
```flux
from(bucket: "power_monitoring")
  |> range(start: -5m)
  |> filter(fn: (r) => r._measurement == "voltage")
  |> filter(fn: (r) => r.phase == "A")
  |> last()
```

**История напряжений за 24 часа:**
```flux
from(bucket: "power_monitoring")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "voltage")
  |> aggregateWindow(every: 1m, fn: mean)
```

### Типы алертов (Grafana Alerting)

| Код | Название | Условие | Severity | For |
|-----|----------|---------|----------|-----|
| `phase_loss` | Обрыв фазы | U < 50V | Critical | 5s |
| `under_voltage` | Низкое напряжение | U < 198V (-10%) | Warning | 30s |
| `over_voltage` | Высокое напряжение | U > 242V (+10%) | Warning | 30s |
| `unbalance` | Перекос фаз | >4% | Warning | 1m |
| `frequency_drift` | Отклонение частоты | \|f - 50\| > 0.5 Hz | Warning | 1m |

---

## Структура проекта

```
oscillator/
├── README.md                   # Документация (этот файл)
├── TODO.md                     # План разработки
│
├── firmware/                   # Прошивка ESP32-S3
│   ├── platformio.ini          # Конфигурация PlatformIO
│   └── src/
│       ├── main.cpp            # Точка входа
│       ├── config.h            # WiFi, InfluxDB URL, Token, пины
│       ├── VoltageSensor.h/cpp # Класс работы с ZMPT101B
│       ├── PowerAnalyzer.h/cpp # Анализ трёхфазной сети
│       └── InfluxClient.h/cpp  # HTTP клиент для InfluxDB
│
├── docker/                     # Docker конфигурация
│   ├── docker-compose.yml      # InfluxDB + Grafana
│   ├── grafana/
│   │   ├── provisioning/
│   │   │   ├── datasources/
│   │   │   │   └── influxdb.yml    # Автонастройка источника
│   │   │   └── dashboards/
│   │   │       └── power.yml       # Автоимпорт дашборда
│   │   └── dashboards/
│   │       └── power-monitoring.json  # Готовый дашборд
│   └── influxdb/
│       └── init.sh             # Создание bucket и token
│
└── docs/
    ├── wiring.md               # Схема подключения
    └── calibration.md          # Инструкция по калибровке
```

---

## Быстрый старт

### 1. Запуск сервера (InfluxDB + Grafana)

```bash
cd docker
docker-compose up -d
```

После запуска:
- **InfluxDB UI:** http://localhost:8086 (admin / adminadmin)
- **Grafana:** http://localhost:3000 (admin / admin)

### 2. Настройка InfluxDB

1. Открыть http://localhost:8086
2. Создать Organization: `home`
3. Создать Bucket: `power_monitoring`
4. Создать API Token (Full Access) → скопировать

### 3. Прошивка ESP32

```bash
cd firmware
```

Отредактировать `src/config.h`:

```cpp
// WiFi
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

// InfluxDB
#define INFLUXDB_URL "http://192.168.1.100:8086"
#define INFLUXDB_ORG "home"
#define INFLUXDB_BUCKET "power_monitoring"
#define INFLUXDB_TOKEN "YOUR_INFLUXDB_TOKEN"

// Device ID
#define DEVICE_ID "esp32-001"
```

```bash
# Сборка и загрузка
pio run -t upload

# Мониторинг
pio device monitor
```

### 4. Настройка Grafana Dashboard

1. Открыть http://localhost:3000
2. **Data Sources → Add → InfluxDB**
   - Query Language: **Flux**
   - URL: `http://influxdb:8086`
   - Organization: `home`
   - Token: `<ваш токен>`
   - Default Bucket: `power_monitoring`
3. **Dashboards → Import** → загрузить `docker/grafana/dashboards/power-monitoring.json`

### 5. Настройка алертов в Grafana

1. **Alerting → Contact Points**
   - Telegram: добавить Bot Token и Chat ID
   - Email: настроить SMTP
2. **Alerting → Alert Rules** → создать правила (см. таблицу выше)

---

## Требования безопасности

| ⚡ ВНИМАНИЕ: Работа с высоким напряжением опасна для жизни! |
|:---:|

1. **Гальваническая развязка**: ZMPT101B обеспечивает изоляцию, но соблюдайте осторожность при монтаже высоковольтной части.

2. **Корпус**: Устройство должно быть в **диэлектрическом корпусе** класса защиты не ниже IP20.

3. **Предохранители**: Обязательно установите плавкие вставки **0.5A** на входе каждой фазы перед датчиком.

4. **Заземление**: Корпус устройства должен быть заземлён.

5. **Квалификация**: Монтаж должен выполнять **квалифицированный электрик**.

---

## Лицензия

MIT License — свободное использование с указанием авторства.

---

## Дополнительно

- [TODO.md](TODO.md) — План разработки и задачи
- [Datasheet ZMPT101B](https://www.google.com/search?q=ZMPT101B+datasheet)
- [ESP32-S3 Technical Reference](https://www.espressif.com/en/products/socs/esp32-s3)
