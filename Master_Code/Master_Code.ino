#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

/*
  FULL MASTER BOARD
  -----------------
  - Controls LEFT hand locally
  - Runs RPS Minus One strategy
  - Sends RIGHT hand commands wirelessly via ESP-NOW

  SERIAL MONITOR COMMANDS:
    n      -> start new round
    PR     -> enter opponent left=P, right=R
    RS     -> enter opponent left=R, right=S
    m      -> reset MASTER hand to paper
    s      -> reset SLAVE hand to paper
    c      -> reset SLAVE hand from scissors to paper
    v      -> reset BOTH servos to open
*/

// =====================================================
// CONFIG
// =====================================================
uint8_t SLAVE_MAC[] = {0xEC, 0x64, 0xC9, 0x5E, 0x80, 0x4C};

// =====================================================
// LEFT HAND HARDWARE
// =====================================================
// Motor A = index + middle
const int L_MOTOR_A_IN1 = 17;
const int L_MOTOR_A_IN2 = 16;

// Motor B = thumb + ring + pinky
const int L_MOTOR_B_IN1 = 27;
const int L_MOTOR_B_IN2 = 26;

// Cover servo
const int LEFT_COVER_SERVO_PIN = 4;

// =====================================================
// SERVO
// =====================================================
Servo leftCoverServo;
bool leftServoAttached = false;

// =====================================================
// TUNING
// =====================================================
const int LEFT_COVER_OPEN_ANGLE   = 90;
const int LEFT_COVER_CLOSED_ANGLE = 0;

// separate rock timings for left hand
const unsigned long PULL_TIME_ROCK_A_MS   = 5000;
const unsigned long PULL_TIME_ROCK_B_MS   = 5000;

const unsigned long PULL_TIME_SCISSORS_MS = 5000;

// separate reset timings for left hand
const unsigned long RESET_TIME_A_MS       = 2500;
const unsigned long RESET_TIME_B_MS       = 2500;

const unsigned long COVER_WAIT_MS         = 500;
const unsigned long BETWEEN_RIGHT_CMDS_MS = 150;
const unsigned long RESULT_VIEW_MS        = 1500;

// =====================================================
// GAME SYMBOLS
// =====================================================
const char ROCK     = 'R';
const char PAPER    = 'P';
const char SCISSORS = 'S';

// =====================================================
// COMMAND PROTOCOL
// =====================================================
enum CommandType : uint8_t {
  CMD_NONE            = 0,
  CMD_SET_GESTURE     = 1,
  CMD_SET_COVER       = 2,
  CMD_RESET_HAND      = 3,
  CMD_RESET_SCISSORS  = 4
};

enum CoverState : uint8_t {
  COVER_UNCHANGED = 0,
  COVER_OPEN      = 1,
  COVER_CLOSED    = 2
};

struct RightHandPacket {
  uint32_t seq;
  uint8_t command;
  uint8_t gesture;
  uint8_t cover;
};

uint32_t txSequence = 1;
volatile bool lastSendOk = false;

// =====================================================
// ROUND STATE
// =====================================================
char myLeftGesture  = PAPER;
char myRightGesture = PAPER;
bool roundActive = false;
int lastChoice = -1;

// =====================================================
// FUNCTION DECLARATIONS
// =====================================================
bool initEspNow();
bool addSlavePeer();
bool sendRightPacket(uint8_t command, uint8_t gesture = 0, uint8_t cover = COVER_UNCHANGED);
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void setLeftGesture(char gesture);
void resetLeftHandToPaper();
void setLeftCover(bool covered);

void motorAForward();
void motorAReverse();
void motorBForward();
void motorBReverse();
void stopMotor(int in1, int in2);
void stopLeftHand();

void sendRightGesture(char gesture);
void sendRightCover(bool covered);
void sendRightReset();
void sendRightScissorsReset();

