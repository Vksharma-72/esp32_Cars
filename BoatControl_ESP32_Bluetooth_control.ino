
#include <Bluepad32.h>

// ===== Confirmed ESP32 / TB6612FNG GPIO mapping =====
const int PWMA = 33;  // Right motor PWM
const int AIN2 = 25;
const int AIN1 = 26;

const int STBY = 27;

const int BIN1 = 14;  // Left motor direction
const int BIN2 = 12;
const int PWMB = 13;  // Left motor PWM

// Change these if a motor runs in reverse.
const bool INVERT_RIGHT = false;
const bool INVERT_LEFT  = false;

// ===== Driving configuration =====
const int MAX_SPEED = 200;  // Increase up to 255 after testing
const int DEADZONE = 60;    // Ignore joystick drift

ControllerPtr gamepad = nullptr;

// ===== PWM support for ESP32 Arduino core 2.x / 3.x =====
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

// speed: negative = reverse, positive = forward
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

  Serial.println("Stellaris gamepad connected!");
  Serial.println(ctl->getModelName());
}

void onDisconnectedController(ControllerPtr ctl) {
  if (gamepad == ctl) {
    gamepad = nullptr;
  }

  stopCar();
  Serial.println("Gamepad disconnected - motors stopped");
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

  Serial.println("Gamepad car ready. Pair the Stellaris.");
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

  // Bluepad32 joystick values usually range from -512 to +512.
  // Pushing forward usually makes Y negative.
  int x = gamepad->axisX();
  int y = gamepad->axisY();

  if (abs(x) < DEADZONE) x = 0;
  if (abs(y) < DEADZONE) y = 0;

  // Convert joystick positions into motor speeds.
  int throttle = map(
    constrain(y, -512, 512),
    -512, 512, MAX_SPEED, -MAX_SPEED
  );

  int steering = map(
    constrain(x, -512, 512),
    -512, 512, -MAX_SPEED, MAX_SPEED
  );

  // Differential steering.
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
