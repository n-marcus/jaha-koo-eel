#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>

// ========== CONTROL MODE SELECTION ==========
// Set to 1 for WiFi webserver control, 0 for remote control
#define USE_WIFI_CONTROL true
// ============================================

#if USE_WIFI_CONTROL
#include <WiFi.h>
#include <WebServer.h>
#endif

#include "external_classes/vars.h"
#include "external_classes/elrs_rx.h"
#include "external_classes/motor_control.h"
#include "external_classes/neopixels.h"
#include "external_classes/nood_control.h"
#include "external_classes/serial_io.h"

float sinCounter = 0.0;
float sinCounterIncrement = 0.05;

bool doSineMovement = false;

byte hVal = 0;
byte vVal = 0;

#if USE_WIFI_CONTROL
// WiFi credentials
const char *ssid = "Theater Utrecht Productie"; // Change this to your WiFi SSID
const char *password = "Productie25!";        // Change this to your WiFi password

WebServer server(80);

// WiFi control variables
struct
{
  int16_t motor1 = 0; // -255 to 255
  int16_t motor2 = 0; // -255 to 255
  uint8_t nood1a = 0; // 0 to 255
  uint8_t nood1b = 0; // 0 to 255
  uint8_t nood2a = 0; // 0 to 255
  uint8_t nood2b = 0; // 0 to 255
  uint8_t eyeR = 0;   // 0 to 255 (red)
  uint8_t eyeG = 0;   // 0 to 255 (green)
  uint8_t eyeB = 0;   // 0 to 255 (blue)
  uint8_t mouthR = 0; // 0 to 255 (red)
  uint8_t mouthG = 0; // 0 to 255 (green)
  uint8_t mouthB = 0; // 0 to 255 (blue)
} wifiControl;

