const int buttonPin = 3;          // Пин кнопки
const int ledPin = LED_BUILTIN;   // Встроенный светодиод (на Nano обычно 13)

bool lastButtonState = HIGH;      // Предыдущее сырое состояние
bool currentButtonState = HIGH;   // Текущее стабильное состояние
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // Задержка подавления дребезга (мс)

bool ledState = false;

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); // Включаем внутреннюю подтяжку к +5В
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  
  Serial.begin(9600);
  Serial.println("Инициализация завершена. Нажмите кнопку...");
}

void loop() {
  int reading = digitalRead(buttonPin);

  // Если состояние изменилось, сбрасываем таймер дребезга
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Если прошло достаточно времени после последнего изменения
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Если состояние изменилось и стабилизировалось
    if (reading != currentButtonState) {
      currentButtonState = reading;

      // Кнопка нажата (замкнута на GND → LOW)
      if (currentButtonState == LOW) {
        ledState = !ledState;            // Инвертируем состояние светодиода
        digitalWrite(ledPin, ledState);  // Применяем
        Serial.println("Кнопка нажата!");
      }
    }
  }
  lastButtonState = reading;
}