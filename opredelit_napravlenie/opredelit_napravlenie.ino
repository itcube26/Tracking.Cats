#define PWMA 6
#define PWMB 5
#define AIN1 7
#define AIN2 8
#define BIN1 10
#define BIN2 11
#define STBY 9

// === НАСТРОЙКИ СКОРОСТИ ===
#define SPEED_DEFAULT 100

// === ФУНКЦИИ ДВИЖЕНИЯ ===

// 🛑 Остановка
void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
}

// ⬆️ Вперёд
void forward(uint8_t speed = SPEED_DEFAULT) {
  // Мотор A
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, speed);
  // Мотор B
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, speed);
}

// ⬇️ Назад
void backward(uint8_t speed = SPEED_DEFAULT) {
  // Мотор A
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speed);
  // Мотор B
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMB, speed);
}

// ➡️ Вправо (поворот на месте)
void right(uint8_t speed = SPEED_DEFAULT) {
  // Мотор A — вперёд
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, speed);
  // Мотор B — назад
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMB, speed);
}

// ⬅️ Влево (поворот на месте)
void left(uint8_t speed = SPEED_DEFAULT) {
  // Мотор A — назад
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speed);
  // Мотор B — вперёд
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, speed);
}

// === SETUP ===
void setup() {
  Serial.begin(115200);
  
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);  // Включаем драйвер
  
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  
  stopMotors();  // На всякий случай останавливаем моторы при старте
  
  Serial.println("✅ Драйвер готов. Функции: forward(), backward(), left(), right()");
}

// === LOOP ===
void loop() {
  right();  // 🚀 Едем вперёд со скоростью по умолчанию
  
  // Если нужно протестировать все направления по очереди — раскомментируйте ниже:
  /*
  forward(100);  delay(1000);  stopMotors();  delay(500);
  backward(100); delay(1000);  stopMotors();  delay(500);
  left(100);     delay(1000);  stopMotors();  delay(500);
  right(100);    delay(1000);  stopMotors();  delay(500);
  */
}