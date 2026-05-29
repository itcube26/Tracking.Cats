#define PWMA 6
#define PWMB 12

#define AIN1 7
#define AIN2 8
#define STBY 9
#define BIN1 10
#define BIN2 11



int motorSpeed = 100; //  скорость мотора

void setup(){

    Serial.begin(115200);

    pinMode(PWMA, OUTPUT);  
    pinMode(PWMB, OUTPUT);  
    pinMode(AIN1, OUTPUT);  
    pinMode(AIN2, OUTPUT);  
    pinMode(BIN1, OUTPUT);  
    pinMode(BIN2, OUTPUT);  
    pinMode(STBY, OUTPUT);  

    digitalWrite(STBY, HIGH); 
    Serial.println("Start");
    
    digitalWrite(13, HIGH);  
}

void loop()
{
    digitalWrite(13, HIGH);
    Serial.println("Tuda");

     digitalWrite(AIN1, LOW);      // крутим моторы в одну сторону
     digitalWrite(AIN2, HIGH);    
     digitalWrite(BIN1, LOW);  
     digitalWrite(BIN2, HIGH); 

     analogWrite(PWMA, motorSpeed);  
     analogWrite(PWMB, motorSpeed); 
     delay(1000);

    Serial.println("Suda");
    digitalWrite(13, LOW); 
    
     digitalWrite(AIN1, HIGH);  
     digitalWrite(AIN2, LOW); 
     digitalWrite(BIN1, HIGH);    // крутим моторы в противоположную сторону
     digitalWrite(BIN2, LOW);
     
     analogWrite(PWMA, motorSpeed); 
     analogWrite(PWMB, motorSpeed);  
     delay(1000);
     
    
      
} 