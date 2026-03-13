// =====================================
// 2-MOTOR TEST CODE
// For ESP32 + MX1508-style motor driver
// =====================================

// -------- PIN ASSIGNMENTS --------
// Motor A
const int MOTOR_A_IN1 = 26;
const int MOTOR_A_IN2 = 16;

// Motor B
const int MOTOR_B_IN1 = 27;
const int MOTOR_B_IN2 = 17;

// -------- TIMING --------
const unsigned long RUN_TIME_MS  = 2000;  // how long each motor runs
const unsigned long STOP_TIME_MS = 1000;  // pause between tests

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);

  stopMotor(MOTOR_A_IN1, MOTOR_A_IN2);
  stopMotor(MOTOR_B_IN1, MOTOR_B_IN2);

  Serial.println("Starting 2-motor test...");
}

void loop() {
  // -----------------------
  // Test Motor A Forward
  // -----------------------
  Serial.println("Motor A forward");
  motorForward(MOTOR_A_IN1, MOTOR_A_IN2);
  delay(RUN_TIME_MS);
  stopMotor(MOTOR_A_IN1, MOTOR_A_IN2);
  delay(STOP_TIME_MS);

  // -----------------------
  // Test Motor A Reverse
  // -----------------------
  Serial.println("Motor A reverse");
  motorReverse(MOTOR_A_IN1, MOTOR_A_IN2);
  delay(RUN_TIME_MS);
  stopMotor(MOTOR_A_IN1, MOTOR_A_IN2);
  delay(STOP_TIME_MS);

  // -----------------------
  // Test Motor B Forward
  // -----------------------
  Serial.println("Motor B forward");
  motorForward(MOTOR_B_IN1, MOTOR_B_IN2);
  delay(RUN_TIME_MS);
  stopMotor(MOTOR_B_IN1, MOTOR_B_IN2);
  delay(STOP_TIME_MS);

  // -----------------------
  // Test Motor B Reverse
  // -----------------------
  Serial.println("Motor B reverse");
  motorReverse(MOTOR_B_IN1, MOTOR_B_IN2);
  delay(RUN_TIME_MS);
  stopMotor(MOTOR_B_IN1, MOTOR_B_IN2);
  delay(STOP_TIME_MS);

  Serial.println("Cycle complete. Restarting...");
  delay(2000);
}

// =====================================
// MOTOR FUNCTIONS
// =====================================
void motorForward(int in1, int in2) {
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
}

void motorReverse(int in1, int in2) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
}

void stopMotor(int in1, int in2) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}