int rpsResult(char a, char b);
bool isIdenticalPair(char a, char b);
bool samePairIgnoringOrder(char a1, char a2, char b1, char b2);
char dominantHandOfPair(char a, char b);
char getSharedHand(char a1, char a2, char b1, char b2);
char getOtherHand(char pairA, char pairB, char oneOfThem);

void chooseStage1Pair(char &leftG, char &rightG);
char chooseFinalHand(char myA, char myB, char oppA, char oppB);

void runNewRound();
void resolveStage2(char oppLeft, char oppRight);

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  randomSeed(esp_random());

  pinMode(L_MOTOR_A_IN1, OUTPUT);
  pinMode(L_MOTOR_A_IN2, OUTPUT);
  pinMode(L_MOTOR_B_IN1, OUTPUT);
  pinMode(L_MOTOR_B_IN2, OUTPUT);

  stopLeftHand();

  // only attach long enough to move, then detach to reduce shaking
  leftCoverServo.attach(LEFT_COVER_SERVO_PIN);
  leftServoAttached = true;
  delay(300);
  leftCoverServo.write(LEFT_COVER_OPEN_ANGLE);
  delay(1000);
  leftCoverServo.detach();
  leftServoAttached = false;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println();
  Serial.println("=== MASTER BOOT ===");
  Serial.print("Master MAC: ");
  Serial.println(WiFi.macAddress());

  if (!initEspNow()) {
    Serial.println("ESP-NOW init failed.");
    while (true) delay(1000);
  }

  if (!addSlavePeer()) {
    Serial.println("Failed to add slave peer.");
    while (true) delay(1000);
  }

  setLeftCover(false);
  delay(300);
  sendRightCover(false);

  Serial.println("=== MASTER READY ===");
  Serial.println("Type:");
  Serial.println("  n   -> start new round");
  Serial.println("  PR  -> enter opponent pair");
  Serial.println("  m   -> reset MASTER hand to paper");
  Serial.println("  s   -> reset SLAVE hand to paper");
  Serial.println("  c   -> reset SLAVE hand from scissors to paper");
  Serial.println("  v   -> reset BOTH servos to OPEN");
  Serial.println();
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();

    if (input == "N") {
      runNewRound();
    }
    else if (input == "M") {
      Serial.println("MANUAL RESET -> MASTER hand to PAPER");
      resetLeftHandToPaper();
    }
    else if (input == "S") {
      Serial.println("MANUAL RESET -> SLAVE hand to PAPER");
      sendRightReset();
    }
    else if (input == "C") {
      Serial.println("MANUAL RESET -> SLAVE scissors to PAPER");
      sendRightScissorsReset();
    }
    else if (input == "V") {
      Serial.println("MANUAL RESET -> BOTH servos OPEN");
      setLeftCover(false);
      sendRightCover(false);
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
      Serial.println("Unknown command.");
    }
  }
}

// =====================================================
// ESP-NOW
// =====================================================
bool initEspNow() {
  if (esp_now_init() != ESP_OK) {
    return false;
  }

  esp_now_register_send_cb(onDataSent);
  return true;
}

bool addSlavePeer() {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, SLAVE_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_is_peer_exist(SLAVE_MAC)) {
    return true;
  }

  return (esp_now_add_peer(&peerInfo) == ESP_OK);
}

bool sendRightPacket(uint8_t command, uint8_t gesture, uint8_t cover) {
  RightHandPacket pkt;
  pkt.seq = txSequence++;
  pkt.command = command;
  pkt.gesture = gesture;
  pkt.cover = cover;

  lastSendOk = false;

  esp_err_t result = esp_now_send(SLAVE_MAC, (uint8_t *)&pkt, sizeof(pkt));

  Serial.print("Sent packet seq=");
  Serial.print(pkt.seq);
  Serial.print(" cmd=");
  Serial.print(pkt.command);
  Serial.print(" gesture=");
  Serial.print((char)pkt.gesture);
  Serial.print(" cover=");
  Serial.println(pkt.cover);

  delay(BETWEEN_RIGHT_CMDS_MS);

  if (result != ESP_OK) {
    Serial.println("ESP-NOW send queue failed.");
    return false;
  }

  return true;
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendOk = (status == ESP_NOW_SEND_SUCCESS);
}

