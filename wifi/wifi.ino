#include <WiFi.h>

void onEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == WIFI_EVENT_STA_DISCONNECTED) {
    Serial.printf("DISCONNECTED reason=%d\n", info.wifi_sta_disconnected.reason);
  }
  if (event == IP_EVENT_STA_GOT_IP) {
    Serial.printf("GOT IP: %s\n", WiFi.localIP().toString().c_str());
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.onEvent(onEvent);
  WiFi.mode(WIFI_STA);
  WiFi.begin("YOUR_SSID", "YOUR_PASSWORD");
  for (int i = 0; i < 30; i++) {
    Serial.printf("[%ds] status=%d rssi=%d\n", i, (int)WiFi.status(), WiFi.RSSI());
    delay(1000);
  }
}
void loop() {}