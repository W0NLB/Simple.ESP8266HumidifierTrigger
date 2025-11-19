// ESP8266 Humidity Node — Dual OLED (SSD1306 @0x3C, SH1106 @0x3D)
// DHT22 on D1, OLED SDA=D6, SCL=D5
// Sends HTTP GET to ESP32: /relay?state=on  or /relay?state=off

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SSD1306Wire.h>
#include <SH1106Wire.h>
#include "DHT.h"

#define DHTPIN D1
#define DHTTYPE DHT22

// I2C pins for ESP8266
#define OLED_SDA D6
#define OLED_SCL D5

// Display objects (ThingPulse library)
SSD1306Wire displaySSD(0x3C, OLED_SDA, OLED_SCL); // 0.96" SSD1306
SH1106Wire  displaySH (0x3D, OLED_SDA, OLED_SCL); // 1.3" SH1106

// WiFi / Server config
const char* ssid = "SIGINT_KeyHole7";
const char* password = "M781166s";
const char* ESP32_IP = "192.168.1.120"; // <-- set the ESP32 IP here
const uint16_t ESP32_PORT = 80;

DHT dht(DHTPIN, DHTTYPE);

String wifiStatus = "WiFi: ?";
String lastCommand = "None";

void drawBoth(String l1, String l2, String l3, String l4, String l5) {
  // SSD1306
  displaySSD.clear();
  displaySSD.setFont(ArialMT_Plain_10);
  displaySSD.drawString(0, 0, l1);
  displaySSD.drawString(0, 12, l2);
  displaySSD.drawString(0, 24, l3);
  displaySSD.drawString(0, 36, l4);
  displaySSD.drawString(0, 48, l5);
  displaySSD.display();

  // SH1106
  displaySH.clear();
  displaySH.setFont(ArialMT_Plain_10);
  displaySH.drawString(0, 0, l1);
  displaySH.drawString(0, 12, l2);
  displaySH.drawString(0, 24, l3);
  displaySH.drawString(0, 36, l4);
  displaySH.drawString(0, 48, l5);
  displaySH.display();
}

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiStatus = "WiFi: Connected";
    return;
  }
  wifiStatus = "WiFi: Reconnecting...";
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
  }
  wifiStatus = (WiFi.status() == WL_CONNECTED) ? "WiFi: Connected" : "WiFi: Failed";
}

void sendCommand(const char* endpoint) {
  WiFiClient client;
  if (!client.connect(ESP32_IP, ESP32_PORT)) {
    Serial.println("SendCommand: connection failed");
    return;
  }
  String req = String("GET ") + endpoint + " HTTP/1.1\r\n" +
               "Host: " + ESP32_IP + "\r\n" +
               "Connection: close\r\n\r\n";
  client.print(req);
  // drain any response (brief)
  unsigned long t = millis();
  while (client.connected() && millis() - t < 200) {
    while (client.available()) client.read();
  }
  client.stop();
  Serial.println("Sent: " + String(endpoint));
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Init displays
  displaySSD.init();
  displaySH.init();
  displaySSD.flipScreenVertically();
  displaySH.flipScreenVertically();

  // Start WiFi
  WiFi.begin(ssid, password);
  wifiStatus = "WiFi: Connecting";
  while (WiFi.status() != WL_CONNECTED) {
    drawBoth("Humidity Node", "Connecting WiFi...", "", "", "");
    delay(400);
  }
  wifiStatus = "WiFi: Connected";
  drawBoth("Humidity Node", "WiFi Connected", "IP: " + WiFi.localIP().toString(), "", "");
  delay(1000);
}

void loop() {
  ensureWiFi();

  float hum = dht.readHumidity();
  float tempF = dht.readTemperature(true); // Fahrenheit

  if (isnan(hum) || isnan(tempF)) {
    Serial.println("DHT read failed");
    drawBoth("Humidity Node", "Sensor read failed", "", "", wifiStatus);
    delay(2000);
    return;
  }

  Serial.printf("Hum: %.1f%%  Temp: %.1fF\n", hum, tempF);

  // Hysteresis control using thresholds
  if (hum < 50.0) {
    // turn humidifier ON
    sendCommand("/relay?state=on");
    lastCommand = "Humidifier ON";
  } else if (hum > 59.0) {
    // turn humidifier OFF
    sendCommand("/relay?state=off");
    lastCommand = "Humidifier OFF";
  }

  // Update displays
  drawBoth("Humidity Node",
           "Hum: " + String(hum, 1) + "%",
           "Temp: " + String(tempF, 1) + " F",
           "Cmd: " + lastCommand,
           wifiStatus);

  delay(5000);
}
