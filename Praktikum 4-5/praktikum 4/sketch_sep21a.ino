#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

// ==========================================
// WIFI
// ==========================================
const char* ssid = "Bhap_you";
const char* password = "asdfghjk";

// ==========================================
// PIN PERANGKAT
// ==========================================
// DHT22:
// -   -> GND
// +   -> 3V3
// OUT -> D5
#define DHTPIN D5
#define DHTTYPE DHT22

// LED:
// LED (+) -> D6
// LED (-) -> resistor 220 ohm -> GND
#define LED_PIN D6

DHT dht(DHTPIN, DHTTYPE);

// ==========================================
// WEBSERVER & WEBSOCKET
// ==========================================
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ==========================================
// DATA
// ==========================================
bool ledState = false;

String currentTemp = "--";
String currentHum = "--";

unsigned long previousMillis = 0;
const unsigned long sensorInterval = 2000;


// ==========================================
// HALAMAN WEB
// ==========================================
const char index_html[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>Smart Room</title>

<style>

body {
  font-family: Arial;
  text-align: center;
  background: #f2f2f2;
  padding: 20px;
}

.card {
  background: white;
  margin: 20px auto;
  padding: 20px;
  max-width: 300px;
  border-radius: 10px;
  box-shadow: 0 3px 10px rgba(0,0,0,0.15);
}

.value {
  font-size: 28px;
  font-weight: bold;
}

button {
  padding: 15px 30px;
  font-size: 18px;
  border: none;
  border-radius: 5px;
  color: white;
  cursor: pointer;
}

.btn-on {
  background-color: #4CAF50;
}

.btn-off {
  background-color: #f44336;
}

</style>

</head>


<body>

<h1>Smart Room</h1>


<div class="card">

<h2>
Suhu:
<span id="tempValue">--</span> °C
</h2>

</div>


<div class="card">

<h2>
Kelembapan:
<span id="humValue">--</span> %
</h2>

</div>


<div class="card">

<h2>
LED:
<span id="ledStatus">OFF</span>
</h2>

<button
id="toggleBtn"
class="btn-off"
onclick="toggleLed()">

Turn ON

</button>

</div>


<script>

var gateway =
`ws://${window.location.hostname}/ws`;

var websocket;


// ==========================================
// START WEBSOCKET
// ==========================================

window.addEventListener(
'load',
onLoad
);


function onLoad() {

  initWebSocket();

}


function initWebSocket() {

  websocket =
  new WebSocket(gateway);

  websocket.onopen =
  onOpen;

  websocket.onclose =
  onClose;

  websocket.onmessage =
  onMessage;

}


function onOpen() {

  console.log(
  "WebSocket terhubung"
  );

}


function onClose() {

  console.log(
  "WebSocket terputus"
  );

  setTimeout(
  initWebSocket,
  2000
  );

}


// ==========================================
// TOMBOL LED
// ==========================================

function toggleLed() {

  if (
    websocket.readyState === WebSocket.OPEN
  ) {

    websocket.send("toggle");

  }

}


// ==========================================
// MENERIMA DATA
// ==========================================

function onMessage(event) {

  var data =
  JSON.parse(event.data);


  // SUHU
  if (
    data.suhu !== undefined
  ) {

    document
    .getElementById("tempValue")
    .innerHTML =
    data.suhu;

  }


  // KELEMBAPAN
  if (
    data.hum !== undefined
  ) {

    document
    .getElementById("humValue")
    .innerHTML =
    data.hum;

  }


  // LED
  if (
    data.led !== undefined
  ) {

    var status =
    document.getElementById(
    "ledStatus"
    );

    var button =
    document.getElementById(
    "toggleBtn"
    );


    if (
      data.led == "1"
    ) {

      status.innerHTML =
      "ON";

      button.innerHTML =
      "Turn OFF";

      button.className =
      "btn-on";

    }

    else {

      status.innerHTML =
      "OFF";

      button.innerHTML =
      "Turn ON";

      button.className =
      "btn-off";

    }

  }

}

</script>

</body>

</html>

)rawliteral";


