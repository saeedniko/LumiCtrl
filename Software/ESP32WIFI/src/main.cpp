#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include <WebServer.h>

/* ========= WIFI ========= */
const char* ssid = "Jen_kocholo";
const char* password = "S@lma+S@eed@02.2023";

/* ========= FASTLED ========= */
#define NUM_LEDS 139
#define DATA_ESP 12
CRGB leds[NUM_LEDS];

/* ========= PINS ========= */
#define SCL_PIN 26
#define SDA_PIN 13
#define eFuse_SHDN 2
#define Button3 14
#define eFuse_Fault 15
#define LED_PIN1 19
#define LED_PIN2 21
#define LVL_GATE 27
#define V_SNS 36
#define I_SNS 39

int adcval;

/* ========= WEB SERVER ========= */
WebServer server(80);

/* ========= WEB PAGE ========= */
void handleRoot() {
  String page = "<!DOCTYPE html><html><head>";
  page += "<meta http-equiv='refresh' content='1'>";
  page += "<title>ESP32 Monitor</title></head><body>";
  page += "<h2>ESP32 Web Monitor</h2>";
  page += "<b>IP:</b> " + WiFi.localIP().toString() + "<br>";
  page += "<b>ADC Value:</b> " + String(adcval) + "<br>";
  page += "</body></html>";

  server.send(200, "text/html", page);
}

void setup() {
  /* ---------- Serial ---------- */
  Serial.begin(115200);
  Serial.println("ESP32 starting...");

  /* ---------- WiFi ---------- */
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /* ---------- OTA ---------- */
  ArduinoOTA.setHostname("esp32-fastled");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress * 100) / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA Ready");

  /* ---------- Web Server ---------- */
  server.on("/", handleRoot);
  server.begin();
  Serial.println("Web server started");

  /* ---------- FastLED ---------- */
  FastLED.addLeds<NEOPIXEL, DATA_ESP>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  /* ---------- GPIO ---------- */
  pinMode(eFuse_SHDN, OUTPUT);
  pinMode(eFuse_Fault, INPUT);
  pinMode(Button3, INPUT);
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(LVL_GATE, OUTPUT);

  /* ---------- Default States ---------- */
  digitalWrite(eFuse_SHDN, LOW);
  digitalWrite(LED_PIN1, HIGH);
  digitalWrite(LED_PIN2, HIGH);
  digitalWrite(LVL_GATE, HIGH);
}

void loop() {
  ArduinoOTA.handle();     // OTA handling
  server.handleClient();   // ⭐ REQUIRED for web server

  /* ---------- LED forward ---------- */
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Orange;
    FastLED.show();
    delay(50);
    ArduinoOTA.handle();
    server.handleClient();
  }

  /* ---------- LED backward ---------- */
  for (int i = NUM_LEDS - 1; i >= 0; i--) {
    leds[i] = CRGB::DarkCyan;
    FastLED.show();
    delay(50);
    ArduinoOTA.handle();
    server.handleClient();
  }

  /* ---------- ADC ---------- */
  adcval = analogRead(V_SNS);

  Serial.print("ADC Value = ");
  Serial.println(adcval);

  delay(200);
}