/*
 * ============================================================================
 *  СИСТЕМА АВТОПОЛИВА НА ESP8266 NodeMCU
 *  Версия: 1.6.1 (исправление бага таймеров)
 *  Автор: sa
 * ============================================================================
 *
 *  ЧТО ИСПРАВЛЕНО В ВЕРСИИ 1.6.1:
 *  ─────────────────────────────────────────────────────────────────────────
 *  • Исправлен критический баг в updateWateringLogic():
 *    реле включались и сразу выключались без учёта onTime.
 *  • wateringTimerStart теперь устанавливается ДО вызова setRelay()
 *  • Проверка условия перенесена на следующий такт через break
 *  • Используется свежее значение millis() при проверке условия
 *
 *  ФУНКЦИОНАЛ (без изменений с v1.6):
 *  ─────────────────────────────────────────────────────────────────────────
 *  • 6 реле с отдельными MQTT-топиками для каждого дня недели (day0-day7)
 *  • Отдельные топики для параметров (time, interval, cycle)
 *  • NTP-синхронизация, время МСК в EEPROM
 *  • Параметр cycle — количество полных циклов (0 = бесконечно)
 *  • Failsafe при потере связи
 *  • Serial CLI + MQTT команды
 *
 *  MQTT ТОПИКИ:
 *  ─────────────────────────────────────────────────────────────────────────
 *  user/watering/status      ← ESP публикует JSON-статус
 *  user/watering/log         ← ESP публикует логи
 *  user/watering/control     ← Команды управления (START, STOP, RELAY_ON...)
 *  user/watering/time        ← Время включения реле (payload: число, сек)
 *  user/watering/interval    ← Интервал между реле (payload: число, мин)
 *  user/watering/cycle       ← Кол-во циклов (payload: число, 0=∞)
 *  user/watering/day0        ← Каждый день (0/1)
 *  user/watering/day1..day7  ← Пн..Вс (0/1)
 *
 * ============================================================================
 * 
# 💧 Auto Watering System ESP8266

## 📖 Описание

**Auto Watering System** — это прошивка для микроконтроллера ESP8266 NodeMCU, реализующая систему автоматического полива с управлением через MQTT-брокер и Serial CLI. Система поддерживает расписание по дням недели, настраиваемые параметры полива, синхронизацию времени по NTP и сохранение всех настроек в энергонезависимой памяти.

Проект разработан для управления **6 независимыми зонами полива** (например, грядками, газонами, теплицами) с возможностью гибкой настройки времени, интервалов и расписания.

---

## ✨ Возможности

### 🎛️ Управление
- ✅ Управление **6 реле** (зоны полива)
- ✅ Ручное включение/выключение каждого реле
- ✅ Автоматический цикл: поочерёдное включение реле 1→2→3→4→5→6
- ✅ Настраиваемое время включения реле (секунды)
- ✅ Настраиваемый интервал между реле (минуты)
- ✅ Настраиваемое количество циклов (0 = бесконечно)

### 📅 Расписание
- ✅ Расписание полива по дням недели
- ✅ Отдельные топики для каждого дня (`day0`...`day7`)
- ✅ Режим "каждый день" (главный переключатель)
- ✅ Автоматическая проверка дня недели перед запуском

### 🌐 Сеть и MQTT
- ✅ Подключение к Wi-Fi (2.4 GHz)
- ✅ MQTT-клиент с автоматическим переподключением
- ✅ Отдельные топики для параметров (числовой payload)
- ✅ LWT (Last Will and Testament) для отслеживания статуса
- ✅ Совместимость с публичным брокером `wqtt.ru`

### 🕐 Время
- ✅ NTP-синхронизация времени
- ✅ Часовой пояс МСК (UTC+3)
- ✅ Сохранение времени в EEPROM (раз в 10 минут)
- ✅ Fallback на uptime при отсутствии синхронизации

### 💾 Энергонезависимость
- ✅ Все настройки сохраняются в EEPROM
- ✅ Восстановление состояния реле после перезагрузки
- ✅ Magic byte для валидации конфигурации
- ✅ Миграция со старых версий прошивки

### 🛡️ Безопасность
- ✅ **Failsafe**: при потере связи все реле немедленно выключаются
- ✅ Автоматическая остановка полива при обрыве связи
- ✅ Непрерывный мониторинг WiFi и MQTT

### 🖥️ Интерфейсы
- ✅ Serial CLI (115200 baud) для настройки и отладки
- ✅ MQTT-команды для удалённого управления
- ✅ JSON-статус для интеграции с умным домом
- ✅ Системные логи в MQTT-топике

---

## 🔧 Аппаратные требования

### Необходимое оборудование

| Компонент | Количество | Примечание |
|---|---|---|
| ESP8266 NodeMCU | 1 | ESP-12E/F |
| Модуль реле 5В | 6 | С оптоизоляцией, активный LOW |
| Блок питания 5В 2А | 1 | Для ESP и реле |
| Провода | — | Подключение реле |

### Распиновка (критично!)

| Реле | Пин NodeMCU | GPIO | Назначение |
|---|---|---|---|
| Реле 1 | **D1** | GPIO5 | Зона полива 1 |
| Реле 2 | **D2** | GPIO4 | Зона полива 2 |
| Реле 3 | **D5** | GPIO14 | Зона полива 3 |
| Реле 4 | **D6** | GPIO12 | Зона полива 4 |
| Реле 5 | **D7** | GPIO13 | Зона полива 5 |
| Реле 6 | **D8** | GPIO15 | Зона полива 6 |

> ⚠️ **ЗАПРЕЩЕНО использовать пины**: D3 (GPIO0), D4 (GPIO2), D9 (GPIO1/TX), D10 (GPIO3/RX) — они конфликтуют с загрузкой и Serial.

> ⚠️ **D8 (GPIO15)** при загрузке должен быть LOW. Убедитесь, что модуль реле корректно работает на этом пине.

### Схема подключения
ESP8266 NodeMCU Модуль реле 5В
┌─────────────┐ ┌─────────────┐
│ D1 ├──────────┤ IN1 │
│ D2 ├──────────┤ IN2 │
│ D5 ├──────────┤ IN3 │
│ D6 ├──────────┤ IN4 │
│ D7 ├──────────┤ IN5 │
│ D8 ├──────────┤ IN6 │
│ 3V3 ├──────────┤ VCC (опто.) │
│ GND ├──────────┤ GND │
└─────────────┘ └─────────────┘
│
[5В БП]


---

## 📡 Структура MQTT-топиков

Все топики используют префикс логина `user` (настраивается в `MQTT_LOGIN`).

### Топики ESP → Клиент (публикация)

| Топик | Частота | Содержимое |
|---|---|---|
| `user/watering/status` | 30 сек | JSON-статус системы |
| `user/watering/log` | По событию | Системные логи |

### Топики Клиент → ESP (команды)

| Топик | Payload | Описание |
|---|---|---|
| `user/watering/control` | `START` | Запустить полив |
| `user/watering/control` | `STOP` | Остановить полив |
| `user/watering/control` | `RELAY_ON N` | Включить реле N (1-6) |
| `user/watering/control` | `RELAY_OFF N` | Выключить реле N |
| `user/watering/control` | `STATUS` | Запрос статуса |
| `user/watering/time` | `30` | Время полива (сек) |
| `user/watering/interval` | `5` | Интервал (мин) |
| `user/watering/cycle` | `10` | Кол-во циклов (0 = ∞) |
| `user/watering/day0` | `0` / `1` | Каждый день |
| `user/watering/day1` | `0` / `1` | Понедельник |
| `user/watering/day2` | `0` / `1` | Вторник |
| `user/watering/day3` | `0` / `1` | Среда |
| `user/watering/day4` | `0` / `1` | Четверг |
| `user/watering/day5` | `0` / `1` | Пятница |
| `user/watering/day6` | `0` / `1` | Суббота |
| `user/watering/day7` | `0` / `1` | Воскресенье |

### Пример JSON-статуса

```json
{
  "moscow_time": "04.07.2026 19:30:15 МСК",
  "ntp_synced": true,
  "uptime": "0d 00:25:10",
  "wifi": true,
  "ip": "192.168.1.105",
  "mqtt": true,
  "watering": true,
  "state": 1,
  "current_relay": 3,
  "cycle_count": 5,
  "cycles_completed": 2,
  "interval_min": 5,
  "ontime_sec": 30,
  "watering_days": "Пн, Ср, Пт",
  "days": {
    "day0": 0, "day1": 1, "day2": 0,
    "day3": 1, "day4": 0, "day5": 1,
    "day6": 0, "day7": 0
  },
  "relays": [0, 0, 1, 0, 0, 0],
  "ssid": "MyHomeWiFi",
  "mqtt_host": "m5.wqtt.ru:13594",
  "ntp_server": "pool.ntp.org"
}

===========================================
 HELP
 ===========================================
 > STATUS
===========================================
 СТАТУС СИСТЕМЫ
===========================================
Время МСК         : 04.07.2026 19:30:15 МСК
Uptime            : 0d 00:25:10
NTP синхрониз.    : ДА
--- Сеть ---
Wi-Fi подключен   : ДА
IP-адрес          : 192.168.1.105
SSID              : WiFi
MQTT подключен    : ДА
MQTT брокер       : m5.wqtt.ru:13594
NTP-сервер        : pool.ntp.org
--- Параметры полива ---
Время вкл. реле   : 30 сек
Интервал          : 5 мин
Кол-во циклов     : ∞ (бесконечно)
Дни полива        : Пн, Ср, Пт
  Детали по дням:
    day0 (Каждый день)      : ВЫКЛ
    day1 (Пн)               : ВКЛ
    day2 (Вт)               : ВЫКЛ
    day3 (Ср)               : ВКЛ
    day4 (Чт)               : ВЫКЛ
    day5 (Пт)               : ВКЛ
    day6 (Сб)               : ВЫКЛ
    day7 (Вс)               : ВЫКЛ
--- Состояние полива ---
Полив активен     : НЕТ
Состояние FSM     : IDLE (ожидание)
Текущее реле      : 1
Выполнено циклов  : 0
--- Состояние реле ---
  Реле 1 (D1)         : ВЫКЛ
  Реле 2 (D2)         : ВЫКЛ
  Реле 3 (D5)         : ВЫКЛ
  Реле 4 (D6)         : ВЫКЛ
  Реле 5 (D7)         : ВЫКЛ
  Реле 6 (D8)         : ВЫКЛ
===========================================
 */

