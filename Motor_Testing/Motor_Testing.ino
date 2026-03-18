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
const unsigned long RUN_TIME_MS  = 2000;
const unsigned long STOP_TIME_MS = 1000;

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
  motorAForward();
  delay(RUN_TIME_MS);
  stopMotor(MOTOR_A_IN1, MOTOR_A_IN2);
  delay(STOP_TIME_MS);

  // -----------------------
  // Test Motor A Reverse
  // -----------------------
  Serial.println("Motor A reverse");
  motorAReverse();
  delay(RUN_TIME_MS);
  stopMotor(MOTOR_A_IN1, MOTOR_A_IN2);
  delay(STOP_TIME_MS);

  // -----------------------
  // Test Motor B Forward
  // -----------------------
  Serial.println("Motor B forward");
  motorBForward();
  delay(RUN_TIME_MS);
  stopMotor(MOTOR_B_IN1, MOTOR_B_IN2);
  delay(STOP_TIME_MS);

  // -----------------------
  // Test Motor B Reverse
  // -----------------------
  Serial.println("Motor B reverse");
  motorBReverse();
  delay(RUN_TIME_MS);
  stopMotor(MOTOR_B_IN1, MOTOR_B_IN2);
  delay(STOP_TIME_MS);

  Serial.println("Cycle complete. Restarting...");
  delay(2000);
}

// =====================================
// MOTOR A FUNCTIONS
// =====================================
void motorAForward() {
  // swapped because A was going backwards before
  digitalWrite(MOTOR_A_IN1, LOW);
  digitalWrite(MOTOR_A_IN2, HIGH);
}

void motorAReverse() {
  digitalWrite(MOTOR_A_IN1, HIGH);
  digitalWrite(MOTOR_A_IN2, LOW);
}

// =====================================
// MOTOR B FUNCTIONS
// =====================================
void motorBForward() {
  digitalWrite(MOTOR_B_IN1, HIGH);
  digitalWrite(MOTOR_B_IN2, LOW);
}

void motorBReverse() {
  digitalWrite(MOTOR_B_IN1, LOW);
  digitalWrite(MOTOR_B_IN2, HIGH);
}

// =====================================
// STOP FUNCTION
// =====================================
void stopMotor(int in1, int in2) {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}