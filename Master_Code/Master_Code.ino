#include <Adafruit_NeoPixel.h> // For controlling the smart LED
#include <Wire.h> // For I2C communication (used by some sensors)
#include <ESP32Servo.h>



/*
  MASTER BOARD
  - Controls LEFT hand directly
  - Runs RPS Minus One strategy
  - Talks to SLAVE board for RIGHT hand control

  SERIAL MONITOR COMMANDS:
    n      -> start a new round
    PR     -> opponent left=P, opponent right=R
    RS     -> opponent left=R, opponent right=S
    etc.

  BOARD-TO-BOARD COMMUNICATION:
    Master TX pin 7 -> Slave RX pin 8
    Common GND required
*/

// =========================
// MASTER LOCAL HARDWARE
// =========================

// Motor A = thumb + ring + pinky
const int L_MOTOR_A_IN1 = 26;
const int L_MOTOR_A_IN2 = 16;

// Motor B = index + middle
const int L_MOTOR_B_IN1 = 27;
const int L_MOTOR_B_IN2 = 17;

// Cover servo
const int LEFT_COVER_SERVO_PIN = 17;

// =========================
// OPTIONAL ENCODER PINS
// =========================
// These are not used yet in the code below.
// They are here so you can set them now for future use.
const int L_MOTOR_A_ENC1 = 35;
const int L_MOTOR_A_ENC2 = 32;
const int L_MOTOR_B_ENC1 = 33;
const int L_MOTOR_B_ENC2 = 25;

// =========================
// COMMUNICATION TO SLAVE
// =========================
const int COMM_RX_UNUSED = 16;   // RX pin on master
const int COMM_TX_TO_SLAVE = 17; // TX pin on master
HardwareSerial slaveSerial(2);
// =========================
// SERVO
// =========================
Servo leftCoverServo;

// =========================
// SERVO ANGLES - TUNE THESE
// =========================
const int LEFT_COVER_OPEN_ANGLE   = 0;
const int LEFT_COVER_CLOSED_ANGLE = 90;

// =========================
// MOTOR TIMINGS - TUNE THESE
// =========================
const unsigned long PULL_TIME_ROCK_MS     = 600;
const unsigned long PULL_TIME_SCISSORS_MS = 500;
const unsigned long RESET_WAIT_MS         = 700;
const unsigned long COVER_WAIT_MS         = 500;

// =========================
// GESTURES
// =========================
const char ROCK     = 'R';
const char PAPER    = 'P';
const char SCISSORS = 'S';

// =========================
// ROUND STATE
// =========================
char myLeftGesture  = PAPER;
char myRightGesture = PAPER;
bool roundActive = false;

// =========================
// FUNCTION DECLARATIONS
// =========================
void sendCommandToSlave(char cmd);

void showGestureLeft(char gesture);
void releaseLeftHand();

void stopMotor(int in1, int in2);
void pullMotorForward(int in1, int in2, unsigned long durationMs);
void pullMotorReverse(int in1, int in2, unsigned long durationMs);

void uncoverLeftHand();
void coverLeftHand();

int rpsResult(char a, char b);
void chooseStage1Pair(char &leftG, char &rightG);
char chooseFinalHand(char myA, char myB, char oppA, char oppB);

bool isIdenticalPair(char a, char b);
bool samePairIgnoringOrder(char a1, char a2, char b1, char b2);
char dominantHandOfPair(char a, char b);
char getSharedHand(char a1, char a2, char b1, char b2);
char getOtherHand(char pairA, char pairB, char oneOfThem);

void runNewRound();
void resolveStage2(char oppLeft, char oppRight);

