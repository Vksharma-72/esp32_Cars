
#include <Bluepad32.h>

// ===== Confirmed TB6612FNG GPIO mapping =====
const int PWMA = 33;  // Right motor PWM
const int AIN2 = 25;
const int AIN1 = 26;

const int STBY = 27;

const int BIN1 = 14;  // Left motor direction
const int BIN2 = 12;
const int PWMB = 13;  // Left motor PWM

const bool INVERT_RIGHT = false;
const bool INVERT_LEFT  = false;

// ===== Driving settings =====
const int MAX_SPEED = 200;  // Range: 0-255
const int DEADZONE = 60;

ControllerPtr gamepad = nullptr;

// ===== PWM compatibility: ESP32 core 2.x / 3.x =====
void pwmSetup(int pin) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pin, 1000, 8);
#else
  int channel = (pin == PWMA) ? 0 : 1;
  ledcSetup(channel, 1000, 8);
  ledcAttachPin(pin, channel);
#endif
}

void pwmWrite(int pin, int value) {
  value = constrain(value, 0, 255);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pin, value);
#else
  int channel = (pin == PWMA) ? 0 : 1;
  ledcWrite(channel, value);
#endif
}

void rightMotor(int speed) {
  if (INVERT_RIGHT) speed = -speed;

  digitalWrite(AIN1, speed > 0 ? HIGH : LOW);
  digitalWrite(AIN2, speed < 0 ? HIGH : LOW);
  pwmWrite(PWMA, abs(speed));
}

void leftMotor(int speed) {
  if (INVERT_LEFT) speed = -speed;

  digitalWrite(BIN1, speed > 0 ? HIGH : LOW);
  digitalWrite(BIN2, speed < 0 ? HIGH : LOW);
  pwmWrite(PWMB, abs(speed));
}

void stopCar() {
  rightMotor(0);
  leftMotor(0);
}

void onConnectedController(ControllerPtr ctl) {
  gamepad = ctl;
  stopCar();
  Serial.println("Stellaris controller connected!");
}

void onDisconnectedController(ControllerPtr ctl) {
  if (gamepad == ctl) {
    gamepad = nullptr;
  }

  stopCar();
  Serial.println("Controller disconnected - car stopped");
}

void setup() {
  Serial.begin(115200);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  pwmSetup(PWMA);
  pwmSetup(PWMB);

  digitalWrite(STBY, HIGH);
  stopCar();

  BP32.setup(
    &onConnectedController,
    &onDisconnectedController
  );

  Serial.println("Gamepad car ready!");
}

void loop() {
  BP32.update();

  if (gamepad == nullptr ||
      !gamepad->isConnected() ||
      !gamepad->hasData()) {
    stopCar();
    delay(10);
    return;
  }

  // LEFT joystick: forward/backward only
  int ly = gamepad->axisY();

  // RIGHT joystick: left/right only
  int rx = gamepad->axisRX();

  if (abs(ly) < DEADZONE) ly = 0;
  if (abs(rx) < DEADZONE) rx = 0;

  // Forward is normally negative LY.
  int throttle = map(
    constrain(ly, -512, 512),
    -512, 512, MAX_SPEED, -MAX_SPEED
  );

  // Right-stick horizontal axis controls steering.
  // Positive RX turns right; negative RX turns left.
  int steering = map(
    constrain(rx, -512, 512),
    -512, 512, -MAX_SPEED, MAX_SPEED
  );

  // Mix forward/backward and steering.
  int leftSpeed = constrain(
    throttle + steering, -MAX_SPEED, MAX_SPEED
  );

  int rightSpeed = constrain(
    throttle - steering, -MAX_SPEED, MAX_SPEED
  );

  leftMotor(leftSpeed);
  rightMotor(rightSpeed);

  delay(10);
}
