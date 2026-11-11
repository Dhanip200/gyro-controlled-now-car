#include <WiFi.h>
#include <esp_now.h>

// ---- PINS: change these to match your wiring ----
#define IN1  12   // left forward  (boot pin: see README)
#define IN2  14   // left backward
#define IN3  27   // right forward
#define IN4  26   // right backward
#define ENA  13   // left speed (PWM)
#define ENB  25   // right speed (PWM)

int maxspeed = 225;

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

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
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

  // Mix: tilt right -> left wheel faster, right wheel slower
  int left  = constrain(t + r, -255, 255);
  int right = constrain(t - r, -255, 255);

  setMotor(IN1, IN2, ENA, left);
  setMotor(IN3, IN4, ENB, right);

  delay(20);
}
