#include <Servo.h>
#include <SoftwareSerial.h>

/*
  SLAVE BOARD
  - Controls RIGHT hand only
  - Receives simple commands from MASTER

  COMMANDS RECEIVED:
    'P' = show paper
    'R' = show rock
    'S' = show scissors
    'C' = cover hand
    'U' = uncover hand

  WIRING:
    Master pin 7 -> Slave pin 8
    Common GND required
*/

// =========================
// RIGHT HAND HARDWARE
// =========================
const int R_MOTOR_A = 2;   // thumb + ring + pinky
const int R_MOTOR_B = 3;   // index + middle
const int RIGHT_COVER_SERVO_PIN = 9;

// =========================
// COMMUNICATION FROM MASTER
// =========================
const int COMM_RX_FROM_MASTER = 8; // wire from master TX pin 7
const int COMM_TX_UNUSED = 10;     // not used, but SoftwareSerial needs a pin
SoftwareSerial masterSerial(COMM_RX_FROM_MASTER, COMM_TX_UNUSED);

// =========================
// SERVO
// =========================
Servo rightCoverServo;

// =========================
// SERVO ANGLES - TUNE THESE
// =========================
const int RIGHT_COVER_OPEN_ANGLE   = 180;
const int RIGHT_COVER_CLOSED_ANGLE = 90;

// =========================
// MOTOR TIMINGS - TUNE THESE
// =========================
const unsigned long PULL_TIME_ROCK_MS     = 600;
const unsigned long PULL_TIME_SCISSORS_MS = 500;
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
void pullMotor(int pin, unsigned long durationMs);

void uncoverRightHand();
void coverRightHand();

void setup() {
  masterSerial.begin(9600);

  pinMode(R_MOTOR_A, OUTPUT);
  pinMode(R_MOTOR_B, OUTPUT);

  digitalWrite(R_MOTOR_A, LOW);
  digitalWrite(R_MOTOR_B, LOW);

  rightCoverServo.attach(RIGHT_COVER_SERVO_PIN);

  uncoverRightHand();
  releaseRightHand();
}

void loop() {
  if (masterSerial.available()) {
    char cmd = masterSerial.read();

    switch (cmd) {
      case 'P':
        uncoverRightHand();
        showGestureRight(PAPER);
        break;

      case 'R':
        uncoverRightHand();
        showGestureRight(ROCK);
        break;

      case 'S':
        uncoverRightHand();
        showGestureRight(SCISSORS);
        break;

      case 'C':
        coverRightHand();
        break;

      case 'U':
        uncoverRightHand();
        break;

      default:
        // ignore unknown commands
        break;
    }
  }
}

// =========================
// RIGHT HAND CONTROL
// =========================
void showGestureRight(char gesture) {
  releaseRightHand();
  delay(100);

  if (gesture == PAPER) {
    return;
  }
  else if (gesture == ROCK) {
    pullMotor(R_MOTOR_A, PULL_TIME_ROCK_MS);
    pullMotor(R_MOTOR_B, PULL_TIME_ROCK_MS);
  }
  else if (gesture == SCISSORS) {
    // close thumb/ring/pinky, leave index/middle open
    pullMotor(R_MOTOR_A, PULL_TIME_SCISSORS_MS);
  }
}

void releaseRightHand() {
  digitalWrite(R_MOTOR_A, LOW);
  digitalWrite(R_MOTOR_B, LOW);
}

void pullMotor(int pin, unsigned long durationMs) {
  digitalWrite(pin, HIGH);
  delay(durationMs);
  digitalWrite(pin, LOW);
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