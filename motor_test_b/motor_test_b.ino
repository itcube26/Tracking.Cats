#define PWMA 6
#define PWMB 5
#define AIN1 7
#define AIN2 8
#define BIN1 10
#define BIN2 11
#define STBY 9

void setup() {
  Serial.begin(115200);
  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, 1);
  
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(PWMA, OUTPUT); pinMode(PWMB, OUTPUT);
  
  Serial.println("Тест драйвера без моторов");
}

void loop() {
  // Тест канала Б
  Serial.println("B: FORWARD");
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, 100);
  delay(1000);
  
  Serial.println("B: BACKWARD");
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMB, 100);
  delay(1000);
  
   Serial.println("B: FORWARD");
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, 100);
  delay(1000);
  
  Serial.println("B: BACKWARD");
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  analogWrite(PWMA, 100);
  delay(1000);
  
}