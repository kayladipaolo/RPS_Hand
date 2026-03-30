#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

/*
  SLAVE BOARD
  -----------
  - Controls RIGHT hand locally
  - Receives commands wirelessly from MASTER via ESP-NOW
*/

// =====================================================
// RIGHT HAND HARDWARE
// =====================================================
// Motor A = index + middle
const int R_MOTOR_A_IN1 = 17;
const int R_MOTOR_A_IN2 = 16;

// Motor B = thumb + ring + pinky
const int R_MOTOR_B_IN1 = 27;
const int R_MOTOR_B_IN2 = 26;

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

// separate reset timings for right hand
const unsigned long RESET_TIME_A_MS       = 5000;  // index + middle
const unsigned long RESET_TIME_B_MS       = 2000;  // thumb + ring + pinky

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
// FUNCTION DECLARATIONS
// =====================================================
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len);

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

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed.");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onDataRecv);

  // safe startup
  setRightCover(false);

  Serial.println("=== SLAVE READY ===");
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  // nothing needed here; slave waits for packets
}

// =====================================================
// ESP-NOW RECEIVE CALLBACK
// =====================================================
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  if (len != sizeof(RightHandPacket)) {
    Serial.println("Received wrong packet size.");
    return;
  }

  RightHandPacket pkt;
  memcpy(&pkt, incomingData, sizeof(pkt));

  Serial.print("Received packet seq=");
  Serial.print(pkt.seq);
  Serial.print(" cmd=");
  Serial.print(pkt.command);
  Serial.print(" gesture=");
  Serial.print((char)pkt.gesture);
  Serial.print(" cover=");
  Serial.println(pkt.cover);

  if (pkt.command == CMD_SET_GESTURE) {
    setRightGesture((char)pkt.gesture);
  }
  else if (pkt.command == CMD_SET_COVER) {
    if (pkt.cover == COVER_OPEN) {
      setRightCover(false);
    }
    else if (pkt.cover == COVER_CLOSED) {
      setRightCover(true);
    }
  }
  else if (pkt.command == CMD_RESET_HAND) {
    resetRightHandToPaper();
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
    Serial.println("RIGHT ROCK: motorAForward + motorBForward");
    motorAForward();
    motorBForward();
    delay(PULL_TIME_ROCK_MS);
    stopRightHand();
  }
  else if (gesture == SCISSORS) {
    // Scissors = curl thumb + ring + pinky only
    Serial.println("RIGHT SCISSORS: motorBForward only");
    stopMotor(R_MOTOR_A_IN1, R_MOTOR_A_IN2);
    motorBForward();
    delay(PULL_TIME_SCISSORS_MS);
    stopRightHand();
  }
}

void resetRightHandToPaper() {
  Serial.println("RIGHT -> reset to PAPER");

  motorAReverse();
  motorBReverse();

  unsigned long startTime = millis();
  bool motorAStopped = false;
  bool motorBStopped = false;

  while (!motorAStopped || !motorBStopped) {
    unsigned long elapsed = millis() - startTime;

    if (!motorAStopped && elapsed >= RESET_TIME_A_MS) {
      stopMotor(R_MOTOR_A_IN1, R_MOTOR_A_IN2);
      motorAStopped = true;
    }

    if (!motorBStopped && elapsed >= RESET_TIME_B_MS) {
      stopMotor(R_MOTOR_B_IN1, R_MOTOR_B_IN2);
      motorBStopped = true;
    }
  }

  stopRightHand();
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
// Working direction map from your motor test
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
  digitalWrite(R_MOTOR_B_IN1, LOW);
  digitalWrite(R_MOTOR_B_IN2, HIGH);
}

void motorBReverse() {
  digitalWrite(R_MOTOR_B_IN1, HIGH);
  digitalWrite(R_MOTOR_B_IN2, LOW);
}

void stopMotor(int in1, int in2) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void stopRightHand() {
  stopMotor(R_MOTOR_A_IN1, R_MOTOR_A_IN2);
  stopMotor(R_MOTOR_B_IN1, R_MOTOR_B_IN2);
}