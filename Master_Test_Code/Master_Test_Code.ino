#include <WiFi.h>
#include <esp_now.h>

uint8_t slaveAddress[] = {0xEC, 0x64, 0xC9, 0x5E, 0x80, 0x4C};

// New callback signature for newer ESP32 core versions
void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);

  Serial.print("Master MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  if (esp_now_register_send_cb(onSent) != ESP_OK) {
    Serial.println("Failed to register send callback");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, slaveAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("MASTER READY");
  Serial.println("a = Motor A forward");
  Serial.println("z = Motor A reverse");
  Serial.println("s = Motor B forward");
  Serial.println("x = Motor B reverse");
  Serial.println("p = stop all");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == '\n' || cmd == '\r') return;

    Serial.print("Sending: ");
    Serial.println(cmd);

    esp_err_t result = esp_now_send(slaveAddress, (uint8_t *)&cmd, sizeof(cmd));

    if (result != ESP_OK) {
      Serial.print("Send error: ");
      Serial.println(result);
    }
  }
}