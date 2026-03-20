#include <WiFi.h>
#include <esp_now.h>

const int P26 = 26;   // Motor A reverse
const int P16 = 16;   // Motor B reverse
const int P27 = 27;   // Motor A forward
const int P17 = 17;   // Motor B forward

void setPins(int s26, int s16, int s27, int s17) {
  digitalWrite(P26, s26);
  digitalWrite(P16, s16);
  digitalWrite(P27, s27);
  digitalWrite(P17, s17);
}

void stopAll() {
  setPins(LOW, LOW, LOW, LOW);
}

void motorAForward() { stopAll(); delay(10); setPins(LOW, LOW, HIGH, LOW); }
void motorAReverse() { stopAll(); delay(10); setPins(HIGH, LOW, LOW, LOW); }
void motorBForward() { stopAll(); delay(10); setPins(LOW, LOW, LOW, HIGH); }
void motorBReverse() { stopAll(); delay(10); setPins(LOW, HIGH, LOW, LOW); }

void onReceive(const esp_now_recv_info *info, const uint8_t *data, int len) {
  if (len < 1) return;

  char cmd = (char)data[0];

  Serial.print("Received: ");
  Serial.println(cmd);

  switch (cmd) {
    case 'a': motorAForward(); break;
    case 'z': motorAReverse(); break;
    case 's': motorBForward(); break;
    case 'x': motorBReverse(); break;
    case 'p': stopAll(); break;
    default:  stopAll(); break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(P26, OUTPUT);
  pinMode(P16, OUTPUT);
  pinMode(P27, OUTPUT);
  pinMode(P17, OUTPUT);
  stopAll();

  WiFi.mode(WIFI_STA);

  Serial.print("Slave MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onReceive);
  Serial.println("SLAVE READY");
}

void loop() {}