void setup() {
  Serial.begin(115200);
  slaveSerial.begin(115200, SERIAL_8N1, COMM_RX_UNUSED, COMM_TX_TO_SLAVE);
  randomSeed(analogRead(A0));

  // Motor driver pins
  pinMode(L_MOTOR_A_IN1, OUTPUT);
  pinMode(L_MOTOR_A_IN2, OUTPUT);
  pinMode(L_MOTOR_B_IN1, OUTPUT);
  pinMode(L_MOTOR_B_IN2, OUTPUT);

  stopMotor(L_MOTOR_A_IN1, L_MOTOR_A_IN2);
  stopMotor(L_MOTOR_B_IN1, L_MOTOR_B_IN2);

  // Optional encoder pins
  pinMode(L_MOTOR_A_ENC1, INPUT);
  pinMode(L_MOTOR_A_ENC2, INPUT);
  pinMode(L_MOTOR_B_ENC1, INPUT);
  pinMode(L_MOTOR_B_ENC2, INPUT);

  leftCoverServo.attach(LEFT_COVER_SERVO_PIN);

  uncoverLeftHand();
  releaseLeftHand();

  // Also tell slave to uncover and reset
  delay(300);
  sendCommandToSlave('U');  // uncover
  sendCommandToSlave('P');  // paper/reset

  Serial.println("=== MASTER READY ===");
  Serial.println("Type:");
  Serial.println("  n   -> start new round");
  Serial.println("  PR  -> enter opponent pair");
  Serial.println();
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();

    if (input == "N") {
      runNewRound();
    }
    else if (input.length() == 2 && roundActive) {
      char oppLeft = input.charAt(0);
      char oppRight = input.charAt(1);

      bool validLeft  = (oppLeft == ROCK || oppLeft == PAPER || oppLeft == SCISSORS);
      bool validRight = (oppRight == ROCK || oppRight == PAPER || oppRight == SCISSORS);

      if (validLeft && validRight) {
        resolveStage2(oppLeft, oppRight);
      } else {
        Serial.println("Invalid input. Use 2 letters from R/P/S, like PR.");
      }
    }
    else {
      Serial.println("Unknown command. Type n to start a round.");
    }
  }
}

// =========================
// MASTER -> SLAVE COMMANDS
// =========================
void sendCommandToSlave(char cmd) {
  slaveSerial.write(cmd);
  delay(50);
}

/*
  Slave command meanings:
  'P' = show paper
  'R' = show rock
  'S' = show scissors
  'C' = cover hand
  'U' = uncover hand
*/

// =========================
// ROUND CONTROL
// =========================
void runNewRound() {
  Serial.println();
  Serial.println("=== NEW ROUND ===");

  // Reset both hands to visible + paper
  uncoverLeftHand();
  releaseLeftHand();

  sendCommandToSlave('U');
  sendCommandToSlave('P');

  delay(RESET_WAIT_MS);

  chooseStage1Pair(myLeftGesture, myRightGesture);

  Serial.print("My stage 1 pair: ");
  Serial.print(myLeftGesture);
  Serial.println(myRightGesture);

  showGestureLeft(myLeftGesture);
  sendCommandToSlave(myRightGesture);

  roundActive = true;

  Serial.println("Enter opponent pair now (example: PR)");
}

void resolveStage2(char oppLeft, char oppRight) {
  Serial.print("Opponent pair: ");
  Serial.print(oppLeft);
  Serial.println(oppRight);

  char keepGesture = chooseFinalHand(myLeftGesture, myRightGesture, oppLeft, oppRight);

  Serial.print("Chosen final gesture to keep: ");
  Serial.println(keepGesture);

  bool keepLeft = false;
  bool keepRight = false;

  if (myLeftGesture == keepGesture)  keepLeft = true;
  if (myRightGesture == keepGesture) keepRight = true;

  if (keepLeft && !keepRight) {
    // Keep left visible, cover right
    uncoverLeftHand();
    sendCommandToSlave('C');
    Serial.println("Keeping LEFT, covering RIGHT.");
  }
  else if (!keepLeft && keepRight) {
    // Cover left, keep right visible
    coverLeftHand();
    sendCommandToSlave('U');
    Serial.println("Keeping RIGHT, covering LEFT.");
  }
  else {
    // Fallback
    uncoverLeftHand();
    sendCommandToSlave('C');
    Serial.println("Ambiguous case -> default KEEP LEFT.");
  }

  roundActive = false;
  Serial.println("=== ROUND COMPLETE ===");
  Serial.println();
}

