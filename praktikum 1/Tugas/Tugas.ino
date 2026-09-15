/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://docs.arduino.cc/hardware/

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  https://docs.arduino.cc/built-in-examples/basics/Blink/
*/

// the setup function runs once when you press reset or power the board
const int buttonPin = 4;
const int ledPin = 5;

int buttonState = 0;
int lastButtonState = LOW; // Menyimpan status tombol sebelumnya
bool ledStatus = false;     // Menyimpan status LED (true = ON, false = OFF)

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  buttonState = digitalRead(buttonPin);

  // Deteksi ketika tombol BARU SAJA ditekan (perubahan dari LOW ke HIGH)
  if (buttonState == HIGH && lastButtonState == LOW) {
    ledStatus = !ledStatus; // Membalikkan status LED (ON jadi OFF, OFF jadi ON)
    digitalWrite(ledPin, ledStatus ? HIGH : LOW);

    if (ledStatus) {
      Serial.println("Tombol ditekan! -> LED ON");
    } else {
      Serial.println("Tombol ditekan! -> LED OFF");
    }

    delay(50); // Debouncing singkat untuk menghindari pembacaan ganda
  }

  // Simpan status tombol saat ini untuk iterasi berikutnya
  lastButtonState = buttonState;
}