#define PWMA 6
#define PWMB 5
#define AIN1 7
#define AIN2 8
#define BIN1 10
#define BIN2 11
#define STBY 9
#define BTN_PIN 3  // Кнопка: D3 ↔ GND

#include <QTRSensors.h>
#include <EEPROM.h>

// ========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ==========
float Kp = 0.01;
float Ki = 0.00; // Интегральный коэффициент
float Kd = 0.008;
int lastError = 0;
float integral = 0.0; // Накопленная ошибка

// Скорости
int rightMaxSpeed = 150;
int leftMaxSpeed = 150;
int rightBaseSpeed = 50;
int leftBaseSpeed = 50;

#define SensorCount 8
#define EMITTER_PIN 2

bool isRunning = false;
// ===========================================

QTRSensors qtr;
uint16_t sensorValues[SensorCount];

#define CMD_BUFFER_SIZE 32
char cmdBuffer[CMD_BUFFER_SIZE];
uint8_t cmdIndex = 0;

bool telemetryEnabled = false;

// ========== EEPROM ==========
const byte EEPROM_MAGIC = 0xA5;
struct EepromConfig {
    byte magic;
    float kp;
    float ki;
    float kd;
    int leftBase;
    int rightBase;
    int leftMax;
    int rightMax;
} eepromConfig;

void loadFromEEPROM() {
    EEPROM.get(0, eepromConfig);
    if (eepromConfig.magic == EEPROM_MAGIC) {
        Kp = eepromConfig.kp;
        Ki = eepromConfig.ki;
        Kd = eepromConfig.kd;
        leftBaseSpeed = eepromConfig.leftBase;
        rightBaseSpeed = eepromConfig.rightBase;
        leftMaxSpeed = eepromConfig.leftMax;
        rightMaxSpeed = eepromConfig.rightMax;
        sendBT(F("📂 Загружено из EEPROM\n"));
    } else {
        sendBT(F("📦 Первая загрузка. Дефолтные параметры.\n"));
    }
}

void saveToEEPROM() {
    eepromConfig.magic = EEPROM_MAGIC;
    eepromConfig.kp = Kp;
    eepromConfig.ki = Ki;
    eepromConfig.kd = Kd;
    eepromConfig.leftBase = leftBaseSpeed;
    eepromConfig.rightBase = rightBaseSpeed;
    eepromConfig.leftMax = leftMaxSpeed;
    eepromConfig.rightMax = rightMaxSpeed;
    EEPROM.put(0, eepromConfig);
    sendBT(F("💾 Сохранено в EEPROM\n"));
}

void factoryResetEEPROM() {
    EEPROM.update(0, 0x00);
    Kp = 0.01; Ki = 0.00; Kd = 0.008; 
    leftBaseSpeed = 50; rightBaseSpeed = 50;
    leftMaxSpeed = 150; rightMaxSpeed = 150;
    sendBT(F("🔄 Сброс. Перезагрузите Arduino.\n"));
}
// ============================

// ========== КНОПКА ==========
bool lastBtnState = HIGH;
unsigned long lastToggleTime = 0;
const unsigned long BTN_DEBOUNCE = 200;

void handleButton() {
    bool currentBtnState = digitalRead(BTN_PIN);
    if (currentBtnState == LOW && lastBtnState == HIGH && (millis() - lastToggleTime) > BTN_DEBOUNCE) {
        lastToggleTime = millis();
        toggleRobotState();
    }
    lastBtnState = currentBtnState;
}

void toggleRobotState() {
    isRunning = !isRunning;
    if (isRunning) {
        lastError = 0;
        integral = 0.0; // Сброс интеграла при старте
        sendBT(F("▶ START\n"));
    } else {
        analogWrite(PWMA, 0);
        analogWrite(PWMB, 0);
        integral = 0.0; // Сброс интеграла при остановке
        sendBT(F("⏹ STOP\n"));
    }
}
// ===========================