// ============================================================================
// INCLUDES
// ============================================================================
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <time.h>

// ============================================================================
// КОНСТАНТЫ И КОНФИГУРАЦИЯ
// ============================================================================
#define MAGIC_BYTE            0xAB
#define EEPROM_SIZE           512

#define NUM_RELAYS            6
const uint8_t RELAY_PINS[NUM_RELAYS] = { D1, D2, D5, D6, D7, D8 };

#define RELAY_ON_STATE        LOW
#define RELAY_OFF_STATE       HIGH

// --- MQTT: логин на wqtt.ru ---
#define MQTT_LOGIN            "user"

// --- MQTT топики ---
#define MQTT_TOPIC_STATUS     MQTT_LOGIN "/watering/status"
#define MQTT_TOPIC_LOG        MQTT_LOGIN "/watering/log"
#define MQTT_TOPIC_CONTROL    MQTT_LOGIN "/watering/control"
#define MQTT_TOPIC_TIME       MQTT_LOGIN "/watering/time"
#define MQTT_TOPIC_INTERVAL   MQTT_LOGIN "/watering/interval"
#define MQTT_TOPIC_CYCLE      MQTT_LOGIN "/watering/cycle"

// --- Топики дней недели ---
#define MQTT_TOPIC_DAY0       MQTT_LOGIN "/watering/day0"
#define MQTT_TOPIC_DAY1       MQTT_LOGIN "/watering/day1"
#define MQTT_TOPIC_DAY2       MQTT_LOGIN "/watering/day2"
#define MQTT_TOPIC_DAY3       MQTT_LOGIN "/watering/day3"
#define MQTT_TOPIC_DAY4       MQTT_LOGIN "/watering/day4"
#define MQTT_TOPIC_DAY5       MQTT_LOGIN "/watering/day5"
#define MQTT_TOPIC_DAY6       MQTT_LOGIN "/watering/day6"
#define MQTT_TOPIC_DAY7       MQTT_LOGIN "/watering/day7"

#define DEFAULT_NTP_SERVER    "pool.ntp.org"
#define MSK_OFFSET_SEC        10800
#define MSK_DST_SEC           0
#define NTP_MIN_VALID_TIME    1000000000UL

#define DEFAULT_WIFI_SSID     "YourSSID"
#define DEFAULT_WIFI_PASS     "YourPassword"
#define DEFAULT_MQTT_HOST     "m5.wqtt.ru"
#define DEFAULT_MQTT_PORT     13594
#define DEFAULT_MQTT_USER     "user"
#define DEFAULT_MQTT_PASS     "12345678"

#define DEFAULT_INTERVAL_MIN  5
#define DEFAULT_ONTIME_SEC    30
#define DEFAULT_CYCLE_COUNT   0
#define DEFAULT_WATERING_DAYS 0x01    // 0x01 = "каждый день"

#define WIFI_RETRY_INTERVAL   10000
#define MQTT_RETRY_INTERVAL   5000
#define STATUS_PUBLISH_INTERVAL 30000
#define NTP_CHECK_INTERVAL    5000
#define TIME_SAVE_INTERVAL    600000

#define MAX_SSID_LEN          32
#define MAX_PASS_LEN          64
#define MAX_HOST_LEN          64
#define MAX_USER_LEN          32
#define MAX_NTP_LEN           32

// ============================================================================
// СТРУКТУРА КОНФИГУРАЦИИ (EEPROM)
// ============================================================================
struct Config {
  uint8_t  magic;
  char     wifiSSID[MAX_SSID_LEN + 1];
  char     wifiPass[MAX_PASS_LEN + 1];
  char     mqttHost[MAX_HOST_LEN + 1];
  uint16_t mqttPort;
  char     mqttUser[MAX_USER_LEN + 1];
  char     mqttPass[MAX_PASS_LEN + 1];
  uint8_t  relayStates[NUM_RELAYS];
  uint32_t intervalMin;
  uint32_t onTimeSec;
  uint32_t cycleCount;
  uint32_t lastKnownTime;
  char     ntpServer[MAX_NTP_LEN + 1];
  uint8_t  wateringDays;
};

