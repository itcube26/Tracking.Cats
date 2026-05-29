#define PWMA 6
#define PWMB 5
#define AIN1 7
#define AIN2 8
#define BIN1 10
#define BIN2 11
#define STBY 9

#include <QTRSensors.h>


#define Kp 0.01 // experiment to determine this, start by something small that just makes your bot follow the line at a slow speed
#define Kd 0.008 // experiment to determine this, slowly increase the speeds and adjust this value. ( Note: Kp < Kd) 
#define rightMaxSpeed 150 // max speed of the robot
#define leftMaxSpeed 150 // max speed of the robot
#define rightBaseSpeed 50 // this is the speed at which the motors should spin when the robot is perfectly on the line
#define leftBaseSpeed 50  // this is the speed at which the motors should spin when the robot is perfectly on the line
#define NUM_SENSORS  6     // number of sensors used
#define TIMEOUT       2500  // waits for 2500 us for sensor outputs to go low
#define EMITTER_PIN   2     // emitter is controlled by digital pin 2


QTRSensors qtr;

const uint8_t SensorCount = 8;
uint16_t sensorValues[SensorCount];




void setup(){

  pinMode(STBY, OUTPUT);
  digitalWrite(STBY, HIGH);

  pinMode(PWMA, OUTPUT);  
  pinMode(PWMB, OUTPUT);  
  pinMode(AIN1, OUTPUT);  
  pinMode(AIN2, OUTPUT);  
  pinMode(BIN1, OUTPUT);  
  pinMode(BIN2, OUTPUT); 

  // configure the sensors
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A7, A6, A5, A4, A3, A2, A1, A0}, SensorCount);
  qtr.setEmitterPin(2);

    
  delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // turn on Arduino's LED to indicate we are in calibration mode

  // analogRead() takes about 0.1 ms on an AVR.
  // 0.1 ms per sensor * 4 samples per sensor read (default) * 6 sensors
  // * 10 reads per calibrate() call = ~24 ms per calibrate() call.
  // Call calibrate() 400 times to make calibration take about 10 seconds.
  for (uint16_t i = 0; i < 400; i++)
  {
    qtr.calibrate();
  }
  digitalWrite(LED_BUILTIN, LOW); // turn off Arduino's LED to indicate we are through with calibration

  // print the calibration minimum values measured when emitters were on
  Serial.begin(9600);
  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(qtr.calibrationOn.minimum[i]);
    Serial.print(' ');
  }
  Serial.println();
  // print the calibration maximum values measured when emitters were on
  for (uint8_t i = 0; i < SensorCount; i++)
  {
    Serial.print(qtr.calibrationOn.maximum[i]);
    Serial.print(' ');
  }
  Serial.println();
  Serial.println();
  delay(1000);
  } 

int lastError = 0;

void loop()
{

  digitalWrite(AIN1, LOW);      // крутим моторы в одну сторону
  digitalWrite(AIN2, HIGH);    
  digitalWrite(BIN1, LOW);  
  digitalWrite(BIN2, HIGH); 

      

  uint16_t position = qtr.readLineBlack(sensorValues);

  int error = position - 3500;

  int motorSpeed = Kp * error + Kd * (error - lastError);

  Serial.print("error : ");
  Serial.println(error);
  Serial.print("error - lastError: ");
  Serial.println(error - lastError);
  lastError = error;


  int rightMotorSpeed = rightBaseSpeed + motorSpeed;
  int leftMotorSpeed = leftBaseSpeed - motorSpeed;
  
    if (rightMotorSpeed > rightMaxSpeed ) rightMotorSpeed = rightMaxSpeed; // prevent the motor from going beyond max speed
  if (leftMotorSpeed > leftMaxSpeed ) leftMotorSpeed = leftMaxSpeed; // prevent the motor from going beyond max speed
  if (rightMotorSpeed < 0) rightMotorSpeed = 0; // keep the motor speed positive
  if (leftMotorSpeed < 0) leftMotorSpeed = 0; // keep the motor speed positive
  
   {
  // move forward with appropriate speeds


  analogWrite(PWMA, rightMotorSpeed); 
  analogWrite(PWMB, leftMotorSpeed); 

  Serial.print("position: ");
  Serial.println(position);
  

  Serial.print("L: ");
  Serial.print(leftMotorSpeed);
  Serial.println();
  Serial.print("R: ");
  Serial.print(rightMotorSpeed);
  Serial.println();
  
  }
}