// =====================================
// MASTER UART TEST
// Type commands in Serial Monitor:
//   a = slave motor A forward
//   z = slave motor A reverse
//   s = slave motor B forward
//   x = slave motor B reverse
//   p = stop both motors
// =====================================

const int COMM_RX_UNUSED   = 21;
const int COMM_TX_TO_SLAVE = 22;

HardwareSerial slaveSerial(2);

void setup() {
  Serial.begin(115200);
  slaveSerial.begin(115200, SERIAL_8N1, COMM_RX_UNUSED, COMM_TX_TO_SLAVE);

  Serial.println("=== MASTER TEST READY ===");
  Serial.println("Type:");
  Serial.println("  a = Motor A forward");
  Serial.println("  z = Motor A reverse");
  Serial.println("  s = Motor B forward");
  Serial.println("  x = Motor B reverse");
  Serial.println("  p = Stop both");
  Serial.println();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == '\n' || cmd == '\r') return;

    Serial.print("Sending command: ");
    Serial.println(cmd);

    slaveSerial.write(cmd);
    delay(20);
  }
}