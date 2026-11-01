// IoT-Based Smart Agriculture Monitoring System - ESP32 (Wokwi)
// Sensors: DHT22 (temp/humidity), soil moisture (potentiometer in simulation)
// Output: Irrigation pump (GPIO 26). Cloud: ThingSpeak over WiFi (optional)

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

#define DHTPIN    15
#define DHTTYPE   DHT22
#define SOIL_PIN  34
#define PUMP_PIN  26

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
const String API_KEY  = "YOUR_THINGSPEAK_WRITE_KEY"; // optional

const int MOIST_LOW  = 30;   // pump ON below this (%)
const int MOIST_HIGH = 50;   // pump OFF above this (%)
const unsigned long SEND_MS = 20000;

DHT dht(DHTPIN, DHTTYPE);
bool pumpOn = false;
int pumpCount = 0;
unsigned long lastSend = 0;

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  Serial.print("Connecting to WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? " connected" : " failed (offline mode)");
}

void sendToCloud(float t, float h, int m) {
  if (WiFi.status() != WL_CONNECTED || API_KEY.startsWith("YOUR_")) {
    Serial.println("Cloud upload skipped (no WiFi or API key)");
    return;
  }
  HTTPClient http;
  String url = "http://api.thingspeak.com/update?api_key=" + API_KEY +
               "&field1=" + String(t) + "&field2=" + String(h) + "&field3=" + String(m);
  http.begin(url);
  int code = http.GET();
  Serial.printf("Cloud upload status: %d\n", code);
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(PUMP_PIN, OUTPUT);
  dht.begin();
  connectWiFi();
  Serial.println("Smart Agriculture Monitoring started");
}

void loop() {
  float temp = dht.readTemperature();
  float hum  = dht.readHumidity();
  int moisture = analogRead(SOIL_PIN) * 100 / 4095;

  // Irrigation automation with hysteresis
  if (!pumpOn && moisture < MOIST_LOW) { pumpOn = true; pumpCount++; }
  else if (pumpOn && moisture > MOIST_HIGH) { pumpOn = false; }
  digitalWrite(PUMP_PIN, pumpOn ? HIGH : LOW);

  Serial.printf("Temp: %.1f C | Humidity: %.1f %% | Soil: %d %% | Pump: %s | Irrigation cycles: %d\n",
                temp, hum, moisture, pumpOn ? "ON" : "OFF", pumpCount);

  if (millis() - lastSend > SEND_MS) {
    lastSend = millis();
    sendToCloud(temp, hum, moisture);
  }
  delay(1000);
}
