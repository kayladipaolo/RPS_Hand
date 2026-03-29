#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

// =====================================================
// RIGHT HAND HARDWARE
// =====================================================
const int R_MOTOR_A_IN1 = 26;
const int R_MOTOR_A_IN2 = 16;
const int R_MOTOR_B_IN1 = 27;
const int R_MOTOR_B_IN2 = 17;

const int RIGHT_COVER_SERVO_PIN = 4;
Servo rightCoverServo;

// =====================================================
// TUNING
// =====================================================
const int RIGHT_COVER_OPEN_ANGLE = 90;
const unsigned long PULL_TIME_ROCK_MS = 5000;

// =====================================================
// COMMAND PROTOCOL
// =====================================================
enum CommandType : uint8_t {
  CMD_NONE = 0,
  CMD_SET_GESTURE = 1
};

struct RightHandPacket {
  uint32_t seq;
  uint8_t command;
  uint8_t gesture;
  uint8_t cover;
};

volatile bool packetPending = false;
RightHandPacket pendingPacket;

// =====================================================
// FUNCTION DECLARATIONS
// =====================================================
bool initEspNow();
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len);

void setRightRock();
void setRightCoverOpen();

void motorAForward();
void motorBForward();
void stopMotor(int in1, int in2);
void stopRightHand();

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(R_MOTOR_A_IN1, OUTPUT);
  pinMode(R_MOTOR_A_IN2, OUTPUT);
  pinMode(R_MOTOR_B_IN1, OUTPUT);
  pinMode(R_MOTOR_B_IN2, OUTPUT);

  stopRightHand();

  rightCoverServo.attach(RIGHT_COVER_SERVO_PIN);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println();
  Serial.println("=== SLAVE ROCK TEST ===");
  Serial.print("Slave MAC: ");
  Serial.println(WiFi.macAddress());

  if (!initEspNow()) {
    Serial.println("ESP-NOW init failed.");
    while (true) delay(1000);
  }

  setRightCoverOpen();

  Serial.println("Waiting for ROCK command from master...");
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

    Serial.print("RX seq=");
    Serial.print(pkt.seq);
    Serial.print(" cmd=");
    Serial.print(pkt.command);
    Serial.print(" gesture=");
    Serial.println((char)pkt.gesture);

    if (pkt.command == CMD_SET_GESTURE && pkt.gesture == 'R') {
      Serial.println("RIGHT HAND -> ROCK");
      setRightRock();
    }
  }
}

// =====================================================
// ESP-NOW
// =====================================================
bool initEspNow() {
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onDataRecv);
  return true;
}

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(RightHandPacket)) return;

  memcpy(&pendingPacket, data, sizeof(RightHandPacket));
  packetPending = true;
}

// =====================================================
// RIGHT HAND CONTROL
// =====================================================
void setRightCoverOpen() {
  rightCoverServo.write(RIGHT_COVER_OPEN_ANGLE);
}

void setRightRock() {
  motorAForward();
  motorBForward();
  delay(PULL_TIME_ROCK_MS);
  stopRightHand();
}

// =====================================================
// MOTOR CONTROL
// =====================================================
void motorAForward() {
  digitalWrite(R_MOTOR_A_IN1, LOW);
  digitalWrite(R_MOTOR_A_IN2, HIGH);
}

void motorBForward() {
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