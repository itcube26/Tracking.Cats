#define PWMA 6
#define PWMB 5
#define AIN1 7
#define AIN2 8
#define BIN1 10
#define BIN2 11
#define STBY 9

#include <QTRSensors.h>

// ========== НАСТРОЙКИ ==========
#define SWEEP_TIME 200           // ⏱ Время свипа в одну сторону (мс)
#define ARC_BASE_SPEED 90        // 🚗 Базовая скорость обоих моторов
#define ARC_DIFF 35              // 🔄 Разница скоростей для плавного поворота
// ===============================

QTRSensors qtr;
const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];

// === 🛑 Остановка ===
void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
}

// === 🌀 Плавный поворот (оба мотора вперёд, разная скорость) ===
// direction: true = вправо, false = влево
void smoothTurn(bool direction, uint8_t baseSpeed = ARC_BASE_SPEED, uint8_t diff = ARC_DIFF) {
  if (direction) {
    // ➡️ Вправо: левый мотор быстрее → робот плавно поворачивает вправо
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
    analogWrite(PWMA, baseSpeed + diff);
    digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);
    analogWrite(PWMB, baseSpeed - diff);
  } else {
    // ⬅️ Влево: правый мотор быстрее → робот плавно поворачивает влево
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
    analogWrite(PWMA, baseSpeed - diff);
    digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);
    analogWrite(PWMB, baseSpeed + diff);
  }
}

// === Обёртки для удобства ===
void turnLeft(uint8_t speed = ARC_BASE_SPEED, uint8_t diff = ARC_DIFF) {
  smoothTurn(false, speed, diff);
}

void turnRight(uint8_t speed = ARC_BASE_SPEED, uint8_t diff = ARC_DIFF) {
  smoothTurn(true, speed, diff);
}

// === Вперёд (для основного цикла) ===
void forward(uint8_t speed = 100) {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speed);
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, speed);
}

void setup() {
  Serial.begin(115200);  // ✅ Единый baud rate
  
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);
  
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  
  stopMotors();
  
  // === Настройка QTR ===
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A7, A6, A5, A4, A3, A2, A1, A0}, SensorCount);
  qtr.setEmitterPin(2);
  
  delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // 🔴 Калибровка началась
  
  Serial.println("↔️ Старт калибровки: плавный свип влево-вправо по 200 мс...");
  
  // === 🔄 Цикл калибровки с боковым сканированием ===
  for (uint16_t i = 0; i < 400; i++) {
    qtr.calibrate();
    
    // Каждые 2 итерации (~50 мс * 2 = 100 мс) меняем направление
    // Но держим направление по 200 мс = ~4 итерации цикла
    if (i % 4 == 0) {
      turnLeft();
      delay(SWEEP_TIME);      // ⬅️ 200 мс влево
      qtr.calibrate();        // Доп. замер на пике поворота
    }
    else if (i % 4 == 2) {
      turnRight();
      delay(SWEEP_TIME);      // ➡️ 200 мс вправо
      qtr.calibrate();        // Доп. замер на пике поворота
    }
  }
  
  stopMotors();
  digitalWrite(LED_BUILTIN, LOW);  // 🟢 Калибровка завершена
  Serial.println("✅ Калибровка готова!");
  
  // === Вывод мин/макс значений ===
  Serial.print("Min: ");
  for (uint8_t i = 0; i < SensorCount; i++) {
    Serial.print(qtr.calibrationOn.minimum[i]);
    Serial.print(' ');
  }
  Serial.println();
  
  Serial.print("Max: ");
  for (uint8_t i = 0; i < SensorCount; i++) {
    Serial.print(qtr.calibrationOn.maximum[i]);
    Serial.print(' ');
  }
  Serial.println("\n");
  delay(1000);
}

void loop() {
  uint16_t position = qtr.readLineBlack(sensorValues);
  
  // Вывод: 8 сенсоров + позиция линии
  for (uint8_t i = 0; i < SensorCount; i++) {
    Serial.print(sensorValues[i]);
    Serial.print('\t');
  }
  Serial.println(position);
  
  // === Простая логика следования ===
  if (position < 3500) {
    turnLeft(90, 30);
  } else if (position > 3500) {
    turnRight(90, 30);
  } else {
    forward(90);
  }
  
  delay(250);
}