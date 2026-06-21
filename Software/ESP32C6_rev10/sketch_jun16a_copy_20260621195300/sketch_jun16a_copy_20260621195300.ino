#include <WiFi.h>
#include <WebServer.h>

#include <Wire.h>
#include <ClosedCube_HDC1080.h>
#include <vl53l8ch.h>

#include <FastLED.h>

// ==========================
// Pins
// ==========================
#define SDA_PIN 6
#define SCL_PIN 7

#define NUM_LEDS 140
#define DATA_ESP 14

#define eFuse_Fault 4
#define eFuse_SHDN 5
#define LVL_GATE 15

// ==========================
// LED
// ==========================
CRGB leds[NUM_LEDS];

// ==========================
// WiFi
// ==========================
const char* ssid = "Jen_kocholo";
const char* password = "S@lma+S@eed@02.2023";

// ==========================
// Sensors
// ==========================
ClosedCube_HDC1080 hdc1080;
VL53L8CH tof(&Wire, -1);
VL53LMZ_ResultsData results;

// ==========================
// Web server
// ==========================
WebServer server(80);

// ==========================
// DATA
// ==========================
#define GRID 8
#define ZONES 64

uint16_t depth[GRID][GRID];
uint16_t prevDepth[GRID][GRID];

float temperature = 0;
float humidity = 0;

// ==========================
// timing
// ==========================
unsigned long lastSensor = 0;

// ==========================
// gesture state
// ==========================
bool gestureLocked = false;
bool colorState = false;

// ==========================
// WiFi
// ==========================
void connectWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Connecting");

    unsigned long t = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t < 20000)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi OK");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("WiFi FAILED");
    }
}

// ==========================
// LED
// ==========================
void setAllLeds(CRGB color)
{
    for (int i = 0; i < NUM_LEDS; i++)
        leds[i] = color;

    FastLED.show();
}

// ==========================
// SENSOR READ
// ==========================
void readSensors()
{
    float t = hdc1080.readTemperature();
    float h = hdc1080.readHumidity();

    if (!isnan(t)) temperature = t;
    if (!isnan(h)) humidity = h;

    uint8_t ready = 0;

    if (tof.check_data_ready(&ready) == VL53LMZ_STATUS_OK && ready)
    {
        if (tof.get_ranging_data(&results) == VL53LMZ_STATUS_OK)
        {
            for (int i = 0; i < ZONES; i++)
            {
                int r = i / GRID;
                int c = i % GRID;

                uint16_t d = results.distance_mm[i];
                if (d > 4000) d = 0;

                depth[r][c] = d;
            }
        }
    }
}

// ==========================
// MOVEMENT GESTURE DETECTION
// ==========================
void checkGesture()
{
    long motion = 0;

    // center 4x4 zone (most stable)
    for (int r = 2; r < 6; r++)
    {
        for (int c = 2; c < 6; c++)
        {
            uint16_t curr = depth[r][c];
            uint16_t prev = prevDepth[r][c];

            motion += abs((int)curr - (int)prev);

            prevDepth[r][c] = curr;
        }
    }

    // debug
    Serial.println(motion);

    // motion threshold (tune this!)
    if (motion > 8000)
    {
        if (!gestureLocked)
        {
            gestureLocked = true;

            colorState = !colorState;

            setAllLeds(colorState ? CRGB::Red : CRGB::Blue);
        }
    }
    else
    {
        gestureLocked = false;
    }
}

// ==========================
// WEB PAGE
// ==========================
void handleRoot()
{
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>ToF 8x8 Motion</title>

<style>
body{
    background:#111;
    color:white;
    text-align:center;
    font-family:Arial;
}

.grid{
    display:grid;
    grid-template-columns:repeat(8, 35px);
    gap:4px;
    justify-content:center;
    margin-top:20px;
}

.cell{
    width:35px;
    height:35px;
    font-size:9px;
    display:flex;
    align-items:center;
    justify-content:center;
    border-radius:4px;
}
</style>
</head>

<body>

<h2>8x8 ToF Motion Gesture</h2>

Temp: <span id="t">--</span> |
Hum: <span id="h">--</span>

<div id="grid" class="grid"></div>

<script>

const grid = document.getElementById("grid");

for(let i=0;i<64;i++){
    let d=document.createElement("div");
    d.className="cell";
    grid.appendChild(d);
}

async function update(){
    let res=await fetch('/data');
    let data=await res.json();

    document.getElementById("t").innerText=data.temp.toFixed(1);
    document.getElementById("h").innerText=data.hum.toFixed(1);

    for(let i=0;i<64;i++){
        let v=data.grid[i];
        let c=grid.children[i];

        c.innerText=v;

        if(v===0)c.style.background="#000";
        else if(v<300)c.style.background="#ff0000";
        else if(v<600)c.style.background="#ff8800";
        else if(v<1000)c.style.background="#ffff00";
        else c.style.background="#00aaff";
    }
}

setInterval(update,200);
update();

</script>

</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

// ==========================
// JSON
// ==========================
void handleData()
{
    String json = "{";
    json += "\"temp\":" + String(temperature) + ",";
    json += "\"hum\":" + String(humidity) + ",";
    json += "\"grid\":[";

    for (int r = 0; r < GRID; r++)
    {
        for (int c = 0; c < GRID; c++)
        {
            json += depth[r][c];
            if (!(r == 7 && c == 7)) json += ",";
        }
    }

    json += "]}";

    server.send(200, "application/json", json);
}

// ==========================
// SETUP
// ==========================
void setup()
{
    Serial.begin(115200);
    Wire.begin(SDA_PIN, SCL_PIN);

    connectWiFi();

    hdc1080.begin(0x40);

    if (tof.begin() != VL53LMZ_STATUS_OK) while (1);
    if (tof.init() != VL53LMZ_STATUS_OK) while (1);

    tof.set_resolution(8 * 8);
    tof.start_ranging();

    /* GPIO */
    pinMode(eFuse_SHDN, OUTPUT);
    pinMode(eFuse_Fault, INPUT);
    pinMode(LVL_GATE, OUTPUT);

    digitalWrite(eFuse_SHDN, LOW);
    digitalWrite(LVL_GATE, HIGH);

    FastLED.addLeds<WS2812B, DATA_ESP, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);

    setAllLeds(CRGB::Black);

    server.on("/", handleRoot);
    server.on("/data", handleData);
    server.begin();

    Serial.println("READY");
}

// ==========================
// LOOP
// ==========================
void loop()
{
    server.handleClient();

    if (millis() - lastSensor > 200)
    {
        lastSensor = millis();
        readSensors();
    }

    checkGesture();
}