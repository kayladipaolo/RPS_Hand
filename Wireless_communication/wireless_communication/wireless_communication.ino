#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(2000);   // gives Serial Monitor time to connect

  Serial.println();
  Serial.println("Booting...");
  
  WiFi.mode(WIFI_STA);
  delay(500);

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
}