Config config;

// ============================================================================
// СОСТОЯНИЕ КОНЕЧНОГО АВТОМАТА ПОЛИВА
// ============================================================================
enum WateringState { WS_IDLE, WS_RELAY_ON, WS_WAIT_INTERVAL };

WateringState wateringState = WS_IDLE;
uint8_t       currentRelayIndex = 0;
unsigned long wateringTimerStart = 0;
bool          wateringActive = false;
uint32_t      cyclesCompleted = 0;

// ============================================================================
// NTP ПЕРЕМЕННЫЕ
// ============================================================================
bool          ntpSynced = false;
unsigned long lastNtpCheck = 0;
unsigned long lastTimeSaveMs = 0;
uint32_t      lastSavedTimeValue = 0;

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================
String serialBuffer = "";
unsigned long lastWifiAttempt    = 0;
unsigned long lastMqttAttempt    = 0;
unsigned long lastStatusPublish  = 0;
String mqttClientId = "";
bool prevWifiConnected = false;
bool prevMqttConnected = false;

WiFiClient   espClient;
PubSubClient mqttClient(espClient);

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ
// ============================================================================
void setupHardware();
void setupNetwork();
void setupNTP();
void handleNTP();
void saveTimeToEEPROM();
bool isTimeSynced();
String getMoscowTimeString();
void loadConfig();
void saveConfig();
void initDefaultConfig();
void setAllRelaysOff();
void setRelay(uint8_t index, bool on);
void logEvent(const String& message);
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void handleSerial();
void handleNetwork();
void updateWateringLogic();
void publishStatus();
void printStatusSerial();
String buildStatusString();
void executeCommand(const String& rawCmd);
void handleParamCommand(const String& topic, const String& value);
void handleDayCommand(const String& topic, const String& value);
String getUptimeString();
uint8_t getPinNumber(uint8_t gpioPin);
bool isWateringDay();
String getWateringDaysString();
bool isDayEnabled(uint8_t dayIndex);
const char* getDayTopicByIndex(uint8_t dayIndex);
const char* getDayName(uint8_t dayIndex);

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("==========================================="));
  Serial.println(F(" СИСТЕМА АВТОПОЛИВА ESP8266 v1.6.1"));
  Serial.println(F(" Автор: sa"));
  Serial.println(F("==========================================="));

  EEPROM.begin(EEPROM_SIZE);
  loadConfig();

  setupHardware();
  Serial.println(F("[BOOT] Аппаратная часть инициализирована. Все реле ВЫКЛ."));

  mqttClientId = "ESP8266_Watering_" + String(ESP.getChipId(), HEX);

  mqttClient.setServer(config.mqttHost, config.mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setBufferSize(512);
  mqttClient.setKeepAlive(60);

  setupNetwork();

  Serial.println(F("[BOOT] Загрузка завершена. Ожидание подключения..."));
  Serial.println(F("[BOOT] Введите HELP для списка команд Serial CLI."));
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
  handleSerial();
  handleNetwork();

  if (WiFi.status() == WL_CONNECTED) {
    handleNTP();
  }

  bool wifiOK = (WiFi.status() == WL_CONNECTED);
  bool mqttOK = mqttClient.connected();

  if (wateringActive && (!wifiOK || !mqttOK)) {
    wateringActive = false;
    wateringState  = WS_IDLE;
    currentRelayIndex = 0;
    cyclesCompleted = 0;
    setAllRelaysOff();
    logEvent(F("!!! FAILSAFE: Потеря связи! Все реле ВЫКЛ. Полив остановлен."));
  }

  updateWateringLogic();

  if (mqttOK && (millis() - lastStatusPublish >= STATUS_PUBLISH_INTERVAL)) {
    lastStatusPublish = millis();
    publishStatus();
  }

  saveTimeToEEPROM();
}

// ============================================================================
// NTP
// ============================================================================
void setupNTP() {
  configTime(MSK_OFFSET_SEC, MSK_DST_SEC, config.ntpServer, "ru.pool.ntp.org", "pool.ntp.org");
  logEvent("NTP: запуск синхронизации с \"" + String(config.ntpServer) + "\"");
}

void handleNTP() {
  if (millis() - lastNtpCheck < NTP_CHECK_INTERVAL) return;
  lastNtpCheck = millis();

  if (!ntpSynced && isTimeSynced()) {
    ntpSynced = true;
    logEvent("NTP: время синхронизировано! МСК: " + getMoscowTimeString());
  }
}

void saveTimeToEEPROM() {
  if (!ntpSynced) return;
  if (millis() - lastTimeSaveMs < TIME_SAVE_INTERVAL) return;

  time_t now = time(nullptr);
  if (now < NTP_MIN_VALID_TIME) return;

  uint32_t now32 = (uint32_t)now;

  if (now32 != lastSavedTimeValue) {
    config.lastKnownTime = now32;
    lastSavedTimeValue = now32;
    EEPROM.put(0, config);
    EEPROM.commit();
    lastTimeSaveMs = millis();

    Serial.print(F("[EEPROM] Время МСК сохранено: "));
    Serial.println(getMoscowTimeString());
  }
}

bool isTimeSynced() {
  time_t now = time(nullptr);
  return (now > NTP_MIN_VALID_TIME);
}

String getMoscowTimeString() {
  time_t now = time(nullptr);

  if (now < NTP_MIN_VALID_TIME) {
    return getUptimeString();
  }

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char buf[30];
  snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d:%02d МСК",
           timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(buf);
}

// ============================================================================
// РАСПИСАНИЕ: ДНИ НЕДЕЛИ
// ============================================================================
const char* getDayName(uint8_t dayIndex) {
  static const char* names[] = {
    "Каждый день", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб", "Вс"
  };
  if (dayIndex > 7) return "?";
  return names[dayIndex];
}

const char* getDayTopicByIndex(uint8_t dayIndex) {
  static const char* topics[] = {
    MQTT_TOPIC_DAY0, MQTT_TOPIC_DAY1, MQTT_TOPIC_DAY2, MQTT_TOPIC_DAY3,
    MQTT_TOPIC_DAY4, MQTT_TOPIC_DAY5, MQTT_TOPIC_DAY6, MQTT_TOPIC_DAY7
  };
  if (dayIndex > 7) return "";
  return topics[dayIndex];
}

bool isDayEnabled(uint8_t dayIndex) {
  if (dayIndex > 7) return false;
  return (config.wateringDays & (1 << dayIndex)) != 0;
}

bool isWateringDay() {
  if (config.wateringDays & 0x01) return true;
  if (!ntpSynced) return true;

  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  uint8_t wday = timeinfo.tm_wday;
  uint8_t bitIndex = (wday == 0) ? 7 : wday;

  return (config.wateringDays & (1 << bitIndex)) != 0;
}