// =========================
// LEFT HAND CONTROL
// =========================
void showGestureLeft(char gesture) {
  releaseLeftHand();
  delay(100);

  if (gesture == PAPER) {
    return;
  }
  else if (gesture == ROCK) {
    // Pull both motor groups
    pullMotorForward(L_MOTOR_A_IN1, L_MOTOR_A_IN2, PULL_TIME_ROCK_MS, "Motor A");
    pullMotorForward(L_MOTOR_B_IN1, L_MOTOR_B_IN2, PULL_TIME_ROCK_MS, "Motor B");
  }
  else if (gesture == SCISSORS) {
    // Close thumb/ring/pinky, leave index/middle open
    pullMotorForward(L_MOTOR_A_IN1, L_MOTOR_A_IN2, PULL_TIME_ROCK_MS, "Motor A");
  }
}

void releaseLeftHand() {
  stopMotor(L_MOTOR_A_IN1, L_MOTOR_A_IN2);
  stopMotor(L_MOTOR_B_IN1, L_MOTOR_B_IN2);
}

// =========================
// MOTOR DRIVER CONTROL
// =========================
void stopMotor(int in1, int in2) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void pullMotorForward(int in1, int in2, unsigned long durationMs, const char* name) {
  Serial.print(name);
  Serial.println(" START");

  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);

  delay(durationMs);

  Serial.print(name);
  Serial.println(" STOP");

  stopMotor(in1, in2);

  Serial.print(name);
  Serial.println(" STOP COMMAND SENT");
}

void pullMotorReverse(int in1, int in2, unsigned long durationMs) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  delay(durationMs);
  stopMotor(in1, in2);
}

// =========================
// LEFT COVER CONTROL
// =========================
void uncoverLeftHand() {
  leftCoverServo.write(LEFT_COVER_OPEN_ANGLE);
  delay(COVER_WAIT_MS);
}

void coverLeftHand() {
  leftCoverServo.write(LEFT_COVER_CLOSED_ANGLE);
  delay(COVER_WAIT_MS);
}

// =========================
// GAME LOGIC
// =========================
int rpsResult(char a, char b) {
  if (a == b) return 0;

  if ((a == ROCK     && b == SCISSORS) ||
      (a == PAPER    && b == ROCK)     ||
      (a == SCISSORS && b == PAPER)) {
    return 1;
  }

  return -1;
}

bool isIdenticalPair(char a, char b) {
  return (a == b);
}

bool samePairIgnoringOrder(char a1, char a2, char b1, char b2) {
  return ((a1 == b1 && a2 == b2) || (a1 == b2 && a2 == b1));
}

char dominantHandOfPair(char a, char b) {
  if (rpsResult(a, b) == 1) return a;
  return b;
}

char getSharedHand(char a1, char a2, char b1, char b2) {
  if (a1 == b1 || a1 == b2) return a1;
  return a2;
}

char getOtherHand(char pairA, char pairB, char oneOfThem) {
  if (pairA == oneOfThem) return pairB;
  return pairA;
}

void chooseStage1Pair(char &leftG, char &rightG) {
  int choice = random(0, 3);

  switch (choice) {
    case 0:
      leftG = PAPER;
      rightG = ROCK;
      break;
    case 1:
      leftG = PAPER;
      rightG = SCISSORS;
      break;
    case 2:
      leftG = ROCK;
      rightG = SCISSORS;
      break;
  }
}

char chooseFinalHand(char myA, char myB, char oppA, char oppB) {
  // Case 1: opponent used identical hands
  if (isIdenticalPair(oppA, oppB)) {
    int scoreA = rpsResult(myA, oppA);
    int scoreB = rpsResult(myB, oppA);

    if (scoreA > scoreB) return myA;
    if (scoreB > scoreA) return myB;

    return myA; // fallback
  }

  // Case 2: same pair as us
  if (samePairIgnoringOrder(myA, myB, oppA, oppB)) {
    return dominantHandOfPair(myA, myB);
  }

  // Case 3: different non-identical viable pair
  char shared = getSharedHand(myA, myB, oppA, oppB);
  char other  = getOtherHand(myA, myB, shared);

  int roll = random(0, 3); // 0,1,2
  if (roll < 2) {
    return shared; // 2/3
  } else {
    return other;  // 1/3
  }
}