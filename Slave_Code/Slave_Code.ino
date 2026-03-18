#include <Adafruit_NeoPixel.h> // For controlling the smart LED
#include <Wire.h> // For I2C communication (used by some sensors)
#include <ESP32Servo.h>

// =========================
// RIGHT HAND HARDWARE
// =========================

// Motor A = thumb + ring + pinky
const int R_MOTOR_A_IN1 = 26;
const int R_MOTOR_A_IN2 = 16;

// Motor B = index + middle
const int R_MOTOR_B_IN1 = 27;
const int R_MOTOR_B_IN2 = 17;

// Cover servo
const int RIGHT_COVER_SERVO_PIN = 4;

// =========================
// UART FROM MASTER
// =========================
const int SLAVE_RX_FROM_MASTER = 16;   // change if needed
const int SLAVE_TX_UNUSED      = 18;   // change if needed
HardwareSerial masterSerial(2);

// =========================
// SERVO
// =========================
Servo rightCoverServo;

// =========================
// SERVO ANGLES
// =========================
const int RIGHT_COVER_OPEN_ANGLE   = 0;
const int RIGHT_COVER_CLOSED_ANGLE = 90;

// =========================
// TIMINGS
// =========================
const unsigned long PULL_TIME_ROCK_MS     = 5000;
const unsigned long PULL_TIME_SCISSORS_MS = 5000;
const unsigned long RESET_TIME_MS         = 5000;
const unsigned long COVER_WAIT_MS         = 500;

// =========================
// GESTURES
// =========================
const char ROCK     = 'R';
const char PAPER    = 'P';
const char SCISSORS = 'S';

// =========================
// FUNCTION DECLARATIONS
// =========================
void showGestureRight(char gesture);
void releaseRightHand();

void motorAForward();
void motorAReverse();
void motorBForward();
void motorBReverse();

void stopMotor(int in1, int in2);
void stopRightHand();

void uncoverRightHand();
void coverRightHand();

void setup() {
  Serial.begin(115200);
  masterSerial.begin(115200, SERIAL_8N1, SLAVE_RX_FROM_MASTER, SLAVE_TX_UNUSED);

  pinMode(R_MOTOR_A_IN1, OUTPUT);
  pinMode(R_MOTOR_A_IN2, OUTPUT);
  pinMode(R_MOTOR_B_IN1, OUTPUT);
  pinMode(R_MOTOR_B_IN2, OUTPUT);

  stopRightHand();

  rightCoverServo.attach(RIGHT_COVER_SERVO_PIN);

  uncoverRightHand();
  releaseRightHand();

  Serial.println("=== SLAVE READY ===");
}

void loop() {
  if (masterSerial.available()) {
    char cmd = masterSerial.read();

    Serial.print("Received command: ");
    Serial.println(cmd);

    if (cmd == 'P') {
      showGestureRight(PAPER);
    }
    else if (cmd == 'R') {
      showGestureRight(ROCK);
    }
    else if (cmd == 'S') {
      showGestureRight(SCISSORS);
    }
    else if (cmd == 'C') {
      coverRightHand();
    }
    else if (cmd == 'U') {
      uncoverRightHand();
    }
  }
}

// =========================
// RIGHT HAND CONTROL
// =========================
void showGestureRight(char gesture) {
  if (gesture == PAPER) {
    Serial.println("Showing PAPER");
    releaseRightHand();
    return;
  }
  else if (gesture == ROCK) {
    Serial.println("Showing ROCK");

    motorAForward();
    motorBForward();

    delay(PULL_TIME_ROCK_MS);
    stopRightHand();
  }
  else if (gesture == SCISSORS) {
    Serial.println("Showing SCISSORS");

    releaseRightHand();
    delay(100);

    motorAForward();

    delay(PULL_TIME_SCISSORS_MS);
    stopRightHand();
  }
}

void releaseRightHand() {
  Serial.println("Resetting right hand to PAPER...");

  motorAReverse();
  motorBReverse();

  delay(RESET_TIME_MS);

  stopRightHand();
  delay(150);
}

// =========================
// MOTOR DRIVER CONTROL
// =========================
// These directions may need to be changed to match the actual wiring
void motorAForward() {
  digitalWrite(R_MOTOR_A_IN1, LOW);
  digitalWrite(R_MOTOR_A_IN2, HIGH);
}

void motorAReverse() {
  digitalWrite(R_MOTOR_A_IN1, HIGH);
  digitalWrite(R_MOTOR_A_IN2, LOW);
}

void motorBForward() {
  digitalWrite(R_MOTOR_B_IN1, HIGH);
  digitalWrite(R_MOTOR_B_IN2, LOW);
}

void motorBReverse() {
  digitalWrite(R_MOTOR_B_IN1, LOW);
  digitalWrite(R_MOTOR_B_IN2, HIGH);
}

void stopMotor(int in1, int in2) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void stopRightHand() {
  stopMotor(R_MOTOR_A_IN1, R_MOTOR_A_IN2);
  stopMotor(R_MOTOR_B_IN1, R_MOTOR_B_IN2);
}

// =========================
// RIGHT COVER CONTROL
// =========================
void uncoverRightHand() {
  rightCoverServo.write(RIGHT_COVER_OPEN_ANGLE);
  delay(COVER_WAIT_MS);
}

void coverRightHand() {
  rightCoverServo.write(RIGHT_COVER_CLOSED_ANGLE);
  delay(COVER_WAIT_MS);
}