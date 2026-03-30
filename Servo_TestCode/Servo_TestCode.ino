#include <ESP32Servo.h>

Servo testServo;

// Change this to your servo signal pin
const int SERVO_PIN = 4;

void setup() {
  Serial.begin(115200);

  testServo.attach(SERVO_PIN);

  Serial.println("Servo test starting...");
}

void loop() {
  Serial.println("Moving to 0 degrees");
  testServo.write(0);
  delay(2000);

  Serial.println("Moving to 90 degrees");
  testServo.write(90);
  delay(2000);

  Serial.println("Moving to 180 degrees");
  testServo.write(180);
  delay(2000);
}