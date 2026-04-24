#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_NeoPixel.h>

// PWM properties
const uint32_t freq = 2500;
const uint8_t resolution = 8;

// Motor pins
#define motor1a_pin D5
#define motor1b_pin D4
#define motor2a_pin D3
#define motor2b_pin D2
#define motor1_chan 4
#define motor2_chan 5

// Nood pins
#define n00d_1a_Pin D8
#define n00d_1b_Pin D1
#define n00d_2a_Pin D10
#define n00d_2b_Pin D0
static const uint8_t nood1a_chan = 0;
static const uint8_t nood1b_chan = 1;
static const uint8_t nood2a_chan = 2;
static const uint8_t nood2b_chan = 3;

// Neopixel
#define NEOPIXEL_PIN D9
#define NUM_NEOPIXELS 3
Adafruit_NeoPixel pixels(NUM_NEOPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Motor control functions
void motorAgo(uint8_t speed)
{
  ledcDetachPin(motor1b_pin);
  digitalWrite(motor1b_pin, 0);
  ledcAttachPin(motor1a_pin, motor1_chan);
  ledcWrite(motor1_chan, speed);
}

void motorArev(uint8_t speed)
{
  ledcDetachPin(motor1a_pin);
  digitalWrite(motor1a_pin, 0);
  ledcAttachPin(motor1b_pin, motor1_chan);
  ledcWrite(motor1_chan, speed);
}

void motorAstop()
{
  ledcDetachPin(motor1a_pin);
  ledcDetachPin(motor1b_pin);
  digitalWrite(motor1a_pin, 0);
  digitalWrite(motor1b_pin, 0);
}

void motorBgo(uint8_t speed)
{
  ledcDetachPin(motor2b_pin);
  digitalWrite(motor2b_pin, 0);
  ledcAttachPin(motor2a_pin, motor2_chan);
  ledcWrite(motor2_chan, speed);
}

void motorBrev(uint8_t speed)
{
  ledcDetachPin(motor2a_pin);
  digitalWrite(motor2a_pin, 0);
  ledcAttachPin(motor2b_pin, motor2_chan);
  ledcWrite(motor2_chan, speed);
}

void motorBstop()
{
  ledcDetachPin(motor2a_pin);
  ledcDetachPin(motor2b_pin);
  digitalWrite(motor2a_pin, 0);
  digitalWrite(motor2b_pin, 0);
}

// Nood control function
void setn00d(uint8_t chan, uint8_t val)
{
  ledcWrite(chan, val);
}

void initNoods()
{
  pinMode(n00d_1a_Pin, OUTPUT);
  pinMode(n00d_1b_Pin, OUTPUT);
  pinMode(n00d_2a_Pin, OUTPUT);
  pinMode(n00d_2b_Pin, OUTPUT);

  ledcSetup(nood1a_chan, freq, resolution);
  ledcAttachPin(n00d_1a_Pin, nood1a_chan);
  ledcSetup(nood1b_chan, freq, resolution);
  ledcAttachPin(n00d_1b_Pin, nood1b_chan);
  ledcSetup(nood2a_chan, freq, resolution);
  ledcAttachPin(n00d_2a_Pin, nood2a_chan);
  ledcSetup(nood2b_chan, freq, resolution);
  ledcAttachPin(n00d_2b_Pin, nood2b_chan);

  setn00d(nood1a_chan, 0);
  setn00d(nood1b_chan, 0);
  setn00d(nood2a_chan, 0);
  setn00d(nood2b_chan, 0);
}

void initMotors()
{
  pinMode(motor1a_pin, OUTPUT);
  pinMode(motor1b_pin, OUTPUT);
  pinMode(motor2a_pin, OUTPUT);
  pinMode(motor2b_pin, OUTPUT);

  ledcSetup(motor1_chan, freq, resolution);
  ledcSetup(motor2_chan, freq, resolution);

  motorAstop();
  motorBstop();
}

// WiFi credentials
const char *ssid = "Pablos Paleis MainFrame";
const char *password = "VraagPablo33";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Eel Controller</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    h3 {font-size: 1.5rem;}
    p {font-size: 1.2rem;}
    body {max-width: 600px; margin:0px auto; padding: 20px;}
    .button {background-color: #4CAF50; border: none; color: white; padding: 15px 32px;
      text-align: center; text-decoration: none; display: inline-block; font-size: 16px;
      margin: 4px 2px; cursor: pointer; border-radius: 4px;}
    .button-stop {background-color: #f44336;}
    .slider {-webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb {width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .pixel-control {display: inline-block; margin: 10px; padding: 10px; border: 1px solid #ccc; border-radius: 5px;}
    .joystick-container {width: 300px; height: 300px; margin: 20px auto; position: relative; background: #ddd; border-radius: 10px; border: 3px solid #333; touch-action: none;}
    .joystick-stick {width: 60px; height: 60px; background: #003249; border-radius: 50%; position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); pointer-events: none;}
    .joystick-center {position: absolute; top: 50%; left: 50%; width: 2px; height: 2px; background: #ff0000;}
  </style>
</head>
<body>
  <h2>ESP32 Eel Controller</h2>
  
  <h3>2D Joystick Control</h3>
  <p>Drag to control motors - Up/Down: Speed, Left/Right: Steering</p>
  <div class="joystick-container" id="joystick">
    <div class="joystick-center"></div>
    <div class="joystick-stick" id="stick"></div>
  </div>
  <p>Motor A: <span id="motorA-speed">0</span> | Motor B: <span id="motorB-speed">0</span></p>
  
  <h3>Motor A</h3>
  <button class="button" onclick="motorControl('A', 'forward')">Forward</button>
  <button class="button button-stop" onclick="motorControl('A', 'stop')">Stop</button>
  <button class="button" onclick="motorControl('A', 'backward')">Backward</button>

  <h3>Motor B</h3>
  <button class="button" onclick="motorControl('B', 'forward')">Forward</button>
  <button class="button button-stop" onclick="motorControl('B', 'stop')">Stop</button>
  <button class="button" onclick="motorControl('B', 'backward')">Backward</button>

  <h3>Noods (Body Lights)</h3>
  <p>Nood 1A: <span id="nood1a-value">0</span></p>
  <input type="range" min="0" max="255" value="0" class="slider" id="nood1a" oninput="updateNood('1a', this.value)">
  <p>Nood 1B: <span id="nood1b-value">0</span></p>
  <input type="range" min="0" max="255" value="0" class="slider" id="nood1b" oninput="updateNood('1b', this.value)">
  <p>Nood 2A: <span id="nood2a-value">0</span></p>
  <input type="range" min="0" max="255" value="0" class="slider" id="nood2a" oninput="updateNood('2a', this.value)">
  <p>Nood 2B: <span id="nood2b-value">0</span></p>
  <input type="range" min="0" max="255" value="0" class="slider" id="nood2b" oninput="updateNood('2b', this.value)">

  <h3>NeoPixels (Head Lights)</h3>
  <div style="margin: 20px;">
    <button class="button" style="background-color: #ff0000;" onclick="setAllPixels(255,0,0)">Red</button>
    <button class="button" style="background-color: #00ff00;" onclick="setAllPixels(0,255,0)">Green</button>
    <button class="button" style="background-color: #0000ff;" onclick="setAllPixels(0,0,255)">Blue</button>
    <button class="button" style="background-color: #ffff00;" onclick="setAllPixels(255,255,0)">Yellow</button>
    <button class="button" style="background-color: #ff00ff;" onclick="setAllPixels(255,0,255)">Magenta</button>
    <button class="button" style="background-color: #00ffff;" onclick="setAllPixels(0,255,255)">Cyan</button>
    <button class="button" style="background-color: #ffffff; color: black;" onclick="setAllPixels(255,255,255)">White</button>
    <button class="button" style="background-color: #000000;" onclick="setAllPixels(0,0,0)">Off</button>
  </div>

<script>
function motorControl(motor, action) {
  fetch('/motor?motor=' + motor + '&action=' + action);
}

function updateNood(nood, value) {
  document.getElementById('nood' + nood + '-value').innerHTML = value;
  fetch('/nood?id=' + nood + '&value=' + value);
}

function setAllPixels(r, g, b) {
  for (let i = 0; i < 3; i++) {
    fetch('/pixel?id=' + i + '&r=' + r + '&g=' + g + '&b=' + b);
  }
}

// 2D Joystick Control
let joystick = document.getElementById('joystick');
let stick = document.getElementById('stick');
let isDragging = false;

function updateJoystick(x, y) {
  let rect = joystick.getBoundingClientRect();
  let centerX = rect.width / 2;
  let centerY = rect.height / 2;
  
  // Calculate position relative to center (-1 to 1)
  let posX = (x - centerX) / centerX;
  let posY = -(y - centerY) / centerY; // Invert Y so up is positive
  
  // Clamp to circle
  let distance = Math.sqrt(posX * posX + posY * posY);
  if (distance > 1) {
    posX /= distance;
    posY /= distance;
  }
  
  // Update stick position
  stick.style.left = (50 + posX * 50) + '%';
  stick.style.top = (50 - posY * 50) + '%';
  
  // Calculate motor speeds (-255 to 255)
  let throttle = Math.round(posY * 255);
  let steering = posX;
  
  // Differential steering
  let motorA = throttle + (steering * Math.abs(throttle) * 0.5);
  let motorB = throttle - (steering * Math.abs(throttle) * 0.5);
  
  // Clamp to -255 to 255
  motorA = Math.max(-255, Math.min(255, Math.round(motorA)));
  motorB = Math.max(-255, Math.min(255, Math.round(motorB)));
  
  // Update display
  document.getElementById('motorA-speed').innerHTML = motorA;
  document.getElementById('motorB-speed').innerHTML = motorB;
  
  // Send to server
  fetch('/joystick?a=' + motorA + '&b=' + motorB);
}

function resetJoystick() {
  stick.style.left = '50%';
  stick.style.top = '50%';
  document.getElementById('motorA-speed').innerHTML = '0';
  document.getElementById('motorB-speed').innerHTML = '0';
  fetch('/joystick?a=0&b=0');
}

joystick.addEventListener('mousedown', (e) => { 
  isDragging = true; 
  updateJoystick(e.offsetX, e.offsetY); 
});

document.addEventListener('mousemove', (e) => { 
  if (isDragging) {
    let rect = joystick.getBoundingClientRect();
    updateJoystick(e.clientX - rect.left, e.clientY - rect.top);
  }
});

document.addEventListener('mouseup', () => { 
  if (isDragging) {
    isDragging = false; 
    resetJoystick(); 
  }
});

joystick.addEventListener('touchstart', (e) => { e.preventDefault(); let touch = e.touches[0]; let rect = joystick.getBoundingClientRect(); updateJoystick(touch.clientX - rect.left, touch.clientY - rect.top); });
joystick.addEventListener('touchmove', (e) => { e.preventDefault(); let touch = e.touches[0]; let rect = joystick.getBoundingClientRect(); updateJoystick(touch.clientX - rect.left, touch.clientY - rect.top); });
joystick.addEventListener('touchend', (e) => { e.preventDefault(); resetJoystick(); });
</script>
</body>
</html>
)rawliteral";

void setup()
{
  Serial.begin(115200);

  // Initialize motors, noods, and neopixels
  initMotors();
  initNoods();
  pixels.begin();

  Serial.println("Initializing WiFi...");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", index_html); });

  // Motor control route
  server.on("/motor", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String motor = request->getParam("motor")->value();
    String action = request->getParam("action")->value();
    
    if (motor == "A") {
      if (action == "forward") {
        motorAgo(200);
      } else if (action == "backward") {
        motorArev(200);
      } else if (action == "stop") {
        motorAstop();
      }
    } else if (motor == "B") {
      if (action == "forward") {
        motorBgo(200);
      } else if (action == "backward") {
        motorBrev(200);
      } else if (action == "stop") {
        motorBstop();
      }
    }
    request->send(200, "text/plain", "OK"); });

  // Nood control route
  server.on("/nood", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String noodId = request->getParam("id")->value();
    int value = request->getParam("value")->value().toInt();
    int invertedValue = 255 - value;  // Invert the intensity
    
    if (noodId == "1a") {
      setn00d(nood1a_chan, invertedValue);
    } else if (noodId == "1b") {
      setn00d(nood1b_chan, invertedValue);
    } else if (noodId == "2a") {
      setn00d(nood2a_chan, invertedValue);
    } else if (noodId == "2b") {
      setn00d(nood2b_chan, invertedValue);
    }
    request->send(200, "text/plain", "OK"); });

  // NeoPixel control route
  server.on("/pixel", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    int pixelId = request->getParam("id")->value().toInt();
    int r = request->getParam("r")->value().toInt();
    int g = request->getParam("g")->value().toInt();
    int b = request->getParam("b")->value().toInt();
    
    pixels.setPixelColor(pixelId, pixels.Color(r, g, b));
    pixels.show();
    
    request->send(200, "text/plain", "OK"); });

  // Joystick dual motor control route
  server.on("/joystick", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    int motorASpeed = request->getParam("a")->value().toInt();
    int motorBSpeed = request->getParam("b")->value().toInt();
    
    // Motor A control
    if (motorASpeed > 10) {
      motorAgo(abs(motorASpeed));
    } else if (motorASpeed < -10) {
      motorArev(abs(motorASpeed));
    } else {
      motorAstop();
    }
    
    // Motor B control
    if (motorBSpeed > 10) {
      motorBgo(abs(motorBSpeed));
    } else if (motorBSpeed < -10) {
      motorBrev(abs(motorBSpeed));
    } else {
      motorBstop();
    }
    
    request->send(200, "text/plain", "OK"); });

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  // Empty loop - all control is handled via web server callbacks
  delay(10);
}
