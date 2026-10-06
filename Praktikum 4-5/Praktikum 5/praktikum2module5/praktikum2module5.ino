#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <DHT.h>

// ===============================
// KONFIGURASI WIFI
// ===============================
const char* ssid = "Bhap_you";
const char* password = "asdfghjk";

// ===============================
// KONFIGURASI PIN
// ===============================
#define DHTPIN D5
#define DHTTYPE DHT22
#define LED_PIN D1

DHT dht(DHTPIN, DHTTYPE);

// WebSocket pada port 81
WebSocketsServer webSocket = WebSocketsServer(81);

// Nilai PWM LED
int nilaiPWM = 0;

// Timer
unsigned long previousMillis = 0;
const long interval = 2000;

// ===============================
// HALAMAN WEB
// ===============================
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Kontrol LED PWM</title>

  <style>
    body {
      font-family: Arial;
      text-align: center;
      margin-top: 30px;
    }

    h1 {
      font-size: 28px;
    }

    .data {
      font-size: 20px;
      margin: 15px;
    }

    input[type=range] {
      width: 80%;
    }

    #pwmValue {
      font-size: 22px;
      font-weight: bold;
    }
  </style>
</head>

<body>

  <h1>Kontrol Intensitas Cahaya</h1>

  <div class="data">
    Suhu:
    <span id="suhuValue">--</span> °C
  </div>

  <div class="data">
    Kelembapan:
    <span id="humValue">--</span> %
  </div>

  <hr>

  <h2>Intensitas LED</h2>

  <input
    type="range"
    min="0"
    max="1023"
    value="0"
    id="pwmSlider"
    onchange="sendPWM(this.value)"
  >

  <p>
    Nilai PWM:
    <span id="pwmValue">0</span>
  </p>

<script>

  var websocket;

  function initWebSocket() {

    websocket = new WebSocket(
      'ws://' + window.location.hostname + ':81/'
    );

    websocket.onopen = function() {
      console.log("WebSocket Terhubung");
    };

    websocket.onclose = function() {
      console.log("WebSocket Terputus");
    };

    websocket.onmessage = function(event) {

      var data = JSON.parse(event.data);

      document.getElementById("suhuValue").innerText = data.suhu;
      document.getElementById("humValue").innerText = data.hum;

    };
  }

  function sendPWM(value) {

    document.getElementById("pwmValue").innerText = value;

    if (websocket.readyState === WebSocket.OPEN) {
      websocket.send("pwm," + value);
    }
  }

  window.onload = initWebSocket;

</script>

</body>
</html>
)=====";

// ===============================
// MENANGANI PESAN WEBSOCKET
// ===============================
void handleWebSocketMessage(uint8_t num, uint8_t *payload, size_t length) {

  String message = "";

  for (size_t i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Pesan diterima: ");
  Serial.println(message);

  // Mengecek perintah PWM
  if (message.startsWith("pwm,")) {

    String nilai = message.substring(4);

    nilaiPWM = nilai.toInt();

    // Membatasi nilai PWM 0 - 1023
    nilaiPWM = constrain(nilaiPWM, 0, 1023);

    // Mengatur intensitas LED
    analogWrite(LED_PIN, nilaiPWM);

    Serial.print("PWM LED: ");
    Serial.println(nilaiPWM);
  }
}

// ===============================
// EVENT WEBSOCKET
// ===============================
void webSocketEvent(
  uint8_t num,
  WStype_t type,
  uint8_t *payload,
  size_t length
) {

  switch (type) {

    case WStype_CONNECTED:
      Serial.println("Client WebSocket terhubung");
      break;

    case WStype_DISCONNECTED:
      Serial.println("Client WebSocket terputus");
      break;

    case WStype_TEXT:
      handleWebSocketMessage(num, payload, length);
      break;
  }
}

// ===============================
// SETUP
// ===============================
void setup() {

  Serial.begin(115200);

  dht.begin();

  // LED
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);

  // Mode WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println();
  Serial.println("Menghubungkan ke WiFi...");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("================================");
  Serial.println(" NODEMCU BERHASIL TERHUBUNG");
  Serial.println("================================");

  Serial.print("IP ADDRESS: ");
  Serial.println(WiFi.localIP());

  Serial.println("WebSocket Port: 81");

  // Memulai WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("WebSocket siap.");
}

// ===============================
// LOOP
// ===============================
void loop() {

  webSocket.loop();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {

    previousMillis = currentMillis;

    float suhu = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(suhu) || isnan(hum)) {

      Serial.println("Gagal membaca DHT22!");

      return;
    }

    // Membuat data JSON
    String json = "{";
    json += "\"suhu\":\"" + String(suhu, 1) + "\",";
    json += "\"hum\":\"" + String(hum, 1) + "\"";
    json += "}";

    // Mengirim data ke semua client WebSocket
    webSocket.broadcastTXT(json);

    Serial.print("Suhu: ");
    Serial.print(suhu);
    Serial.print(" C | Kelembapan: ");
    Serial.print(hum);
    Serial.print(" % | PWM: ");
    Serial.println(nilaiPWM);
  }
}