#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// ---- Freak Engineer PCB pins (TB6612FNG) ----
const int PWMA = 33;  // Motor A (right) speed
const int AIN2 = 25;
const int AIN1 = 26;
const int STBY = 27;  // Driver enable
const int BIN1 = 14;  // Motor B (left)
const int BIN2 = 12;
const int PWMB = 13;  // Motor B (left) speed

const char* DEVICE_NAME = "BoatControl-ESP32";

// Set to true if a motor spins the wrong way
const bool INVERT_RIGHT = false;
const bool INVERT_LEFT  = false;

int speedVal = 200;  // 0-255

// ---- PWM helper (works on ESP32 core 2.x and 3.x) ----
void pwmSetup(int pin) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, 1000, 8);
#else
  ledcSetup(pin == PWMA ? 0 : 1, 1000, 8);
  ledcAttachPin(pin, pin == PWMA ? 0 : 1);
#endif
}

void pwmWrite(int pin, int val) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, val);
#else
  ledcWrite(pin == PWMA ? 0 : 1, val);
#endif
}

// dir: 1 = forward, -1 = backward, 0 = stop
void rightMotor(int dir) {
  if (INVERT_RIGHT) dir = -dir;
  digitalWrite(AIN1, dir > 0);
  digitalWrite(AIN2, dir < 0);
  pwmWrite(PWMA, dir == 0 ? 0 : speedVal);
}

void leftMotor(int dir) {
  if (INVERT_LEFT) dir = -dir;
  digitalWrite(BIN1, dir > 0);
  digitalWrite(BIN2, dir < 0);
  pwmWrite(PWMB, dir == 0 ? 0 : speedVal);
}

void moveForward()  { leftMotor(1);  rightMotor(1);  }
void moveBackward() { leftMotor(-1); rightMotor(-1); }
void turnLeft()     { leftMotor(-1); rightMotor(1);  }  // pivot left
void turnRight()    { leftMotor(1);  rightMotor(-1); }  // pivot right
void stopBoat()     { leftMotor(0);  rightMotor(0);  }

void setup() {
  Serial.begin(115200);

  int pins[] = {AIN1, AIN2, BIN1, BIN2, STBY};
  for (int p : pins) pinMode(p, OUTPUT);
  pwmSetup(PWMA);
  pwmSetup(PWMB);

  digitalWrite(STBY, HIGH);  // enable driver
  stopBoat();

  SerialBT.begin(DEVICE_NAME);
  Serial.println("Bluetooth started: " + String(DEVICE_NAME));
}

void loop() {
  if (SerialBT.available()) {
    char cmd = toupper(SerialBT.read());
    Serial.println(cmd);

    switch (cmd) {
      case 'F': moveForward();  break;
      case 'B': moveBackward(); break;
      case 'L': turnLeft();     break;
      case 'R': turnRight();    break;
      case 'S': stopBoat();     break;

      // Speed: '0'-'9' (0 = slow, 9 = full)
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        speedVal = map(cmd - '0', 0, 9, 100, 255);
        break;
      case 'Q': speedVal = 255; break;  // some apps send 'q' for full speed
    }
  }
}
