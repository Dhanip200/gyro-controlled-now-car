#include <WiFi.h>
#include <esp_now.h>

// ---- PINS: change these to match your wiring ----
#define IN1  27   // left motor
#define IN2  26
#define IN3  25   // right motor
#define IN4  33
#define ENA  14   // left speed (PWM)
#define ENB  32   // right speed (PWM)
#define TRIG 18
#define ECHO 19   // HC-SR04 echo is 5V: use a voltage divider!

int mindistance = 25;   // cm
int maxspeed    = 225;

struct Packet {
  int16_t throttle;
  int16_t turn;
};
volatile int16_t throttle = 0, turn = 0;
volatile unsigned long lastPacket = 0;

// Called automatically whenever a packet arrives
// (Old ESP32 board package 2.x? use: void onReceive(const uint8_t *mac, const uint8_t *data, int len) )
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(Packet)) return;
  Packet p;
  memcpy(&p, data, sizeof(p));
  throttle = p.throttle;
  turn = p.turn;
  lastPacket = millis();
}

// speed: -255..255  (negative = backwards)
void setMotor(int inA, int inB, int en, int speed) {
  if (speed > 0)      { digitalWrite(inA, HIGH); digitalWrite(inB, LOW);  }
  else if (speed < 0) { digitalWrite(inA, LOW);  digitalWrite(inB, HIGH); }
  else                { digitalWrite(inA, HIGH); digitalWrite(inB, HIGH); } // brake
  analogWrite(en, min(abs(speed), maxspeed));
}

void brake() {
  setMotor(IN1, IN2, ENA, 0);
  setMotor(IN3, IN4, ENB, 0);
}

long getdistance() {
  digitalWrite(TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long duration = pulseIn(ECHO, HIGH, 25000);  // timeout so it never freezes
  if (duration == 0) return 999;               // no echo = nothing in front
  return duration * 0.0343 / 2;
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  brake();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true);
  }
  esp_now_register_recv_cb(onReceive);
}

void loop() {
  // Failsafe: no signal for 0.5 s -> stop
  if (millis() - lastPacket > 500) {
    brake();
    return;
  }

  int t = throttle;
  int r = turn;

  // Obstacle stop: block forward motion only, reversing is still allowed
  if (t > 0 && getdistance() < mindistance) t = 0;

  // Mix: tilt right -> left wheel faster, right wheel slower
  int left  = constrain(t + r, -255, 255);
  int right = constrain(t - r, -255, 255);

  setMotor(IN1, IN2, ENA, left);
  setMotor(IN3, IN4, ENB, right);

  delay(20);
}
