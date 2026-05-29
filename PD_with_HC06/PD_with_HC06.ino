#define PWMA 6
#define PWMB 5
#define AIN1 7
#define AIN2 8
#define BIN1 10
#define BIN2 11
#define STBY 9

#include <QTRSensors.h>

// ========== НАСТРОЙКИ ==========
float Kp = 0.015;
float Kd = 0.01;

#define rightMaxSpeed 175
#define leftMaxSpeed 175
int rightBaseSpeed = 100;  // Изменяемые переменные
int leftBaseSpeed = 100;

#define SensorCount 8
#define EMITTER_PIN 2

// Состояние робота
bool isRunning = false; // По умолчанию СТОИТ после калибровки
// ================================

QTRSensors qtr;
uint16_t sensorValues[SensorCount];

// Буфер для команд
#define CMD_BUFFER_SIZE 32
char cmdBuffer[CMD_BUFFER_SIZE];
uint8_t cmdIndex = 0;

// Телеметрия
bool telemetryEnabled = false;

void setup(){
  // Инициализация пинов моторов
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  // Инициализация датчиков
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A7, A6, A5, A4, A3, A2, A1, A0}, SensorCount);
  qtr.setEmitterPin(EMITTER_PIN);

  Serial.begin(9600);
  delay(1000); // Даем время HC-06 подключиться

  // --- ЭТАП КАЛИБРОВКИ ---
  sendBT(F("🔄 Calibration started...\n"));
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Крутимся или стоим - зависит от вашей механики, 
  // но обычно калибровка требует провоза датчиков над линией вручную 
  // или авто-разворотом. Здесь стандартный цикл calibrate().
  for (uint16_t i = 0; i < 400; i++) {
    qtr.calibrate();
  }
  
  digitalWrite(LED_BUILTIN, LOW);
  sendBT(F("✅ Calibration done.\n"));
  sendBT(F("⏹ Robot is STOPPED. Ready for commands.\n"));
  sendBT(F("Type 'help' to see commands.\n"));
  
  // Гарантируем остановку моторов
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  isRunning = false;
  
  printParams();
}

int lastError = 0;

void loop()
{
  // 1. ВСЕГДА слушаем команды (даже если стоим)
  handleBluetoothCommands();

  // 2. Если НЕ бежим - просто выходим из loop (моторы уже выключены)
  if (!isRunning) {
    delay(20); 
    return;
  }

  // 3. Логика движения (только если isRunning == true)
  
  // Направление вращения (зависит от подключения моторов)
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);

  uint16_t position = qtr.readLineBlack(sensorValues);
  int error = position - 3500; // 3500 - середина для 8 сенсоров (0..7000)
  
  // PD регулятор
  int motorSpeed = Kp * error + Kd * (error - lastError);
  lastError = error;

  // Расчет скоростей
  int rightMotorSpeed = constrain(rightBaseSpeed + motorSpeed, 0, rightMaxSpeed);
  int leftMotorSpeed  = constrain(leftBaseSpeed - motorSpeed, 0, leftMaxSpeed);
  
  // Подача ШИМ
  analogWrite(PWMA, rightMotorSpeed);
  analogWrite(PWMB, leftMotorSpeed);

  // Телеметрия (если включена)
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
// 📡 ОБРАБОТКА КОМАНД BLUETOOTH
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
        Serial.print(F("> ")); Serial.println(cmdBuffer); // Эхо
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
  while (*cmd == ' ') cmd++; // Убираем пробелы в начале
  
  // ▶ START
  if (strcmp(cmd, "start") == 0) {
    if (isRunning) {
      sendBT(F("⚠ Already running!\n"));
    } else {
      isRunning = true;
      lastError = 0; // Сброс ошибки для плавного старта
      sendBT(F("▶ Robot STARTED\n"));
    }
    return;
  }
  
  // ⏹ STOP
  if (strcmp(cmd, "stop") == 0) {
    if (!isRunning) {
      sendBT(F("⚠ Already stopped!\n"));
    } else {
      isRunning = false;
      analogWrite(PWMA, 0);
      analogWrite(PWMB, 0);
      sendBT(F("⏹ Robot STOPPED\n"));
    }
    return;
  }
  
  // KP
  if (strncmp(cmd, "kp:", 3) == 0) {
    float val = atof(cmd + 3);
    if (val >= 0 && val <= 1.0) {
      Kp = val;
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
      sendBT(F("✅ Kd=")); sendBT(String(Kd, 4) + "\n");
      printParams();
    } else sendBT(F("❌ Kd range: 0.0...1.0\n"));
    return;
  }

  // BASE SPEED
  if (strncmp(cmd, "base:", 5) == 0) {
    int l = atoi(cmd + 5);
    char* comma = strchr(cmd + 5, ',');
    if (comma) {
      int r = atoi(comma + 1);
      leftBaseSpeed = constrain(l, 0, 255);
      rightBaseSpeed = constrain(r, 0, 255);
      sendBT(F("✅ Base L=")); sendBT(String(leftBaseSpeed));
      sendBT(F(" R=")); sendBT(String(rightBaseSpeed) + "\n");
      printParams();
    } else {
      sendBT(F("❌ Format: base:left,right\n"));
    }
    return;
  }
  
  // PARAMS
  if (strcmp(cmd, "params") == 0) {
    printParams();
    return;
  }

  // TELEMETRY
  if (strncmp(cmd, "telemetry:", 10) == 0) {
    if (strcmp(cmd + 10, "on") == 0) {
      telemetryEnabled = true;
      sendBT(F("✅ Telemetry: ON\n"));
    } else if (strcmp(cmd + 10, "off") == 0) {
      telemetryEnabled = false;
      sendBT(F("✅ Telemetry: OFF\n"));
    }
    return;
  }
  
  // HELP
  if (strcmp(cmd, "help") == 0) {
    sendBT(F("\n📋 Commands:\n"));
    sendBT(F("start          — начать движение\n"));
    sendBT(F("stop           — остановить\n"));
    sendBT(F("kp:<val>       — коэффициент P (0.0-1.0)\n"));
    sendBT(F("kd:<val>       — коэффициент D (0.0-1.0)\n"));
    sendBT(F("base:L,R       — базовая скорость (0-255)\n"));
    sendBT(F("telemetry:on/off — данные с датчиков\n"));
    sendBT(F("params         — текущие настройки\n"));
    sendBT(F("help           — справка\n"));
    sendBT(F("───────────────\n"));
    return;
  }
  
  sendBT(F("❓ Unknown: ")); sendBT(String(cmd) + "\n");
}

void printParams() {
  sendBT(F("⚙ Kp=")); sendBT(String(Kp, 4));
  sendBT(F(" | Kd=")); sendBT(String(Kd, 4));
  sendBT(F(" | Base L=")); sendBT(String(leftBaseSpeed));
  sendBT(F(" R=")); sendBT(String(rightBaseSpeed));
  sendBT(isRunning ? F(" | Status: RUNNING\n") : F(" | Status: STOPPED\n"));
}