// =====================================
// SLAVE UART + MOTOR TEST
// Receives commands from master and moves motors
// =====================================

// Motor A
const int R_MOTOR_A_IN1 = 26;
const int R_MOTOR_A_IN2 = 16;  // CHANGE if 16 is being used for UART RX on your setup

// Motor B
const int R_MOTOR_B_IN1 = 27;
const int R_MOTOR_B_IN2 = 17;

// UART from master
const int SLAVE_RX_FROM_MASTER = 16;   // If using 16 for RX, DO NOT also use it for motor pin
const int SLAVE_TX_UNUSED      = 4;

HardwareSerial masterSerial(2);

void setup() {
  Serial.begin(115200);
  masterSerial.begin(115200, SERIAL_8N1, SLAVE_RX_FROM_MASTER, SLAVE_TX_UNUSED);

  pinMode(R_MOTOR_A_IN1, OUTPUT);
  pinMode(R_MOTOR_A_IN2, OUTPUT);
  pinMode(R_MOTOR_B_IN1, OUTPUT);
  pinMode(R_MOTOR_B_IN2, OUTPUT);

  stopAll();

  Serial.println("=== SLAVE TEST READY ===");
  Serial.println("Waiting for master commands...");
}

void loop() {
  if (masterSerial.available()) {
    char cmd = masterSerial.read();

    Serial.print("Received: ");
    Serial.println(cmd);

    if (cmd == 'a') {
      Serial.println("Motor A forward");
      motorAForward();
      delay(1000);
      stopAll();
    }
    else if (cmd == 'z') {
      Serial.println("Motor A reverse");
      motorAReverse();
      delay(1000);
      stopAll();
    }
    else if (cmd == 's') {
      Serial.println("Motor B forward");
      motorBForward();
      delay(1000);
      stopAll();
    }
    else if (cmd == 'x') {
      Serial.println("Motor B reverse");
      motorBReverse();
      delay(1000);
      stopAll();
    }
    else if (cmd == 'p') {
      Serial.println("Stopping all motors");
      stopAll();
    }
  }
}

// =========================
// MOTOR FUNCTIONS
// Use the same directions that worked in your motor test
// =========================
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

void stopAll() {
  stopMotor(R_MOTOR_A_IN1, R_MOTOR_A_IN2);
  stopMotor(R_MOTOR_B_IN1, R_MOTOR_B_IN2);
}