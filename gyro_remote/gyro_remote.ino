#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>

// Broadcast address: no need to look up the car's MAC
uint8_t carAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct Packet {
  int16_t throttle;   // -255 (back) .. 255 (forward)
  int16_t turn;       // -255 (left) .. 255 (right)
};
Packet pkt;

const int MPU = 0x68;
const int DEADZONE = 40;   // ignore small hand wobble

int16_t ax, ay;

void readMPU() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);                 // start of accel data
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 4);
  ax = (Wire.read() << 8) | Wire.read();
  ay = (Wire.read() << 8) | Wire.read();
}

int tiltToSpeed(int16_t raw) {
  // about 45 degrees of tilt = full speed
  int v = map(raw, -11000, 11000, -255, 255);
  v = constrain(v, -255, 255);
  if (abs(v) < DEADZONE) v = 0;
  return v;
}

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // power register
  Wire.write(0);     // wake up
  Wire.endTransmission();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true);
  }
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, carAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void loop() {
  readMPU();
  pkt.throttle = tiltToSpeed(ax);   // tilt forward/back
  pkt.turn     = tiltToSpeed(ay);   // tilt left/right

  esp_now_send(carAddress, (uint8_t*)&pkt, sizeof(pkt));

  Serial.printf("ax=%d ay=%d -> throttle=%d turn=%d\n",
                ax, ay, pkt.throttle, pkt.turn);
  delay(50);   // 20 updates per second
}