String getWateringDaysString() {
  if (config.wateringDays & 0x01) {
    return "Каждый день";
  }

  String result = "";
  for (uint8_t i = 1; i <= 7; i++) {
    if (config.wateringDays & (1 << i)) {
      if (result.length() > 0) result += ", ";
      result += getDayName(i);
    }
  }

  if (result.length() == 0) {
    return "Ни один день не выбран";
  }

  return result;
}

void handleDayCommand(const String& topic, const String& value) {
  int8_t dayIndex = -1;
  for (uint8_t i = 0; i <= 7; i++) {
    if (topic == getDayTopicByIndex(i)) {
      dayIndex = i;
      break;
    }
  }

  if (dayIndex < 0) {
    logEvent("ERROR: Неизвестный топик дня: " + topic);
    return;
  }

  int val = value.toInt();
  if (val != 0 && val != 1) {
    logEvent("ERROR: В топике " + topic + " допустимы только значения 0 или 1");
    return;
  }

  if (val == 1) {
    config.wateringDays |= (1 << dayIndex);
  } else {
    config.wateringDays &= ~(1 << dayIndex);
  }

  String state = val ? "ВКЛ" : "ВЫКЛ";
  logEvent("CONFIG: " + String(getDayName(dayIndex)) + " (" + topic + ") = " + state);

  if (dayIndex == 0 && val == 1) {
    logEvent(F("NOTE: Режим 'каждый день' активен. Остальные дни игнорируются."));
  }
}

// ============================================================================
// АППАРАТНАЯ ЧАСТЬ
// ============================================================================
void setupHardware() {
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    digitalWrite(RELAY_PINS[i], RELAY_OFF_STATE);
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], RELAY_OFF_STATE);
  }

  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    if (config.relayStates[i] == 1) {
      setRelay(i, true);
    }
  }
}

void setRelay(uint8_t index, bool on) {
  if (index >= NUM_RELAYS) return;

  digitalWrite(RELAY_PINS[index], on ? RELAY_ON_STATE : RELAY_OFF_STATE);
  config.relayStates[index] = on ? 1 : 0;

  String stateStr = on ? "ON" : "OFF";
  logEvent("RELAY " + String(index + 1) + " → " + stateStr +
           " (pin D" + String(getPinNumber(RELAY_PINS[index])) + ")");
}

uint8_t getPinNumber(uint8_t gpioPin) {
  if (gpioPin == 5)  return 1;
  if (gpioPin == 4)  return 2;
  if (gpioPin == 14) return 5;
  if (gpioPin == 12) return 6;
  if (gpioPin == 13) return 7;
  if (gpioPin == 15) return 8;
  return 0;
}

void setAllRelaysOff() {
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    digitalWrite(RELAY_PINS[i], RELAY_OFF_STATE);
    config.relayStates[i] = 0;
  }
  logEvent(F("ALL RELAYS → OFF"));
}

// ============================================================================
// EEPROM
// ============================================================================
void initDefaultConfig() {
  config.magic = MAGIC_BYTE;

  strncpy(config.wifiSSID, DEFAULT_WIFI_SSID, MAX_SSID_LEN);
  config.wifiSSID[MAX_SSID_LEN] = '\0';

  strncpy(config.wifiPass, DEFAULT_WIFI_PASS, MAX_PASS_LEN);
  config.wifiPass[MAX_PASS_LEN] = '\0';

  strncpy(config.mqttHost, DEFAULT_MQTT_HOST, MAX_HOST_LEN);
  config.mqttHost[MAX_HOST_LEN] = '\0';

  config.mqttPort = DEFAULT_MQTT_PORT;

  strncpy(config.mqttUser, DEFAULT_MQTT_USER, MAX_USER_LEN);
  config.mqttUser[MAX_USER_LEN] = '\0';

  strncpy(config.mqttPass, DEFAULT_MQTT_PASS, MAX_PASS_LEN);
  config.mqttPass[MAX_PASS_LEN] = '\0';

  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    config.relayStates[i] = 0;
  }

  config.intervalMin = DEFAULT_INTERVAL_MIN;
  config.onTimeSec   = DEFAULT_ONTIME_SEC;
  config.cycleCount  = DEFAULT_CYCLE_COUNT;
  config.lastKnownTime = 0;

  strncpy(config.ntpServer, DEFAULT_NTP_SERVER, MAX_NTP_LEN);
  config.ntpServer[MAX_NTP_LEN] = '\0';

  config.wateringDays = DEFAULT_WATERING_DAYS;
}

void loadConfig() {
  EEPROM.get(0, config);

  if (config.magic != MAGIC_BYTE) {
    Serial.println(F("[EEPROM] Magic byte не найден. Инициализация дефолтных настроек."));
    initDefaultConfig();
    saveConfig();
    Serial.println(F("[EEPROM] Дефолтные настройки записаны."));
  } else {
    Serial.println(F("[EEPROM] Конфигурация загружена успешно."));

    if (config.onTimeSec == 0)   config.onTimeSec = DEFAULT_ONTIME_SEC;

    if (config.cycleCount > 1000000UL) {
      config.cycleCount = DEFAULT_CYCLE_COUNT;
      Serial.println(F("[EEPROM] cycleCount сброшен (миграция со старой версии)."));
    }

    if (strlen(config.ntpServer) == 0) {
      strncpy(config.ntpServer, DEFAULT_NTP_SERVER, MAX_NTP_LEN);
      config.ntpServer[MAX_NTP_LEN] = '\0';
      Serial.println(F("[EEPROM] NTP-сервер не задан. Установлен pool.ntp.org"));
    }

    if (config.lastKnownTime > NTP_MIN_VALID_TIME) {
      time_t savedTime = (time_t)config.lastKnownTime;
      struct tm timeinfo;
      localtime_r(&savedTime, &timeinfo);
      char buf[30];
      snprintf(buf, sizeof(buf), "%02d.%02d.%04d %02d:%02d:%02d",
               timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
               timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      Serial.print(F("[EEPROM] Последнее сохранённое время МСК: "));
      Serial.println(buf);
      lastSavedTimeValue = config.lastKnownTime;
    }

    if (config.cycleCount == 0) {
      Serial.println(F("[EEPROM] Cycle: ∞ (бесконечный)"));
    } else {
      Serial.print(F("[EEPROM] Cycle: "));
      Serial.print(config.cycleCount);
      Serial.println(F(" полных циклов"));
    }

    Serial.print(F("[EEPROM] Дни полива: "));
    Serial.println(getWateringDaysString());

    for (uint8_t i = 0; i <= 7; i++) {
      Serial.print(F("  day"));
      Serial.print(i);
      Serial.print(F(" ("));
      Serial.print(getDayName(i));
      Serial.print(F("): "));
      Serial.println(isDayEnabled(i) ? F("ВКЛ") : F("ВЫКЛ"));
    }
  }
}

void saveConfig() {
  config.magic = MAGIC_BYTE;
  EEPROM.put(0, config);
  EEPROM.commit();
  Serial.println(F("[EEPROM] Конфигурация сохранена."));
}

// ============================================================================
// СЕТЬ
// ============================================================================
void setupNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(config.wifiSSID, config.wifiPass);
  lastWifiAttempt = millis();
  logEvent("WiFi: подключение к \"" + String(config.wifiSSID) + "\"...");
}