// =====================================================
// LEFT HAND CONTROL
// =====================================================
void setLeftGesture(char gesture) {
  if (gesture != ROCK && gesture != PAPER && gesture != SCISSORS) return;

  Serial.print("LEFT -> ");
  Serial.println(gesture);

  if (gesture == PAPER) {
    Serial.println("LEFT already in PAPER / no motor move");
    stopLeftHand();
    return;
  }
  else if (gesture == ROCK) {
    Serial.println("LEFT ROCK: motorAForward + motorBForward");

    motorAForward();
    motorBForward();

    unsigned long startTime = millis();
    bool motorAStopped = false;
    bool motorBStopped = false;

    while (!motorAStopped || !motorBStopped) {
      unsigned long elapsed = millis() - startTime;

      if (!motorAStopped && elapsed >= PULL_TIME_ROCK_A_MS) {
        stopMotor(L_MOTOR_A_IN1, L_MOTOR_A_IN2);
        motorAStopped = true;
      }

      if (!motorBStopped && elapsed >= PULL_TIME_ROCK_B_MS) {
        stopMotor(L_MOTOR_B_IN1, L_MOTOR_B_IN2);
        motorBStopped = true;
      }
    }

    stopLeftHand();
  }
  else if (gesture == SCISSORS) {
    Serial.println("LEFT SCISSORS: motorBForward only");
    stopMotor(L_MOTOR_A_IN1, L_MOTOR_A_IN2);
    motorBForward();
    delay(PULL_TIME_SCISSORS_MS);
    stopLeftHand();
  }
}

void resetLeftHandToPaper() {
  Serial.println("LEFT -> reset to PAPER");

  motorAReverse();
  motorBReverse();

  unsigned long startTime = millis();
  bool motorAStopped = false;
  bool motorBStopped = false;

  while (!motorAStopped || !motorBStopped) {
    unsigned long elapsed = millis() - startTime;

    if (!motorAStopped && elapsed >= RESET_TIME_A_MS) {
      stopMotor(L_MOTOR_A_IN1, L_MOTOR_A_IN2);
      motorAStopped = true;
    }

    if (!motorBStopped && elapsed >= RESET_TIME_B_MS) {
      stopMotor(L_MOTOR_B_IN1, L_MOTOR_B_IN2);
      motorBStopped = true;
    }
  }

  stopLeftHand();
}

void setLeftCover(bool covered) {
  if (!leftServoAttached) {
    leftCoverServo.attach(LEFT_COVER_SERVO_PIN);
    leftServoAttached = true;
    delay(50);
  }

  if (covered) {
    Serial.println("LEFT COVER -> CLOSE");
    leftCoverServo.write(LEFT_COVER_CLOSED_ANGLE);
  } else {
    Serial.println("LEFT COVER -> OPEN");
    leftCoverServo.write(LEFT_COVER_OPEN_ANGLE);
  }

  delay(COVER_WAIT_MS);

  leftCoverServo.detach();
  leftServoAttached = false;
}

// =====================================================
// MOTOR CONTROL
// =====================================================
void motorAForward() {
  digitalWrite(L_MOTOR_A_IN1, LOW);
  digitalWrite(L_MOTOR_A_IN2, HIGH);
}

void motorAReverse() {
  digitalWrite(L_MOTOR_A_IN1, HIGH);
  digitalWrite(L_MOTOR_A_IN2, LOW);
}

void motorBForward() {
  digitalWrite(L_MOTOR_B_IN1, LOW);
  digitalWrite(L_MOTOR_B_IN2, HIGH);
}

void motorBReverse() {
  digitalWrite(L_MOTOR_B_IN1, HIGH);
  digitalWrite(L_MOTOR_B_IN2, LOW);
}