void setup(){
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);

  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A7, A6, A5, A4, A3, A2, A1, A0}, SensorCount);
  qtr.setEmitterPin(EMITTER_PIN);

  Serial.begin(9600);
  delay(1000);

  loadFromEEPROM();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  sendBT(F("🔄 Калибровка... (проведите над линией)\n"));
  for (uint16_t i = 0; i < 400; i++) qtr.calibrate();
  digitalWrite(LED_BUILTIN, LOW);

  sendBT(F("✅ Калибровка завершена.\n"));
  sendBT(F("⏹ Робот стоит. Старт: кнопка D3 или команда 'start'\n"));
  
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  isRunning = false;
  
  printParams();
}

void loop()
{
  handleBluetoothCommands();
  handleButton();

  if (!isRunning) {
    delay(20);
    return;
  }

  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);

  uint16_t position = qtr.readLineBlack(sensorValues);
  int error = position - 3500;
  
  // === PID РАСЧЁТ ===
  integral += error;
  // Anti-windup: ограничиваем накопление, чтобы интеграл не "уносил" моторы в насыщение
  if (integral > 3000) integral = 3000;
  if (integral < -3000) integral = -3000;

  int motorSpeed = (Kp * error) + (Ki * integral) + (Kd * (error - lastError));
  lastError = error;

  int rightMotorSpeed = constrain(rightBaseSpeed + motorSpeed, 0, rightMaxSpeed);
  int leftMotorSpeed  = constrain(leftBaseSpeed - motorSpeed, 0, leftMaxSpeed);
  
  analogWrite(PWMA, rightMotorSpeed);
  analogWrite(PWMB, leftMotorSpeed);

  if (telemetryEnabled) {
    static unsigned long lastSend = 0;
    if (millis() - lastSend > 100) {
      Serial.print("P:"); Serial.print(position);
      Serial.print(" E:"); Serial.print(error);
      Serial.print(" L:"); Serial.print(leftMotorSpeed);
      Serial.print(" R:"); Serial.println(rightMotorSpeed);
      lastSend = millis();
    }
  }
  
  delay(10);
}

// ============================================================================
// 📡 BLUETOOTH
// ============================================================================

void sendBT(const __FlashStringHelper* str) { Serial.print(str); }
void sendBT(String str) { Serial.print(str); }

void handleBluetoothCommands() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      cmdBuffer[cmdIndex] = '\0';
      if (cmdIndex > 0) {
        parseCommand(cmdBuffer);
        Serial.print(F("> ")); Serial.println(cmdBuffer);
      }
      cmdIndex = 0;
      continue;
    }
    if (cmdIndex < CMD_BUFFER_SIZE - 1) cmdBuffer[cmdIndex++] = c;
  }
}