void handleNetwork() {
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);

  if (!wifiConnected) {
    if (millis() - lastWifiAttempt >= WIFI_RETRY_INTERVAL) {
      lastWifiAttempt = millis();
      WiFi.begin(config.wifiSSID, config.wifiPass);
      Serial.println(F("[WiFi] Попытка переподключения..."));
    }
    prevWifiConnected = false;
    prevMqttConnected = false;
    if (ntpSynced) {
      ntpSynced = false;
      logEvent(F("NTP: сброшен (WiFi потерян)"));
    }
    return;
  }

  if (!prevWifiConnected && wifiConnected) {
    prevWifiConnected = true;
    logEvent("WiFi: подключен! IP: " + WiFi.localIP().toString());
    lastMqttAttempt = 0;
    setupNTP();
  }

  if (!mqttClient.connected()) {
    if (millis() - lastMqttAttempt >= MQTT_RETRY_INTERVAL) {
      lastMqttAttempt = millis();
      connectMQTT();
    }
    prevMqttConnected = false;
  } else {
    mqttClient.loop();

    if (!prevMqttConnected) {
      prevMqttConnected = true;
      logEvent(F("Связь восстановлена. Ожидание команд."));
    }
  }
}

// ============================================================================
// MQTT: ПОДКЛЮЧЕНИЕ
// ============================================================================
void connectMQTT() {
  Serial.print(F("[MQTT] Подключение к "));
  Serial.print(config.mqttHost);
  Serial.print(F(":"));
  Serial.print(config.mqttPort);
  Serial.print(F(" ... "));

  bool connected = mqttClient.connect(
    mqttClientId.c_str(),
    config.mqttUser,
    config.mqttPass,
    MQTT_TOPIC_STATUS,
    0,
    true,
    "OFFLINE"
  );

  if (connected) {
    Serial.println(F("OK"));

    mqttClient.subscribe(MQTT_TOPIC_CONTROL);
    mqttClient.subscribe(MQTT_TOPIC_TIME);
    mqttClient.subscribe(MQTT_TOPIC_INTERVAL);
    mqttClient.subscribe(MQTT_TOPIC_CYCLE);

    mqttClient.subscribe(MQTT_TOPIC_DAY0);
    mqttClient.subscribe(MQTT_TOPIC_DAY1);
    mqttClient.subscribe(MQTT_TOPIC_DAY2);
    mqttClient.subscribe(MQTT_TOPIC_DAY3);
    mqttClient.subscribe(MQTT_TOPIC_DAY4);
    mqttClient.subscribe(MQTT_TOPIC_DAY5);
    mqttClient.subscribe(MQTT_TOPIC_DAY6);
    mqttClient.subscribe(MQTT_TOPIC_DAY7);

    mqttClient.publish(MQTT_TOPIC_STATUS, "ONLINE", true);
    logEvent("MQTT: подключен. ClientID: " + mqttClientId);
    logEvent("MQTT: подписка на control, time, interval, cycle, day0-day7");
  } else {
    Serial.print(F("FAIL (rc="));
    Serial.print(mqttClient.state());
    Serial.println(F(")"));
  }
}

// ============================================================================
// MQTT: CALLBACK
// ============================================================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  String topicStr = String(topic);

  logEvent("MQTT RX [" + topicStr + "]: " + message);

  if (topicStr == MQTT_TOPIC_DAY0 ||
      topicStr == MQTT_TOPIC_DAY1 ||
      topicStr == MQTT_TOPIC_DAY2 ||
      topicStr == MQTT_TOPIC_DAY3 ||
      topicStr == MQTT_TOPIC_DAY4 ||
      topicStr == MQTT_TOPIC_DAY5 ||
      topicStr == MQTT_TOPIC_DAY6 ||
      topicStr == MQTT_TOPIC_DAY7) {
    handleDayCommand(topicStr, message);
    return;
  }

  if (topicStr == MQTT_TOPIC_TIME ||
      topicStr == MQTT_TOPIC_INTERVAL ||
      topicStr == MQTT_TOPIC_CYCLE) {
    handleParamCommand(topicStr, message);
    return;
  }

  if (topicStr == MQTT_TOPIC_CONTROL) {
    executeCommand(message);
    return;
  }
}

// ============================================================================
// ОБРАБОТКА ПАРАМЕТРИЧЕСКИХ КОМАНД (число)
// ============================================================================
void handleParamCommand(const String& topic, const String& value) {
  int32_t val = value.toInt();

  if (value.length() == 0 || (val == 0 && value != "0")) {
    logEvent("ERROR: Неверное значение в топике " + topic + ": \"" + value + "\"");
    return;
  }

  if (topic == MQTT_TOPIC_TIME) {
    if (val > 0) {
      config.onTimeSec = (uint32_t)val;
      logEvent("CONFIG: OnTime = " + String(val) + " sec");
    } else {
      logEvent(F("ERROR: OnTime должен быть > 0"));
    }
    return;
  }

  if (topic == MQTT_TOPIC_INTERVAL) {
    if (val > 0) {
      config.intervalMin = (uint32_t)val;
      logEvent("CONFIG: Interval = " + String(val) + " min");
    } else {
      logEvent(F("ERROR: Interval должен быть > 0"));
    }
    return;
  }

  if (topic == MQTT_TOPIC_CYCLE) {
    if (val >= 0) {
      config.cycleCount = (uint32_t)val;
      String desc = (val == 0) ? "∞ (бесконечно)" : String(val) + " циклов";
      logEvent("CONFIG: Cycle = " + desc);

      if (wateringActive) {
        logEvent(F("NOTE: Новое значение cycle применится после завершения текущего цикла."));
      }
    } else {
      logEvent(F("ERROR: Cycle должен быть >= 0"));
    }
    return;
  }
}

// ============================================================================
// СЕРИЙНЫЙ CLI
// ============================================================================
void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        serialBuffer.trim();
        Serial.println("> " + serialBuffer);
        executeCommand(serialBuffer);
        serialBuffer = "";
      }
    } else {
      serialBuffer += c;
      if (serialBuffer.length() > 256) {
        serialBuffer = "";
      }
    }
  }
}