void stopMotor(int in1, int in2) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void stopLeftHand() {
  stopMotor(L_MOTOR_A_IN1, L_MOTOR_A_IN2);
  stopMotor(L_MOTOR_B_IN1, L_MOTOR_B_IN2);
}

// =====================================================
// RIGHT HAND WIRELESS HELPERS
// =====================================================
void sendRightGesture(char gesture) {
  Serial.print("RIGHT CMD -> gesture ");
  Serial.println(gesture);
  sendRightPacket(CMD_SET_GESTURE, (uint8_t)gesture, COVER_UNCHANGED);
}

void sendRightCover(bool covered) {
  Serial.print("RIGHT CMD -> ");
  Serial.println(covered ? "cover" : "uncover");
  sendRightPacket(CMD_SET_COVER, 0, covered ? COVER_CLOSED : COVER_OPEN);
}

void sendRightReset() {
  Serial.println("RIGHT CMD -> reset");
  sendRightPacket(CMD_RESET_HAND, 0, COVER_UNCHANGED);
}

void sendRightScissorsReset() {
  Serial.println("RIGHT CMD -> scissors reset");
  sendRightPacket(CMD_RESET_SCISSORS, 0, COVER_UNCHANGED);
}

// =====================================================
// ROUND CONTROL
// =====================================================
void runNewRound() {
  Serial.println();
  Serial.println("=== NEW ROUND ===");

  setLeftCover(false);
  sendRightCover(false);

  chooseStage1Pair(myLeftGesture, myRightGesture);

  Serial.print("My stage 1 pair: ");
  Serial.print(myLeftGesture);
  Serial.println(myRightGesture);

  setLeftGesture(myLeftGesture);
  sendRightGesture(myRightGesture);

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

  bool keepLeft  = (myLeftGesture  == keepGesture);
  bool keepRight = (myRightGesture == keepGesture);

  // Only move the servo that needs to cover the hand being discarded
  if (keepLeft && !keepRight) {
    Serial.println("Keeping LEFT, covering RIGHT only.");
    setLeftCover(false);    // make sure left stays open
    sendRightCover(true);   // cover right only
  }
  else if (!keepLeft && keepRight) {
    Serial.println("Keeping RIGHT, covering LEFT only.");
    setLeftCover(true);     // cover left only
    sendRightCover(false);  // make sure right stays open
  }
  else {
    Serial.println("Ambiguous case -> default KEEP LEFT.");
    setLeftCover(false);
    sendRightCover(true);
  }

  delay(RESULT_VIEW_MS);

  // No automatic reset here
  roundActive = false;
  Serial.println("=== ROUND COMPLETE ===");
  Serial.println("Use m, s, c, or v for manual resets.");
  Serial.println();
}

// =====================================================
// GAME LOGIC
// =====================================================
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
  if (a == b) return a;
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
  int choice;
  do {
    choice = random(0, 3);
  } while (choice == lastChoice);

  lastChoice = choice;

  if (choice == 0) {
    leftG = ROCK;
    rightG = SCISSORS;
  }
  else if (choice == 1) {
    leftG = PAPER;
    rightG = ROCK;
  }
  else {
    leftG = PAPER;
    rightG = SCISSORS;
  }
}

char chooseFinalHand(char myA, char myB, char oppA, char oppB) {
  if (isIdenticalPair(oppA, oppB)) {
    if (rpsResult(myA, oppA) == 1) return myA;
    if (rpsResult(myB, oppA) == 1) return myB;
    return myA;
  }

  if (samePairIgnoringOrder(myA, myB, oppA, oppB)) {
    return dominantHandOfPair(myA, myB);
  }

  char shared = getSharedHand(myA, myB, oppA, oppB);
  char myOther = getOtherHand(myA, myB, shared);
  char oppOther = getOtherHand(oppA, oppB, shared);

  if (rpsResult(myOther, oppOther) >= 0) return myOther;
  return shared;
}