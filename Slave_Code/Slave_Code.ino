#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

/*
  CLEAN SLAVE BOARD
  -----------------
  - Controls RIGHT hand only
  - Receives commands from master via ESP-NOW
  - No game logic on this board
*/

// =====================================================
// RIGHT HAND HARDWARE
// =====================================================

// Motor A = thumb + ring + pinky
const int R_MOTOR_A_IN1 = 26;
const int R_MOTOR_A_IN2 = 16;

// Motor B = index + middle
const int R_MOTOR_B_IN1 = 27;
const int R_MOTOR_B_IN2 = 17;

// Cover servo
const int RIGHT_COVER_SERVO_PIN = 4;

// =====================================================
// SERVO
// =====================================================
Servo rightCoverServo;

// =====================================================
// TUNING
// =====================================================
const int RIGHT_COVER_OPEN_ANGLE   = 90;
const int RIGHT_COVER_CLOSED_ANGLE = 0;

const unsigned long PULL_TIME_ROCK_MS     = 5000;
const unsigned long PULL_TIME_SCISSORS_MS = 5000;
const unsigned long RESET_TIME_MS         = 5000;
const unsigned long COVER_WAIT_MS         = 500;

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
  CMD_NONE = 0,
  CMD_SET_GESTURE = 1,
  CMD_SET_COVER   = 2,
  CMD_RESET_HAND  = 3
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

// =====================================================
// STATE
// =====================================================
char rightCurrentGesture = PAPER;
bool rightCovered = false;

volatile bool packetPending = false;
volatile uint32_t newestSeqSeen = 0;

RightHandPacket pendingPacket;
uint32_t lastProcessedSeq = 0;

// =====================================================
// FUNCTION DECLARATIONS
// =====================================================
bool initEspNow();
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);

void executePacket(const RightHandPacket &pkt);

void setRightGesture(char gesture);
void resetRightHandToPaper();
void setRightCover(bool covered);

void motorAForward();
void motorAReverse();
void motorBForward();
void motorBReverse();
void stopMotor(int in1, int in2);
void stopRightHand();

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(R_MOTOR_A_IN1, OUTPUT);
  pinMode(R_MOTOR_A_IN2, OUTPUT);
  pinMode(R_MOTOR_B_IN1, OUTPUT);
  pinMode(R_MOTOR_B_IN2, OUTPUT);

  stopRightHand();

  rightCoverServo.attach(RIGHT_COVER_SERVO_PIN);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println();
  Serial.println("=== SLAVE BOOT ===");
  Serial.print("Slave MAC: ");
  Serial.println(WiFi.macAddress());

  if (!initEspNow()) {
    Serial.println("ESP-NOW init failed.");
    while (true) delay(1000);
  }

  // Safe startup
  setRightCover(false);
  resetRightHandToPaper();

  Serial.println("=== SLAVE READY ===");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  if (packetPending) {
    noInterrupts();
    RightHandPacket pkt = pendingPacket;
    packetPending = false;
    interrupts();

    if (pkt.seq > lastProcessedSeq) {
      executePacket(pkt);
      lastProcessedSeq = pkt.seq;
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

  esp_now_register_recv_cb(onDataRecv);
  return true;
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(RightHandPacket)) return;

  RightHandPacket pkt;
  memcpy(&pkt, data, sizeof(pkt));

  // Keep only newest packet
  if (pkt.seq >= newestSeqSeen) {
    newestSeqSeen = pkt.seq;
    pendingPacket = pkt;
    packetPending = true;
  }
}

// =====================================================
// PACKET EXECUTION
// =====================================================
void executePacket(const RightHandPacket &pkt) {
  Serial.print("RX seq=");
  Serial.print(pkt.seq);
  Serial.print(" cmd=");
  Serial.print(pkt.command);
  Serial.print(" gesture=");
  Serial.print((char)pkt.gesture);
  Serial.print(" cover=");
  Serial.println(pkt.cover);

  switch (pkt.command) {
    case CMD_SET_GESTURE:
      setRightGesture((char)pkt.gesture);
      break;

    case CMD_SET_COVER:
      if (pkt.cover == COVER_OPEN) {
        setRightCover(false);
      } else if (pkt.cover == COVER_CLOSED) {
        setRightCover(true);
      }
      break;

    case CMD_RESET_HAND:
      resetRightHandToPaper();
      break;

    default:
      break;
  }
}

// =====================================================
// RIGHT HAND CONTROL
// =====================================================
void setRightGesture(char gesture) {
  if (gesture != ROCK && gesture != PAPER && gesture != SCISSORS) return;

  Serial.print("RIGHT -> ");
  Serial.println(gesture);

  if (gesture == PAPER) {
    Serial.println("RIGHT already in PAPER / no motor move");
    stopRightHand();
    return;
  }
  else if (gesture == ROCK) {
    motorAForward();
    motorBForward();
    delay(PULL_TIME_ROCK_MS);
    stopRightHand();
  }
  else if (gesture == SCISSORS) {
    motorAForward();
    stopMotor(R_MOTOR_B_IN1, R_MOTOR_B_IN2);
    delay(PULL_TIME_SCISSORS_MS);
    stopRightHand();
  }
}

void resetRightHandToPaper() {
  Serial.println("RIGHT -> reset to PAPER");
  motorAReverse();
  motorBReverse();
  delay(RESET_TIME_MS);
  stopRightHand();
  delay(150);
  rightCurrentGesture = PAPER;
}

void setRightCover(bool covered) {
  if (covered) {
    Serial.println("RIGHT COVER -> CLOSE");
    rightCoverServo.write(RIGHT_COVER_CLOSED_ANGLE);
  } else {
    Serial.println("RIGHT COVER -> OPEN");
    rightCoverServo.write(RIGHT_COVER_OPEN_ANGLE);
  }
  delay(COVER_WAIT_MS);
}

// =====================================================
// MOTOR CONTROL
// =====================================================
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