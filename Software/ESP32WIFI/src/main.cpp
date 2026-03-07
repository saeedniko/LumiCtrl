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

/* ========= LED MODE ========= */
int ledMode = 0;
bool ledPower = true;

uint8_t hue = 0;
uint8_t brightness = 0;
bool fadeDirection = true;

/* ========= LED EFFECTS ========= */

void effectSolid(CRGB color)
{
    for(int i=0;i<NUM_LEDS;i++)
        leds[i]=color;

    FastLED.show();
}

void effectRainbow()
{
    fill_rainbow(leds, NUM_LEDS, hue++, 5);
    FastLED.show();
}

void effectColorWipe()
{
    static int pos = 0;

    leds[pos] = CHSV(hue,255,255);
    FastLED.show();

    pos++;
    if(pos>=NUM_LEDS)
    {
        pos=0;
        FastLED.clear();
        hue+=30;
    }
}

void effectBreathing()
{
    static uint8_t currentHue = random8();   // start with random color

    if(fadeDirection) brightness++;
    else brightness--;

    if(brightness >= 255)
    {
        fadeDirection = false;
    }

    if(brightness <= 0)
    {
        fadeDirection = true;
        currentHue = random8();   // new random color each breath
    }

    fill_solid(leds, NUM_LEDS, CHSV(currentHue,255,brightness));
    FastLED.show();
}

/* ========= LED CONTROLLER ========= */

void updateLEDs()
{
    if(!ledPower)
    {
        FastLED.clear();
        FastLED.show();
        return;
    }

    switch(ledMode)
    {
        case 1: effectSolid(CRGB::Red); break;
        case 2: effectSolid(CRGB::Green); break;
        case 3: effectSolid(CRGB::Blue); break;
        case 4: effectSolid(CRGB::DarkViolet); break;
        case 5: effectRainbow(); break;
        case 6: effectColorWipe(); break;
        case 7: effectBreathing(); break;
    }
}

/* ========= WEB PAGE ========= */

void handleRoot()
{
String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP32 LED Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
button{width:120px;height:40px;font-size:16px;margin:5px;}
</style>
</head>
<body>

<h2>ESP32 LED Controller</h2>

<h3>Power</h3>
<a href="/on"><button>ON</button></a>
<a href="/off"><button>OFF</button></a>

<h3>Colors</h3>
<a href="/mode?m=1">
<button style="background-color:green;color:white;">Green</button>
</a>
<a href="/mode?m=2">
<button style="background-color:red;color:white;">Red</button>
</a>
<a href="/mode?m=3">
<button style="background-color:Blue;color:white;">Blue</button>
</a>
<a href="/mode?m=4">
<button style="background-color:indigo;color:white;">indigo</button>
</a>

<h3>Effects</h3>
<a href="/mode?m=5">
<button style="
background: linear-gradient(90deg, red, orange, yellow, green, blue, indigo, violet);
color:white;
font-weight:bold;
width:120px;
height:40px;
border:none;
border-radius:8px;">
Rainbow
</button>
</a>
<a href="/mode?m=6"><button>Color Wipe</button></a>
<a href="/mode?m=7"><button>Breathing</button></a>

</body>
</html>
)rawliteral";

server.send(200,"text/html",page);
}

/* ========= WEB ACTIONS ========= */

void handleOn()
{
    ledPower=true;
    server.sendHeader("Location","/");
    server.send(303);
}

void handleOff()
{
    ledPower=false;
    server.sendHeader("Location","/");
    server.send(303);
}

void handleMode()
{
    if(server.hasArg("m"))
    {
        ledMode = server.arg("m").toInt();
    }

    server.sendHeader("Location","/");
    server.send(303);
}

/* ========= SETUP ========= */

void setup()
{
Serial.begin(115200);

/* WIFI */
WiFi.begin(ssid,password);

while(WiFi.status()!=WL_CONNECTED)
{
delay(500);
Serial.print(".");
}

Serial.println(WiFi.localIP());

/* OTA */
ArduinoOTA.begin();

/* FASTLED */
FastLED.addLeds<NEOPIXEL, DATA_ESP>(leds, NUM_LEDS);

/* WEB */
server.on("/",handleRoot);
server.on("/on",handleOn);
server.on("/off",handleOff);
server.on("/mode",handleMode);

server.begin();

/* GPIO */
pinMode(eFuse_SHDN, OUTPUT);
pinMode(eFuse_Fault, INPUT);
pinMode(Button3, INPUT);
pinMode(LED_PIN1, OUTPUT);
pinMode(LED_PIN2, OUTPUT);
pinMode(LVL_GATE, OUTPUT);

digitalWrite(eFuse_SHDN, LOW);
digitalWrite(LED_PIN1, HIGH);
digitalWrite(LED_PIN2, HIGH);
digitalWrite(LVL_GATE, HIGH);
}

/* ========= LOOP ========= */

void loop()
{
ArduinoOTA.handle();
server.handleClient();

updateLEDs();

adcval = analogRead(V_SNS);

delay(20);
}