void parseCommand(char* cmd) {
  while (*cmd == ' ') cmd++;
  
  if (strcmp(cmd, "start") == 0) {
    if (!isRunning) toggleRobotState();
    else sendBT(F("⚠ Уже работает!\n"));
    return;
  }
  
  if (strcmp(cmd, "stop") == 0) {
    if (isRunning) toggleRobotState();
    else sendBT(F("⚠ Уже остановлен!\n"));
    return;
  }
  
  // KP
  if (strncmp(cmd, "kp:", 3) == 0) {
    float val = atof(cmd + 3);
    if (val >= 0 && val <= 1.0) { Kp = val; saveToEEPROM(); sendBT(F("✅ Kp=")); sendBT(String(Kp, 4) + "\n"); printParams(); }
    else sendBT(F("❌ Kp range: 0.0...1.0\n"));
    return;
  }
  
  // 🆕 KI
  if (strncmp(cmd, "ki:", 3) == 0) {
    float val = atof(cmd + 3);
    if (val >= 0 && val <= 0.5) {
      Ki = val;
      integral = 0.0; // Сброс интеграла при смене коэффициента
      saveToEEPROM();
      sendBT(F("✅ Ki=")); sendBT(String(Ki, 4) + "\n");
      printParams();
    } else sendBT(F("❌ Ki range: 0.0...0.5\n"));
    return;
  }

  // KD
  if (strncmp(cmd, "kd:", 3) == 0) {
    float val = atof(cmd + 3);
    if (val >= 0 && val <= 1.0) { Kd = val; saveToEEPROM(); sendBT(F("✅ Kd=")); sendBT(String(Kd, 4) + "\n"); printParams(); }
    else sendBT(F("❌ Kd range: 0.0...1.0\n"));
    return;
  }

  // BASE
  if (strncmp(cmd, "base:", 5) == 0) {
    int l = atoi(cmd + 5);
    char* comma = strchr(cmd + 5, ',');
    if (comma) {
      int r = atoi(comma + 1);
      leftBaseSpeed = constrain(l, 0, 255);
      rightBaseSpeed = constrain(r, 0, 255);
      saveToEEPROM();
      sendBT(F("✅ Base L=")); sendBT(String(leftBaseSpeed));
      sendBT(F(" R=")); sendBT(String(rightBaseSpeed) + "\n");
      printParams();
    } else sendBT(F("❌ Format: base:left,right\n"));
    return;
  }

  // MAX
  if (strncmp(cmd, "max:", 4) == 0) {
    int l = atoi(cmd + 4);
    char* comma = strchr(cmd + 4, ',');
    if (comma) {
      int r = atoi(comma + 1);
      leftMaxSpeed = constrain(l, 0, 255);
      rightMaxSpeed = constrain(r, 0, 255);
      saveToEEPROM();
      sendBT(F("✅ Max L=")); sendBT(String(leftMaxSpeed));
      sendBT(F(" R=")); sendBT(String(rightMaxSpeed) + "\n");
      printParams();
    } else sendBT(F("❌ Format: max:left,right\n"));
    return;
  }
  
  if (strcmp(cmd, "params") == 0) { printParams(); return; }
  
  if (strncmp(cmd, "telemetry:", 10) == 0) {
    if (strcmp(cmd + 10, "on") == 0) { telemetryEnabled = true; sendBT(F("✅ Telemetry: ON\n")); }
    else if (strcmp(cmd + 10, "off") == 0) { telemetryEnabled = false; sendBT(F("✅ Telemetry: OFF\n")); }
    return;
  }

  if (strcmp(cmd, "save") == 0) { saveToEEPROM(); return; }
  if (strcmp(cmd, "reset") == 0) { factoryResetEEPROM(); return; }
  
  if (strcmp(cmd, "help") == 0) {
    sendBT(F("\n📋 Commands:\n"));
    sendBT(F("start/stop     — управление (BT или кнопка D3)\n"));
    sendBT(F("kp:<val>       — Proportional (0.0-1.0)\n"));
    sendBT(F("ki:<val>       — Integral     (0.0-0.5)\n"));
    sendBT(F("kd:<val>       — Derivative   (0.0-1.0)\n"));
    sendBT(F("base:L,R       — базовая скорость\n"));
    sendBT(F("max:L,R        — максимальная скорость\n"));
    sendBT(F("telemetry:on/off — поток данных\n"));
    sendBT(F("save           — в EEPROM\n"));
    sendBT(F("reset          — сброс настроек\n"));
    sendBT(F("params         — показать текущие\n"));
    sendBT(F("help           — справка\n"));
    return;
  }
  
  sendBT(F("❓ Unknown: ")); sendBT(String(cmd) + "\n");
}

void printParams() {
  sendBT(F("⚙ Kp=")); sendBT(String(Kp, 4));
  sendBT(F(" | Ki=")); sendBT(String(Ki, 4));
  sendBT(F(" | Kd=")); sendBT(String(Kd, 4));
  sendBT(F(" | Base L=")); sendBT(String(leftBaseSpeed));
  sendBT(F(" R=")); sendBT(String(rightBaseSpeed));
  sendBT(F(" | Max L=")); sendBT(String(leftMaxSpeed));
  sendBT(F(" R=")); sendBT(String(rightMaxSpeed));
  sendBT(isRunning ? F(" | RUN\n") : F(" | STOP\n"));
}