// ==========================================
// KIRIM DATA WEBSOCKET
// ==========================================

void notifyClients() {

  String json = "{";

  json += "\"led\":\"";
  json += String(ledState ? 1 : 0);
  json += "\",";

  json += "\"suhu\":\"";
  json += currentTemp;
  json += "\",";

  json += "\"hum\":\"";
  json += currentHum;

  json += "\"}";

  ws.textAll(json);

}


// ==========================================
// PESAN DARI BROWSER
// ==========================================

void handleWebSocketMessage(
void *arg,
uint8_t *data,
size_t len
) {

  AwsFrameInfo *info =
  (AwsFrameInfo*)arg;


  if (
    info->final &&
    info->index == 0 &&
    info->len == len &&
    info->opcode == WS_TEXT
  ) {

    data[len] = 0;


    if (
      strcmp(
        (char*)data,
        "toggle"
      ) == 0
    ) {

      ledState =
      !ledState;


      digitalWrite(
        LED_PIN,
        ledState ?
        HIGH :
        LOW
      );


      notifyClients();

    }

  }

}


// ==========================================
// EVENT WEBSOCKET
// ==========================================

void onEvent(
AsyncWebSocket *server,
AsyncWebSocketClient *client,
AwsEventType type,
void *arg,
uint8_t *data,
size_t len
) {

  switch(type) {

    case WS_EVT_CONNECT:

      // Tidak menampilkan data sensor
      // di Serial Monitor

      notifyClients();

      break;


    case WS_EVT_DISCONNECT:

      break;


    case WS_EVT_DATA:

      handleWebSocketMessage(
        arg,
        data,
        len
      );

      break;


    default:

      break;

  }

}


// ==========================================
// SETUP
// ==========================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  // LED
  pinMode(
    LED_PIN,
    OUTPUT
  );

  digitalWrite(
    LED_PIN,
    LOW
  );


  // DHT22
  dht.begin();


  // ========================================
  // WIFI
  // ========================================

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    ssid,
    password
  );


  // Tunggu sampai WiFi terhubung
  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);

  }


  // ========================================
  // IP ADDRESS
  // ========================================

  Serial.println();

  Serial.println(
    "================================"
  );

  Serial.println(
    "   NODEMCU BERHASIL TERHUBUNG"
  );

  Serial.println(
    "================================"
  );

  Serial.print(
    "IP ADDRESS: "
  );

  Serial.println(
    WiFi.localIP()
  );

  Serial.println();

  Serial.print(
    "Buka browser: http://"
  );

  Serial.println(
    WiFi.localIP()
  );

  Serial.println();


  // ========================================
  // WEBSOCKET
  // ========================================

  ws.onEvent(onEvent);

  server.addHandler(&ws);


  // ========================================
  // WEB SERVER
  // ========================================

  server.on(
    "/",
    HTTP_GET,
    [](AsyncWebServerRequest *request) {

      request->send_P(
        200,
        "text/html",
        index_html
      );

    }
  );


  server.begin();

}


// ==========================================
// LOOP
// ==========================================

void loop() {

  ws.cleanupClients();


  // ========================================
  // BACA DHT22 SETIAP 2 DETIK
  // ========================================

  if (
    millis() - previousMillis >=
    sensorInterval
  ) {

    previousMillis =
    millis();


    float temperature =
    dht.readTemperature();


    float humidity =
    dht.readHumidity();


    if (
      !isnan(temperature) &&
      !isnan(humidity)
    ) {

      currentTemp =
      String(
        temperature,
        1
      );


      currentHum =
      String(
        humidity,
        1
      );


      // Kirim data ke browser
      // TANPA mencetaknya ke Serial Monitor
      notifyClients();

    }

  }

}