// ============================================================================
// УНИВЕРСАЛЬНЫЙ ОБРАБОТЧИК КОМАНД
// ============================================================================
void executeCommand(const String& rawCmd) {
  String cmd = rawCmd;
  cmd.trim();
  if (cmd.length() == 0) return;

  String upperCmd = cmd;
  upperCmd.toUpperCase();

  int spaceIdx = upperCmd.indexOf(' ');
  String command = (spaceIdx > 0) ? upperCmd.substring(0, spaceIdx) : upperCmd;
  String arg     = (spaceIdx > 0) ? cmd.substring(spaceIdx + 1) : "";
  arg.trim();

  // ------------------------------------------------------------------
  // HELP
  // ------------------------------------------------------------------
  if (command == "HELP") {
    Serial.println(F("==========================================="));
    Serial.println(F(" ДОСТУПНЫЕ КОМАНДЫ:"));
    Serial.println(F("==========================================="));
    Serial.println(F("--- Сеть (требуют SAVE) ---"));
    Serial.println(F("  WIFI_SSID <name>       — Wi-Fi SSID"));
    Serial.println(F("  WIFI_PASS <pass>       — Wi-Fi пароль"));
    Serial.println(F("  MQTT_HOST <url>        — MQTT брокер"));
    Serial.println(F("  MQTT_PORT <port>       — MQTT порт"));
    Serial.println(F("  MQTT_USER <user>       — MQTT пользователь"));
    Serial.println(F("  MQTT_PASS <pass>       — MQTT пароль"));
    Serial.println(F("  NTP_SERVER <url>       — NTP-сервер"));
    Serial.println(F("  SAVE                   — сохранить и перезагрузить"));
    Serial.println(F("--- Полив ---"));
    Serial.println(F("  START                  — запустить полив"));
    Serial.println(F("  STOP                   — остановить полив"));
    Serial.println(F("  RELAY_ON <1-6>         — включить реле"));
    Serial.println(F("  RELAY_OFF <1-6>        — выключить реле"));
    Serial.println(F("  SET_INTERVAL <min>     — интервал между реле (мин)"));
    Serial.println(F("  SET_ONTIME <sec>       — время вкл. реле (сек)"));
    Serial.println(F("  SET_CYCLE <N>          — кол-во циклов (0 = бесконечно)"));
    Serial.println(F("--- Дни полива (0=выкл, 1=вкл) ---"));
    Serial.println(F("  SET_DAY0 <0|1>         — каждый день (игнорирует др.)"));
    Serial.println(F("  SET_DAY1 <0|1>         — понедельник"));
    Serial.println(F("  SET_DAY2 <0|1>         — вторник"));
    Serial.println(F("  SET_DAY3 <0|1>         — среда"));
    Serial.println(F("  SET_DAY4 <0|1>         — четверг"));
    Serial.println(F("  SET_DAY5 <0|1>         — пятница"));
    Serial.println(F("  SET_DAY6 <0|1>         — суббота"));
    Serial.println(F("  SET_DAY7 <0|1>         — воскресенье"));
    Serial.println(F("--- Диагностика ---"));
    Serial.println(F("  STATUS                 — показать статус"));
    Serial.println(F("  TIME                   — текущее время МСК"));
    Serial.println(F("  HELP                   — эта справка"));
    Serial.println(F("==========================================="));
    Serial.println(F(""));
    Serial.println(F("--- MQTT ТОПИКИ (payload) ---"));
    Serial.println(F("  user/watering/time      — число (сек)"));
    Serial.println(F("  user/watering/interval  — число (мин)"));
    Serial.println(F("  user/watering/cycle     — число (0=∞)"));
    Serial.println(F("  user/watering/day0..day7 — 0/1 (дни недели)"));
    Serial.println(F("--- MQTT КОМАНДЫ (топик user/watering/control) ---"));
    Serial.println(F("  START / STOP / RELAY_ON N / RELAY_OFF N / STATUS"));
    Serial.println(F("==========================================="));
    return;
  }

  // ------------------------------------------------------------------
  // НАСТРОЙКИ СЕТИ
  // ------------------------------------------------------------------
  if (command == "WIFI_SSID") {
    strncpy(config.wifiSSID, arg.c_str(), MAX_SSID_LEN);
    config.wifiSSID[MAX_SSID_LEN] = '\0';
    logEvent("Config: WIFI_SSID = \"" + arg + "\"");
    return;
  }
  if (command == "WIFI_PASS") {
    strncpy(config.wifiPass, arg.c_str(), MAX_PASS_LEN);
    config.wifiPass[MAX_PASS_LEN] = '\0';
    logEvent(F("Config: WIFI_PASS = ***"));
    return;
  }
  if (command == "MQTT_HOST") {
    strncpy(config.mqttHost, arg.c_str(), MAX_HOST_LEN);
    config.mqttHost[MAX_HOST_LEN] = '\0';
    logEvent("Config: MQTT_HOST = \"" + arg + "\"");
    return;
  }
  if (command == "MQTT_PORT") {
    uint16_t port = arg.toInt();
    if (port > 0) {
      config.mqttPort = port;
      logEvent("Config: MQTT_PORT = " + String(port));
    } else {
      logEvent(F("ERROR: Неверный порт."));
    }
    return;
  }
  if (command == "MQTT_USER") {
    strncpy(config.mqttUser, arg.c_str(), MAX_USER_LEN);
    config.mqttUser[MAX_USER_LEN] = '\0';
    logEvent("Config: MQTT_USER = \"" + arg + "\"");
    return;
  }
  if (command == "MQTT_PASS") {
    strncpy(config.mqttPass, arg.c_str(), MAX_PASS_LEN);
    config.mqttPass[MAX_PASS_LEN] = '\0';
    logEvent(F("Config: MQTT_PASS = ***"));
    return;
  }
  if (command == "NTP_SERVER") {
    if (arg.length() > 0 && arg.length() <= MAX_NTP_LEN) {
      strncpy(config.ntpServer, arg.c_str(), MAX_NTP_LEN);
      config.ntpServer[MAX_NTP_LEN] = '\0';
      logEvent("Config: NTP_SERVER = \"" + arg + "\"");
      if (WiFi.status() == WL_CONNECTED) {
        ntpSynced = false;
        setupNTP();
      }
    } else {
      logEvent(F("ERROR: Неверный NTP-сервер"));
    }
    return;
  }

  // ------------------------------------------------------------------
  // SAVE
  // ------------------------------------------------------------------
  if (command == "SAVE") {
    saveConfig();
    logEvent(F("Config: сохранено. Перезагрузка через 1 сек..."));
    delay(1000);
    ESP.restart();
    return;
  }

  // ------------------------------------------------------------------
  // STATUS
  // ------------------------------------------------------------------
  if (command == "STATUS") {
    printStatusSerial();
    if (mqttClient.connected()) {
      String statusStr = buildStatusString();
      mqttClient.publish(MQTT_TOPIC_STATUS, statusStr.c_str(), false);
    }
    return;
  }

  // ------------------------------------------------------------------
  // TIME
  // ------------------------------------------------------------------
  if (command == "TIME") {
    if (ntpSynced) {
      logEvent("TIME: " + getMoscowTimeString());
    } else {
      logEvent(F("TIME: NTP ещё не синхронизирован. Ожидание..."));
    }
    return;
  }

  // ------------------------------------------------------------------
  // START
  // ------------------------------------------------------------------
  if (command == "START") {
    if (wateringActive) {
      logEvent(F("Полив уже запущен. Игнорируем START."));
      return;
    }

    if (!isWateringDay()) {
      logEvent("START: Сегодня не день полива. Расписание: " + getWateringDaysString());
      return;
    }

    wateringActive    = true;
    wateringState     = WS_RELAY_ON;
    currentRelayIndex = 0;
    cyclesCompleted   = 0;
    wateringTimerStart = millis();

    String cycleInfo = (config.cycleCount == 0) ?
      String("∞ (бесконечно)") :
      String(config.cycleCount) + " циклов";
    logEvent("START: Полив запущен. Циклов: " + cycleInfo +
             ", OnTime: " + String(config.onTimeSec) + "s" +
             ", Interval: " + String(config.intervalMin) + "min" +
             ", Days: " + getWateringDaysString());
    return;
  }

  // ------------------------------------------------------------------
  // STOP
  // ------------------------------------------------------------------
  if (command == "STOP") {
    wateringActive    = false;
    wateringState     = WS_IDLE;
    currentRelayIndex = 0;
    cyclesCompleted   = 0;
    setAllRelaysOff();
    logEvent(F("STOP: Полив остановлен. Все реле выключены."));
    return;
  }

  // ------------------------------------------------------------------
  // RELAY_ON <1-6>
  // ------------------------------------------------------------------
  if (command == "RELAY_ON") {
    int relayNum = arg.toInt();
    if (relayNum >= 1 && relayNum <= NUM_RELAYS) {
      setRelay(relayNum - 1, true);
      saveConfig();
    } else {
      logEvent("ERROR: Неверный номер реле: " + arg + " (1-6)");
    }
    return;
  }

  // ------------------------------------------------------------------
  // RELAY_OFF <1-6>
  // ------------------------------------------------------------------
  if (command == "RELAY_OFF") {
    int relayNum = arg.toInt();
    if (relayNum >= 1 && relayNum <= NUM_RELAYS) {
      setRelay(relayNum - 1, false);
      saveConfig();
    } else {
      logEvent("ERROR: Неверный номер реле: " + arg + " (1-6)");
    }
    return;
  }

  // ------------------------------------------------------------------
  // SET_INTERVAL <minutes>
  // ------------------------------------------------------------------
  if (command == "SET_INTERVAL") {
    uint32_t val = arg.toInt();
    if (val > 0) {
      config.intervalMin = val;
      logEvent("CONFIG: Interval = " + String(val) + " min");
    } else {
      logEvent(F("ERROR: Interval должен быть > 0"));
    }
    return;
  }

  // ------------------------------------------------------------------
  // SET_ONTIME <seconds>
  // ------------------------------------------------------------------
  if (command == "SET_ONTIME") {
    uint32_t val = arg.toInt();
    if (val > 0) {
      config.onTimeSec = val;
      logEvent("CONFIG: OnTime = " + String(val) + " sec");
    } else {
      logEvent(F("ERROR: OnTime должен быть > 0"));
    }
    return;
  }

  // ------------------------------------------------------------------
  // SET_CYCLE <N>
  // ------------------------------------------------------------------
  if (command == "SET_CYCLE") {
    int32_t val = arg.toInt();
    if (val >= 0) {
      config.cycleCount = (uint32_t)val;
      String desc = (val == 0) ? "∞ (бесконечно)" : String(val) + " циклов";
      logEvent("CONFIG: Cycle = " + desc);

      if (wateringActive) {
        logEvent(F("NOTE: Новое значение cycle применится после завершения текущего цикла."));
      }
    } else {
      logEvent(F("ERROR: Cycle должен быть >= 0"));
    }
    return;
  }

  // ------------------------------------------------------------------
  // SET_DAY0 ... SET_DAY7
  // ------------------------------------------------------------------
  if (command.startsWith("SET_DAY")) {
    String dayNumStr = command.substring(7);
    int dayNum = dayNumStr.toInt();

    if (dayNum >= 0 && dayNum <= 7) {
      int val = arg.toInt();
      if (val == 0 || val == 1) {
        if (val == 1) {
          config.wateringDays |= (1 << dayNum);
        } else {
          config.wateringDays &= ~(1 << dayNum);
        }

        String state = val ? "ВКЛ" : "ВЫКЛ";
        logEvent("CONFIG: " + String(getDayName(dayNum)) + " = " + state);

        if (dayNum == 0 && val == 1) {
          logEvent(F("NOTE: Режим 'каждый день' активен. Остальные дни игнорируются."));
        }
      } else {
        logEvent(F("ERROR: Значение должно быть 0 или 1"));
      }
    } else {
      logEvent(F("ERROR: День должен быть от 0 до 7"));
    }
    return;
  }

  // ------------------------------------------------------------------
  // Неизвестная команда
  // ------------------------------------------------------------------
  logEvent("UNKNOWN COMMAND: \"" + rawCmd + "\". Введите HELP.");
}

