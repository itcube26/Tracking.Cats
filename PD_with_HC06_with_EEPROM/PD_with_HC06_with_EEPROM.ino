#define PWMA 6
#define PWMB 5
#define AIN1 7
#define AIN2 8
#define BIN1 10
#define BIN2 11
#define STBY 9

#include <QTRSensors.h>
#include <EEPROM.h>

// ========== НАСТРОЙКИ ==========
float Kp = 0.01;
float Kd = 0.008;

#define rightMaxSpeed 150
#define leftMaxSpeed 150
int rightBaseSpeed = 50;
int leftBaseSpeed = 50;

#define SensorCount 8
#define EMITTER_PIN 2

bool isRunning = false;
// ================================

QTRSensors qtr;
uint16_t sensorValues[SensorCount];

// Буфер для команд
#define CMD_BUFFER_SIZE 32
char cmdBuffer[CMD_BUFFER_SIZE];
uint8_t cmdIndex = 0;

// Телеметрия
bool telemetryEnabled = false;

// ========== EEPROM СТРУКТУРА ==========
const byte EEPROM_MAGIC = 0xA5;
struct EepromConfig {
    byte magic;
    float kp;
    float kd;
    int leftBase;
    int rightBase;
} eepromConfig;

void loadFromEEPROM() {
    EEPROM.get(0, eepromConfig);
    if (eepromConfig.magic == EEPROM_MAGIC) {
        Kp = eepromConfig.kp;
        Kd = eepromConfig.kd;
        leftBaseSpeed = eepromConfig.leftBase;
        rightBaseSpeed = eepromConfig.rightBase;
        sendBT(F("📂 Загружено из EEPROM\n"));
    } else {
        sendBT(F("📦 Первая загрузка. Используются дефолты.\n"));
    }
}

void saveToEEPROM() {
    eepromConfig.magic = EEPROM_MAGIC;
    eepromConfig.kp = Kp;
    eepromConfig.kd = Kd;
    eepromConfig.leftBase = leftBaseSpeed;
    eepromConfig.rightBase = rightBaseSpeed;
    EEPROM.put(0, eepromConfig);
    sendBT(F("💾 Сохранено в EEPROM\n"));
}

void factoryResetEEPROM() {
    // Записываем неверный магический байт, чтобы сбросить настройки
    EEPROM.update(0, 0x00);
    Kp = 0.01; Kd = 0.008; leftBaseSpeed = 50; rightBaseSpeed = 50;
    sendBT(F("🔄 Сброс настроек. Перезагрузите Arduino.\n"));
}
// ======================================

void setup(){
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A7, A6, A5, A4, A3, A2, A1, A0}, SensorCount);
  qtr.setEmitterPin(EMITTER_PIN);

  Serial.begin(9600);
  delay(1000);

  // Загружаем сохранённые параметры
  loadFromEEPROM();

  // Калибровка
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  sendBT(F("🔄 Калибровка...\n"));
  for (uint16_t i = 0; i < 400; i++) qtr.calibrate();
  digitalWrite(LED_BUILTIN, LOW);

  sendBT(F("✅ Готово. Робот стоит.\n"));
  sendBT(F("Type 'help' for commands\n"));
  
  // Гарантируем остановку
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  isRunning = false;
  
  printParams();
}

int lastError = 0;

void loop()
{
  handleBluetoothCommands();

  if (!isRunning) {
    delay(20);
    return;
  }

  // Движение
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);

  uint16_t position = qtr.readLineBlack(sensorValues);
  int error = position - 3500;
  
  int motorSpeed = Kp * error + Kd * (error - lastError);
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
// 📡 ОБРАБОТКА КОМАНД
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
    
    if (cmdIndex < CMD_BUFFER_SIZE - 1) {
      cmdBuffer[cmdIndex++] = c;
    }
  }
}

void parseCommand(char* cmd) {
  while (*cmd == ' ') cmd++;
  
  // ▶ START
  if (strcmp(cmd, "start") == 0) {
    if (isRunning) sendBT(F("⚠ Уже работает!\n"));
    else {
      isRunning = true;
      lastError = 0;
      sendBT(F("▶ START\n"));
    }
    return;
  }
  
  // ⏹ STOP
  if (strcmp(cmd, "stop") == 0) {
    if (!isRunning) sendBT(F("⚠ Уже остановлен!\n"));
    else {
      isRunning = false;
      analogWrite(PWMA, 0); analogWrite(PWMB, 0);
      sendBT(F("⏹ STOP\n"));
    }
    return;
  }
  
  // KP
  if (strncmp(cmd, "kp:", 3) == 0) {
    float val = atof(cmd + 3);
    if (val >= 0 && val <= 1.0) {
      Kp = val;
      saveToEEPROM();
      sendBT(F("✅ Kp=")); sendBT(String(Kp, 4) + "\n");
      printParams();
    } else sendBT(F("❌ Kp range: 0.0...1.0\n"));
    return;
  }
  
  // KD
  if (strncmp(cmd, "kd:", 3) == 0) {
    float val = atof(cmd + 3);
    if (val >= 0 && val <= 1.0) {
      Kd = val;
      saveToEEPROM();
      sendBT(F("✅ Kd=")); sendBT(String(Kd, 4) + "\n");
      printParams();
    } else sendBT(F("❌ Kd range: 0.0...1.0\n"));
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
  
  // PARAMS
  if (strcmp(cmd, "params") == 0) { printParams(); return; }

  // TELEMETRY
  if (strncmp(cmd, "telemetry:", 10) == 0) {
    if (strcmp(cmd + 10, "on") == 0) { telemetryEnabled = true; sendBT(F("✅ Telemetry: ON\n")); }
    else if (strcmp(cmd + 10, "off") == 0) { telemetryEnabled = false; sendBT(F("✅ Telemetry: OFF\n")); }
    return;
  }

  // 💾 SAVE (принудительное сохранение, если нужно)
  if (strcmp(cmd, "save") == 0) { saveToEEPROM(); return; }

  // 🔄 RESET (сброс к заводским настройкам)
  if (strcmp(cmd, "reset") == 0) { factoryResetEEPROM(); return; }
  
  // HELP
  if (strcmp(cmd, "help") == 0) {
    sendBT(F("\n📋 Commands:\n"));
    sendBT(F("start          — поехать\n"));
    sendBT(F("stop           — остановиться\n"));
    sendBT(F("kp:<val>       — P (0.0-1.0)\n"));
    sendBT(F("kd:<val>       — D (0.0-1.0)\n"));
    sendBT(F("base:L,R       — скорость (0-255)\n"));
    sendBT(F("telemetry:on/off — данные\n"));
    sendBT(F("save           — сохранить в EEPROM\n"));
    sendBT(F("reset          — сброс настроек\n"));
    sendBT(F("params         — показать настройки\n"));
    sendBT(F("help           — справка\n"));
    return;
  }
  
  sendBT(F("❓ Unknown: ")); sendBT(String(cmd) + "\n");
}

void printParams() {
  sendBT(F("⚙ Kp=")); sendBT(String(Kp, 4));
  sendBT(F(" | Kd=")); sendBT(String(Kd, 4));
  sendBT(F(" | Base L=")); sendBT(String(leftBaseSpeed));
  sendBT(F(" R=")); sendBT(String(rightBaseSpeed));
  sendBT(isRunning ? F(" | RUN\n") : F(" | STOP\n"));
}