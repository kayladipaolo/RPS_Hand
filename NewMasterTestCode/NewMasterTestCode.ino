#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

// =====================================================
// SLAVE MAC ADDRESS
// =====================================================
uint8_t SLAVE_MAC[] = {0xEC, 0x64, 0xC9, 0x5E, 0x80, 0x4C};

// =====================================================
// LEFT HAND HARDWARE
// =====================================================
const int L_MOTOR_A_IN1 = 26;
const int L_MOTOR_A_IN2 = 16;
const int L_MOTOR_B_IN1 = 27;
const int L_MOTOR_B_IN2 = 17;

const int LEFT_COVER_SERVO_PIN = 4;
Servo leftCoverServo;

// =====================================================
// TUNING
// =====================================================
const int LEFT_COVER_OPEN_ANGLE = 90;
const unsigned long PULL_TIME_ROCK_MS = 6000;

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

uint32_t txSequence = 1;
volatile bool lastSendOk = false;

// =====================================================
// FUNCTION DECLARATIONS
// =====================================================
bool initEspNow();
bool addSlavePeer();
bool sendRightRock();

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void setLeftRock();
void setLeftCoverOpen();

void motorAForward();
void motorBForward();
void stopMotor(int in1, int in2);
void stopLeftHand();

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(L_MOTOR_A_IN1, OUTPUT);
  pinMode(L_MOTOR_A_IN2, OUTPUT);
  pinMode(L_MOTOR_B_IN1, OUTPUT);
  pinMode(L_MOTOR_B_IN2, OUTPUT);

  stopLeftHand();

  leftCoverServo.attach(LEFT_COVER_SERVO_PIN);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println();
  Serial.println("=== MASTER ROCK TEST ===");
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

  setLeftCoverOpen();
  delay(500);

  Serial.println("LEFT HAND -> ROCK");
  setLeftRock();

  delay(500);

  Serial.println("RIGHT HAND CMD -> ROCK");
  sendRightRock();
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  // Nothing needed
}

// =====================================================
// ESP-NOW
// =====================================================
bool initEspNow() {
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_send_cb(onDataSent);
  return true;
}

bool addSlavePeer() {
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, SLAVE_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_is_peer_exist(SLAVE_MAC)) return true;

  return (esp_now_add_peer(&peerInfo) == ESP_OK);
}

bool sendRightRock() {
  RightHandPacket pkt;
  pkt.seq = txSequence++;
  pkt.command = CMD_SET_GESTURE;
  pkt.gesture = 'R';
  pkt.cover = 0;

  lastSendOk = false;

  esp_err_t result = esp_now_send(SLAVE_MAC, (uint8_t *)&pkt, sizeof(pkt));

  Serial.print("Sent packet seq=");
  Serial.print(pkt.seq);
  Serial.print(" cmd=");
  Serial.print(pkt.command);
  Serial.print(" gesture=");
  Serial.println((char)pkt.gesture);

  delay(200);

  Serial.print("ESP-NOW send queued: ");
  Serial.println(result == ESP_OK ? "YES" : "NO");
  Serial.print("Delivery callback success: ");
  Serial.println(lastSendOk ? "YES" : "NO");

  return (result == ESP_OK);
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendOk = (status == ESP_NOW_SEND_SUCCESS);
}

// =====================================================
// LEFT HAND CONTROL
// =====================================================
void setLeftCoverOpen() {
  leftCoverServo.write(LEFT_COVER_OPEN_ANGLE);
}

void setLeftRock() {
  motorAForward();
  motorBForward();
  delay(PULL_TIME_ROCK_MS);
  stopLeftHand();
}

// =====================================================
// MOTOR CONTROL
// =====================================================
void motorAForward() {
  digitalWrite(L_MOTOR_A_IN1, HIGH);
  digitalWrite(L_MOTOR_A_IN2, LOW);
}

void motorBForward() {
  digitalWrite(L_MOTOR_B_IN1, LOW);
  digitalWrite(L_MOTOR_B_IN2, HIGH);
}

void stopMotor(int in1, int in2) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

void stopLeftHand() {
  stopMotor(L_MOTOR_A_IN1, L_MOTOR_A_IN2);
  stopMotor(L_MOTOR_B_IN1, L_MOTOR_B_IN2);
}