// ============================================================================
// ЛОГИКА АВТОПОЛИВА (ИСПРАВЛЕНО: корректная работа с таймерами)
// ============================================================================
void updateWateringLogic() {
  if (!wateringActive) return;

  switch (wateringState) {

    case WS_IDLE:
      break;

    case WS_RELAY_ON: {
      // Включаем реле, если оно ещё не включено
      if (config.relayStates[currentRelayIndex] != 1) {
        // ИСПРАВЛЕНИЕ: сначала сбрасываем таймер, потом включаем реле
        wateringTimerStart = millis();
        setRelay(currentRelayIndex, true);
        // ИСПРАВЛЕНИЕ: выходим из switch, чтобы не проверять условие в этом же такте
        break;
      }

      // ИСПРАВЛЕНИЕ: получаем СВЕЖЕЕ значение millis() при проверке условия
      unsigned long onTimeMs = (unsigned long)config.onTimeSec * 1000UL;
      unsigned long elapsed = millis() - wateringTimerStart;

      if (elapsed >= onTimeMs) {
        setRelay(currentRelayIndex, false);
        wateringTimerStart = millis();
        wateringState = WS_WAIT_INTERVAL;
      }
      break;
    }

    case WS_WAIT_INTERVAL: {
      unsigned long intervalMs = (unsigned long)config.intervalMin * 60UL * 1000UL;
      // ИСПРАВЛЕНИЕ: свежее значение millis()
      unsigned long elapsed = millis() - wateringTimerStart;

      if (elapsed >= intervalMs) {
        currentRelayIndex++;

        if (currentRelayIndex >= NUM_RELAYS) {
          cyclesCompleted++;

          logEvent("CYCLE: завершён цикл #" + String(cyclesCompleted) +
                   ". Выполнено: " + String(cyclesCompleted) + "/" +
                   ((config.cycleCount == 0) ? String("∞") : String(config.cycleCount)));

          bool continueCycle = (config.cycleCount == 0) ||
                               (cyclesCompleted < config.cycleCount);

          if (continueCycle) {
            if (!isWateringDay()) {
              wateringActive = false;
              wateringState  = WS_IDLE;
              currentRelayIndex = 0;
              cyclesCompleted = 0;
              logEvent("SCHEDULE: Сегодня не день полива. Расписание: " + getWateringDaysString());
            } else {
              currentRelayIndex = 0;
              wateringState = WS_RELAY_ON;
              wateringTimerStart = millis();
            }
          } else {
            wateringActive = false;
            wateringState  = WS_IDLE;
            currentRelayIndex = 0;
            cyclesCompleted = 0;
            logEvent("FINISH: Все " + String(config.cycleCount) +
                     " циклов выполнены. Полив остановлен.");
          }
        } else {
          wateringState = WS_RELAY_ON;
          wateringTimerStart = millis();
        }
      }
      break;
    }
  }
}

// ============================================================================
// ЛОГИРОВАНИЕ
// ============================================================================
void logEvent(const String& message) {
  String timeStr = getMoscowTimeString();
  String logLine = "[" + timeStr + "] " + message;
  Serial.println(logLine);

  if (mqttClient.connected()) {
    String mqttMsg = logLine;
    if (mqttMsg.length() > 450) {
      mqttMsg = mqttMsg.substring(0, 450) + "...";
    }
    mqttClient.publish(MQTT_TOPIC_LOG, mqttMsg.c_str(), false);
  }
}

