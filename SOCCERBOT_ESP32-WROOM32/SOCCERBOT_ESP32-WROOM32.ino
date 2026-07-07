#include "BluetoothSerial.h"
BluetoothSerial serialBT;

// ===== Motor Pin Configuration =====
const int ENA = 18;   // Right Motor PWM
const int IN1 = 19;   // Right Motor Dir1
const int IN2 = 21;   // Right Motor Dir2
const int ENB = 5;    // Left Motor PWM
const int IN3 = 17;   // Left Motor Dir1
const int IN4 = 16;   // Left Motor Dir2

// ===== Speed Settings =====
int Speed = 255;          // Default speed (0-255)
const int TURN_SPEED = 180;  // Reduced speed for turns
const float CURVE_RATIO = 0.7; // Speed ratio for curves

void setup() {
  Serial.begin(115200);
  serialBT.begin("KINGFINIX");

  // Motor pins setup
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // PWM setup (kompatibel semua versi ESP32)
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcAttach(ENA, 5000, 8);
    ledcAttach(ENB, 5000, 8);
  #else
    ledcSetup(0, 5000, 8);
    ledcSetup(1, 5000, 8);
    ledcAttachPin(ENA, 0);
    ledcAttachPin(ENB, 1);
  #endif

  stop();
  Serial.println("Soccer Bot READY");
}

void loop() {
  if (serialBT.available()) {
    handleCommand(serialBT.read());
  }
}

// ===== Movement Functions =====
void go_forward() {
  setMotorDirection(LOW, HIGH, LOW, HIGH);
  setMotorSpeed(Speed, Speed);
  Serial.println("FORWARD");
}

void go_backward() {
  setMotorDirection(HIGH, LOW, HIGH, LOW);
  setMotorSpeed(Speed, Speed);
  Serial.println("BACKWARD");
}

void turn_left() {
  setMotorDirection(HIGH, LOW, LOW, HIGH);
  setMotorSpeed(TURN_SPEED, TURN_SPEED);
  Serial.println("TURN LEFT");
}

void turn_right() {
  setMotorDirection(LOW, HIGH, HIGH, LOW);
  setMotorSpeed(TURN_SPEED, TURN_SPEED);
  Serial.println("TURN RIGHT");
}

void forward_left() {
  setMotorDirection(LOW, HIGH, LOW, HIGH);
  setMotorSpeed(CURVE_RATIO * Speed, Speed);
  Serial.println("FORWARD LEFT CURVE");
}

void forward_right() {
  setMotorDirection(HIGH, LOW, HIGH, LOW);
  setMotorSpeed(Speed, CURVE_RATIO * Speed);
  Serial.println("FORWARD RIGHT CURVE");             
}

// HIGH, LOW, HIGH, LOW


// LOW, HIGH, LOW, HIGH

void backward_left() {
  setMotorDirection(LOW, HIGH, LOW, HIGH);
  setMotorSpeed(Speed, Speed * CURVE_RATIO);
  Serial.println("BACKWARD LEFT CURVE");
}

void backward_right() {
  setMotorDirection(HIGH, LOW, HIGH, LOW);
  setMotorSpeed(Speed * CURVE_RATIO, Speed);
  Serial.println("BACKWARD RIGHT CURVE");
}

void stop() {
  setMotorDirection(LOW, LOW, LOW, LOW);
  setMotorSpeed(0, 0);
  Serial.println("STOP");
}

// ===== Helper Functions =====
void setMotorDirection(int right1, int right2, int left1, int left2) {
  digitalWrite(IN1, right1);
  digitalWrite(IN2, right2);
  digitalWrite(IN3, left1);
  digitalWrite(IN4, left2);
}

void setMotorSpeed(int rightSpeed, int leftSpeed) {
  rightSpeed = constrain(rightSpeed, 0, 250);
  leftSpeed = constrain(leftSpeed, 0, 250);
  #if ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(ENA, rightSpeed);
    ledcWrite(ENB, leftSpeed);
  #else
    ledcWrite(0, rightSpeed);
    ledcWrite(1, leftSpeed);
  #endif
}

void handleCommand(char cmd) {
  switch(cmd) {
    case 'F': go_forward(); break;
    case 'B': go_backward(); break;
    case 'L': turn_left(); break;
    case 'R': turn_right(); break;
    case 'H': forward_right(); break;
    case 'G': forward_left(); break;
    case 'J': backward_right(); break;
    case 'I': backward_left(); break;
    case 'S': stop(); break;
    // Speed control
    case '0': Speed = 100; break;
    case '1': Speed = 130; break;
    case '2': Speed = 160; break;
    case '3': Speed = 190; break;
    case '4': Speed = 220; break;
    case '5': Speed = 250; break;
    case 'q': Speed = 255; break;
  }
  Serial.print("Speed: "); Serial.println(Speed);
}