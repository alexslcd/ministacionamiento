#include <Servo.h>

const int trigPin=3, echoPin=2;
const int ledRojo=4, ledVerde=5;
Servo miServo;
const float UMBRAL_CM=15.0;
unsigned long tSend=0;

void setup(){
  Serial.begin(9600);
  pinMode(trigPin,OUTPUT); pinMode(echoPin,INPUT);
  pinMode(ledRojo,OUTPUT); pinMode(ledVerde,OUTPUT);
  miServo.attach(6); miServo.write(90); // abierta
  Serial.println("Arduino listo...");
}

float distCM(){
  digitalWrite(trigPin,LOW); delayMicroseconds(2);
  digitalWrite(trigPin,HIGH); delayMicroseconds(10);
  digitalWrite(trigPin,LOW);
  unsigned long d=pulseIn(echoPin,HIGH,30000);
  if(d==0) return 999.0;
  return (d*0.0343f)/2.0f;
}

void setPuerta(bool cerrada){
  if(cerrada){ miServo.write(0);  digitalWrite(ledRojo,HIGH); digitalWrite(ledVerde,LOW); }
  else       { miServo.write(90); digitalWrite(ledRojo,LOW);  digitalWrite(ledVerde,HIGH); }
}

void loop(){
  if(Serial.available()){
    String cmd=Serial.readStringUntil('\n'); cmd.trim();
    if(cmd=="MOTOR:ON")  setPuerta(true);
    if(cmd=="MOTOR:OFF") setPuerta(false);
  }
  bool ocupada = (distCM()<UMBRAL_CM);
  setPuerta(ocupada);
  if(millis()-tSend>=300){
    tSend=millis();
    Serial.print("PROX:"); Serial.println(ocupada?1:0);
  }
}