// Web server handlers
void handleRoot()
{
  String html = R"=====(<!DOCTYPE html>
<html>
<head>
  <title>Eel Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; max-width: 600px; margin: 20px auto; padding: 20px; }
    h1 { color: #333; }
    .control-group { margin: 20px 0; padding: 15px; background: #f0f0f0; border-radius: 8px; }
    .slider-container { margin: 10px 0; }
    label { display: inline-block; width: 80px; }
    input[type="range"] { width: 200px; }
    .value { display: inline-block; width: 50px; text-align: right; }
    button { padding: 10px 20px; margin: 5px; background: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; }
    button:hover { background: #45a049; }
    .status { margin: 20px 0; padding: 10px; background: #e7f3ff; border-radius: 5px; }
  </style>
</head>
<body>
  <h1>Eel Control Panel</h1>
  <div class="status" id="status">Connected</div>
  
  <div class="control-group">
    <h2>Motors</h2>
    <div class="slider-container">
      <label>Motor 1:</label>
      <input type="range" id="motor1" min="-255" max="255" value="0" oninput="updateValue('motor1')" onchange="setMotors()">
      <span class="value" id="motor1_val">0</span>
    </div>
    <div class="slider-container">
      <label>Motor 2:</label>
      <input type="range" id="motor2" min="-255" max="255" value="0" oninput="updateValue('motor2')" onchange="setMotors()">
      <span class="value" id="motor2_val">0</span>
    </div>
    <button onclick="stopMotors()">Stop All</button>
  </div>

  <div class="control-group">
    <h2>Noods (Body Lights)</h2>
    <div class="slider-container">
      <label>Nood 1a:</label>
      <input type="range" id="nood1a" min="0" max="255" value="0" oninput="updateValue('nood1a')" onchange="setNoods()">
      <span class="value" id="nood1a_val">0</span>
    </div>
    <div class="slider-container">
      <label>Nood 1b:</label>
      <input type="range" id="nood1b" min="0" max="255" value="0" oninput="updateValue('nood1b')" onchange="setNoods()">
      <span class="value" id="nood1b_val">0</span>
    </div>
    <div class="slider-container">
      <label>Nood 2a:</label>
      <input type="range" id="nood2a" min="0" max="255" value="0" oninput="updateValue('nood2a')" onchange="setNoods()">
      <span class="value" id="nood2a_val">0</span>
    </div>
    <div class="slider-container">
      <label>Nood 2b:</label>
      <input type="range" id="nood2b" min="0" max="255" value="0" oninput="updateValue('nood2b')" onchange="setNoods()">
      <span class="value" id="nood2b_val">0</span>
    </div>
    <button onclick="noodsOff()">Noods Off</button>
  </div>

  <div class="control-group">
    <h2>Eyes (NeoPixels)</h2>
    <div class="slider-container">
      <label>Red:</label>
      <input type="range" id="eyeR" min="0" max="255" value="0" oninput="updateValue('eyeR')" onchange="setEyes()">
      <span class="value" id="eyeR_val">0</span>
    </div>
    <div class="slider-container">
      <label>Green:</label>
      <input type="range" id="eyeG" min="0" max="255" value="0" oninput="updateValue('eyeG')" onchange="setEyes()">
      <span class="value" id="eyeG_val">0</span>
    </div>
    <div class="slider-container">
      <label>Blue:</label>
      <input type="range" id="eyeB" min="0" max="255" value="0" oninput="updateValue('eyeB')" onchange="setEyes()">
      <span class="value" id="eyeB_val">0</span>
    </div>
  </div>

  <div class="control-group">
    <h2>Mouth (NeoPixel)</h2>
    <div class="slider-container">
      <label>Red:</label>
      <input type="range" id="mouthR" min="0" max="255" value="0" oninput="updateValue('mouthR')" onchange="setMouth()">
      <span class="value" id="mouthR_val">0</span>
    </div>
    <div class="slider-container">
      <label>Green:</label>
      <input type="range" id="mouthG" min="0" max="255" value="0" oninput="updateValue('mouthG')" onchange="setMouth()">
      <span class="value" id="mouthG_val">0</span>
    </div>
    <div class="slider-container">
      <label>Blue:</label>
      <input type="range" id="mouthB" min="0" max="255" value="0" oninput="updateValue('mouthB')" onchange="setMouth()">
      <span class="value" id="mouthB_val">0</span>
    </div>
  </div>

  <script>
    function updateValue(id) {
      document.getElementById(id + '_val').textContent = document.getElementById(id).value;
    }

    function sendCommand(endpoint, data) {
      fetch(endpoint, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(data)
      })
      .then(response => response.text())
      .then(data => {
        document.getElementById('status').textContent = data;
        setTimeout(() => document.getElementById('status').textContent = 'Connected', 2000);
      })
      .catch(err => {
        document.getElementById('status').textContent = 'Error: ' + err;
      });
    }

    function setMotors() {
      sendCommand('/motors', {
        motor1: parseInt(document.getElementById('motor1').value),
        motor2: parseInt(document.getElementById('motor2').value)
      });
    }

    function stopMotors() {
      document.getElementById('motor1').value = 0;
      document.getElementById('motor2').value = 0;
      updateValue('motor1');
      updateValue('motor2');
      setMotors();
    }

    function setNoods() {
      sendCommand('/noods', {
        nood1a: parseInt(document.getElementById('nood1a').value),
        nood1b: parseInt(document.getElementById('nood1b').value),
        nood2a: parseInt(document.getElementById('nood2a').value),
        nood2b: parseInt(document.getElementById('nood2b').value)
      });
    }

    function noodsOff() {
      ['nood1a', 'nood1b', 'nood2a', 'nood2b'].forEach(id => {
        document.getElementById(id).value = 0;
        updateValue(id);
      });
      setNoods();
    }

    function setEyes() {
      sendCommand('/eyes', {
        r: parseInt(document.getElementById('eyeR').value),
        g: parseInt(document.getElementById('eyeG').value),
        b: parseInt(document.getElementById('eyeB').value)
      });
    }

    function setMouth() {
      sendCommand('/mouth', {
        r: parseInt(document.getElementById('mouthR').value),
        g: parseInt(document.getElementById('mouthG').value),
        b: parseInt(document.getElementById('mouthB').value)
      });
    }
  </script>
</body>
</html>
)=====";
  server.send(200, "text/html", html);
}

void handleMotors()
{
  if (server.hasArg("plain"))
  {
    String body = server.arg("plain");
    // Simple JSON parsing
    int motor1Idx = body.indexOf("\"motor1\":");
    int motor2Idx = body.indexOf("\"motor2\":");

    if (motor1Idx >= 0)
    {
      int start = body.indexOf(':', motor1Idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.motor1 = body.substring(start, end).toInt();
    }

    if (motor2Idx >= 0)
    {
      int start = body.indexOf(':', motor2Idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.motor2 = body.substring(start, end).toInt();
    }

    server.send(200, "text/plain", "Motors set: M1=" + String(wifiControl.motor1) + " M2=" + String(wifiControl.motor2));
  }
  else
  {
    server.send(400, "text/plain", "No data");
  }
}

void handleNoods()
{
  if (server.hasArg("plain"))
  {
    String body = server.arg("plain");

    int idx = body.indexOf("\"nood1a\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.nood1a = body.substring(start, end).toInt();
    }

    idx = body.indexOf("\"nood1b\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.nood1b = body.substring(start, end).toInt();
    }

    idx = body.indexOf("\"nood2a\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.nood2a = body.substring(start, end).toInt();
    }

    idx = body.indexOf("\"nood2b\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.nood2b = body.substring(start, end).toInt();
    }

    server.send(200, "text/plain", "Noods set");
  }
  else
  {
    server.send(400, "text/plain", "No data");
  }
}

void handleEyes()
{
  if (server.hasArg("plain"))
  {
    String body = server.arg("plain");

    int idx = body.indexOf("\"r\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.eyeR = body.substring(start, end).toInt();
    }

    idx = body.indexOf("\"g\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.eyeG = body.substring(start, end).toInt();
    }

    idx = body.indexOf("\"b\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.eyeB = body.substring(start, end).toInt();
    }

    server.send(200, "text/plain", "Eyes set");
  }
  else
  {
    server.send(400, "text/plain", "No data");
  }
}

void handleMouth()
{
  if (server.hasArg("plain"))
  {
    String body = server.arg("plain");

    int idx = body.indexOf("\"r\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.mouthR = body.substring(start, end).toInt();
    }

    idx = body.indexOf("\"g\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.mouthG = body.substring(start, end).toInt();
    }

    idx = body.indexOf("\"b\":");
    if (idx >= 0)
    {
      int start = body.indexOf(':', idx) + 1;
      int end = body.indexOf(',', start);
      if (end == -1)
        end = body.indexOf('}', start);
      wifiControl.mouthB = body.substring(start, end).toInt();
    }

    server.send(200, "text/plain", "Mouth set");
  }
  else
  {
    server.send(400, "text/plain", "No data");
  }
}

void handleStatus()
{
  String json = "{";
  json += "\"motor1\":" + String(wifiControl.motor1) + ",";
  json += "\"motor2\":" + String(wifiControl.motor2) + ",";
  json += "\"nood1a\":" + String(wifiControl.nood1a) + ",";
  json += "\"nood1b\":" + String(wifiControl.nood1b) + ",";
  json += "\"nood2a\":" + String(wifiControl.nood2a) + ",";
  json += "\"nood2b\":" + String(wifiControl.nood2b) + ",";
  json += "\"eyeR\":" + String(wifiControl.eyeR) + ",";
  json += "\"eyeG\":" + String(wifiControl.eyeG) + ",";
  json += "\"eyeB\":" + String(wifiControl.eyeB) + ",";
  json += "\"mouthR\":" + String(wifiControl.mouthR) + ",";
  json += "\"mouthG\":" + String(wifiControl.mouthG) + ",";
  json += "\"mouthB\":" + String(wifiControl.mouthB);
  json += "}";
  server.send(200, "application/json", json);
}

void initWiFi()
{
  Serial.println("\n=== WiFi Control Mode Enabled ===");
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("\nAccess the control panel at:");
    Serial.print("http://");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\nWiFi connection failed! Check credentials.");
  }

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/motors", HTTP_POST, handleMotors);
  server.on("/noods", HTTP_POST, handleNoods);
  server.on("/eyes", HTTP_POST, handleEyes);
  server.on("/mouth", HTTP_POST, handleMouth);
  server.on("/status", HTTP_GET, handleStatus);

  server.begin();
  Serial.println("Web server started");
  Serial.println("================================\n");
}
#endif

// define methods
void updateHeadBodyLights_disconnected();
void updateHeadBodyState();

void setup()
{
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize

#if USE_WIFI_CONTROL
  initWiFi();
#else
  initELRSRX();
#endif

  initNoods();
  initMotors();
  initPixels();

  Serial.println("Hello World!");
#if USE_WIFI_CONTROL
  Serial.println("Mode: WiFi Control");
#else
  Serial.println("Mode: Remote Control");
#endif
}

void loop()
{
#if USE_WIFI_CONTROL
  // WiFi mode: handle web server and apply WiFi control values directly
  server.handleClient();

  // Apply WiFi control values directly to outputs
  motor1Val = wifiControl.motor1;
  motor2Val = wifiControl.motor2;
  driveMotors();

  // Set noods directly
  noodAvgVals[0] = wifiControl.nood1a;
  noodAvgVals[1] = wifiControl.nood1b;
  noodAvgVals[2] = wifiControl.nood2a;
  noodAvgVals[3] = wifiControl.nood2b;
  setBodyLights();

  // Set NeoPixels directly
  rgbLeds[RIGHT_EYE] = pixels.Color(wifiControl.eyeR, wifiControl.eyeG, wifiControl.eyeB);
  rgbLeds[LEFT_EYE] = pixels.Color(wifiControl.eyeR, wifiControl.eyeG, wifiControl.eyeB);
  rgbLeds[MOUTH] = pixels.Color(wifiControl.mouthR, wifiControl.mouthG, wifiControl.mouthB);
  setHeadLights();

  delay(10); // Small delay for stability

#else
  // Remote control mode: original behavior
  // Serial.println("loop start " + String(millis()));
  // read SBUS
  parseSBUS(false);

  updateSerialIO();

  updateHeadBodyState();

  // determine how to set the head and body lights
  switch (connectionState)
  {
  case DISCONNECTED:
    resetSbusData();
    updateHeadBodyLights_disconnected();
    break;
  case CONNECTION_ESTABLISHED:
    // maybe do some temporary transition lighting here from disconnected to connected, but for now just go to connected
    connectionState = CONNECTED;
    break;
  case CONNECTION_LOST:
    // maybe do some temporary transition lighting here from connected to disconnected, but for now just go to disconnected
    connectionState = DISCONNECTED;
    break;
  case CONNECTED:
    updateBodyLightValues();
    updateHeadLightValues();
    break;
  }

  setBodyLights();
  setHeadLights();

  calcMotorValues();
  driveMotors();

  // EVERY_N_SECONDS(1)
  // {
  //   Serial.println("connectionState: " + String(connectionState) + ", headState: " + String(headState));
  // }

  // delay a little.
  delay(1000 / 200);
#endif
}

void updateHeadBodyState()
{
  if (data.ch[TX_AUX2] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    headState = STATE_1;
  }
  else if (data.ch[TX_AUX2] > SBUS_SWITCH_MAX_THRESHOLD)
  {
    headState = STATE_2;
  }
  else
  {
    headState = STATE_3;
  }

  // override head state if aux1 is low
  if (data.ch[TX_AUX1] > SBUS_SWITCH_MIN_THRESHOLD)
  {
    headState = STATE_4;
  }
}

void updateHeadBodyLights_disconnected()
{
  // set noods off
  noodAvgVals[0] = 0;
  noodAvgVals[1] = 0;
  noodAvgVals[2] = 0;
  noodAvgVals[3] = 0;

  // blink rgb leds red
  if ((millis() / 500) % 2 == 0)
  {
    rgbLeds[RIGHT_EYE] = pixels.Color(255, 0, 0);
    rgbLeds[MOUTH] = pixels.Color(255, 0, 0);
    rgbLeds[LEFT_EYE] = pixels.Color(255, 0, 0);
  }
  else
  {
    rgbLeds[RIGHT_EYE] = pixels.Color(0, 0, 0);
    rgbLeds[MOUTH] = pixels.Color(0, 0, 0);
    rgbLeds[LEFT_EYE] = pixels.Color(0, 0, 0);
  }
}