// ============================================================================
// ПУБЛИКАЦИЯ СТАТУСА В MQTT (JSON)
// ============================================================================
void publishStatus() {
  String statusStr = buildStatusString();
  if (mqttClient.connected()) {
    mqttClient.publish(MQTT_TOPIC_STATUS, statusStr.c_str(), false);
  }
}

// ============================================================================
// ЧИТАЕМЫЙ ВЫВОД СТАТУСА В SERIAL
// ============================================================================
void printStatusSerial() {
  Serial.println(F("==========================================="));
  Serial.println(F(" СТАТУС СИСТЕМЫ"));
  Serial.println(F("==========================================="));

  Serial.print(F("Время МСК         : "));
  Serial.println(getMoscowTimeString());
  Serial.print(F("Uptime            : "));
  Serial.println(getUptimeString());
  Serial.print(F("NTP синхрониз.    : "));
  Serial.println(ntpSynced ? F("ДА") : F("НЕТ"));

  Serial.println(F("--- Сеть ---"));
  Serial.print(F("Wi-Fi подключен   : "));
  Serial.println(WiFi.status() == WL_CONNECTED ? F("ДА") : F("НЕТ"));
  Serial.print(F("IP-адрес          : "));
  Serial.println(WiFi.localIP().toString());
  Serial.print(F("SSID              : "));
  Serial.println(config.wifiSSID);
  Serial.print(F("MQTT подключен    : "));
  Serial.println(mqttClient.connected() ? F("ДА") : F("НЕТ"));
  Serial.print(F("MQTT брокер       : "));
  Serial.print(config.mqttHost);
  Serial.print(F(":"));
  Serial.println(config.mqttPort);
  Serial.print(F("NTP-сервер        : "));
  Serial.println(config.ntpServer);

  Serial.println(F("--- Параметры полива ---"));
  Serial.print(F("Время вкл. реле   : "));
  Serial.print(config.onTimeSec);
  Serial.println(F(" сек"));
  Serial.print(F("Интервал          : "));
  Serial.print(config.intervalMin);
  Serial.println(F(" мин"));
  Serial.print(F("Кол-во циклов     : "));
  if (config.cycleCount == 0) {
    Serial.println(F("∞ (бесконечно)"));
  } else {
    Serial.println(config.cycleCount);
  }
  Serial.print(F("Дни полива        : "));
  Serial.println(getWateringDaysString());

  Serial.println(F("  Детали по дням:"));
  for (uint8_t i = 0; i <= 7; i++) {
    Serial.print(F("    day"));
    Serial.print(i);
    Serial.print(F(" ("));
    Serial.print(getDayName(i));
    Serial.print(F(")      : "));
    Serial.println(isDayEnabled(i) ? F("ВКЛ") : F("ВЫКЛ"));
  }

  Serial.println(F("--- Состояние полива ---"));
  Serial.print(F("Полив активен     : "));
  Serial.println(wateringActive ? F("ДА") : F("НЕТ"));
  Serial.print(F("Состояние FSM     : "));
  switch (wateringState) {
    case WS_IDLE:          Serial.println(F("IDLE (ожидание)")); break;
    case WS_RELAY_ON:      Serial.println(F("RELAY_ON (реле включено)")); break;
    case WS_WAIT_INTERVAL: Serial.println(F("WAIT_INTERVAL (пауза)")); break;
  }
  Serial.print(F("Текущее реле      : "));
  Serial.println(currentRelayIndex + 1);
  Serial.print(F("Выполнено циклов  : "));
  Serial.println(cyclesCompleted);

  Serial.println(F("--- Состояние реле ---"));
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    Serial.print(F("  Реле "));
    Serial.print(i + 1);
    Serial.print(F(" (D"));
    Serial.print(getPinNumber(RELAY_PINS[i]));
    Serial.print(F(")         : "));
    Serial.println(config.relayStates[i] ? F("ВКЛ") : F("ВЫКЛ"));
  }

  Serial.println(F("==========================================="));
}

// ============================================================================
// JSON-СТАТУС ДЛЯ MQTT
// ============================================================================
String buildStatusString() {
  String s = "{";
  s += "\"moscow_time\":\"" + getMoscowTimeString() + "\",";
  s += "\"ntp_synced\":" + String(ntpSynced ? "true" : "false") + ",";
  s += "\"uptime\":\"" + getUptimeString() + "\",";
  s += "\"wifi\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  s += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  s += "\"mqtt\":" + String(mqttClient.connected() ? "true" : "false") + ",";
  s += "\"watering\":" + String(wateringActive ? "true" : "false") + ",";
  s += "\"state\":" + String((int)wateringState) + ",";
  s += "\"current_relay\":" + String(currentRelayIndex + 1) + ",";
  s += "\"cycle_count\":" + String(config.cycleCount) + ",";
  s += "\"cycles_completed\":" + String(cyclesCompleted) + ",";
  s += "\"interval_min\":" + String(config.intervalMin) + ",";
  s += "\"ontime_sec\":" + String(config.onTimeSec) + ",";
  s += "\"watering_days\":\"" + getWateringDaysString() + "\",";

  s += "\"days\":{";
  s += "\"day0\":" + String(isDayEnabled(0) ? 1 : 0) + ",";
  s += "\"day1\":" + String(isDayEnabled(1) ? 1 : 0) + ",";
  s += "\"day2\":" + String(isDayEnabled(2) ? 1 : 0) + ",";
  s += "\"day3\":" + String(isDayEnabled(3) ? 1 : 0) + ",";
  s += "\"day4\":" + String(isDayEnabled(4) ? 1 : 0) + ",";
  s += "\"day5\":" + String(isDayEnabled(5) ? 1 : 0) + ",";
  s += "\"day6\":" + String(isDayEnabled(6) ? 1 : 0) + ",";
  s += "\"day7\":" + String(isDayEnabled(7) ? 1 : 0);
  s += "},";

  s += "\"relays\":[";
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    s += String(config.relayStates[i]);
    if (i < NUM_RELAYS - 1) s += ",";
  }
  s += "],";
  s += "\"ssid\":\"" + String(config.wifiSSID) + "\",";
  s += "\"mqtt_host\":\"" + String(config.mqttHost) + ":" + String(config.mqttPort) + "\",";
  s += "\"ntp_server\":\"" + String(config.ntpServer) + "\"";
  s += "}";
  return s;
}

// ============================================================================
// УТИЛИТЫ
// ============================================================================
String getUptimeString() {
  unsigned long ms = millis();
  unsigned long sec  = ms / 1000;
  unsigned long min  = sec / 60;
  unsigned long hour = min / 60;
  unsigned long day  = hour / 24;

  sec  %= 60;
  min  %= 60;
  hour %= 24;

  String result = String(day) + "d ";
  if (hour < 10) result += "0";
  result += String(hour) + ":";
  if (min < 10) result += "0";
  result += String(min) + ":";
  if (sec < 10) result += "0";
  result += String(sec);

  return result;
}
