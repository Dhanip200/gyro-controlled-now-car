# Gyro-Controlled ESP-NOW Car

A small car driven by tilting your hand. One ESP32 reads an MPU6050 tilt sensor and sends the tilt wirelessly over **ESP-NOW** (direct ESP32-to-ESP32, no router). A second ESP32 on the car receives it and drives the motors.

```
[ Hand: ESP32 + MPU6050 ]  ── ESP-NOW ──>  [ Car: ESP32 + motor driver ]
```

## Folders

| Folder | Upload to | What it does |
|---|---|---|
| `gyro_remote/` | Hand ESP32 | Reads tilt, sends `throttle` and `turn` (-255..255) 20 times a second |
| `gyro_car/` | Car ESP32 | Receives tilt, mixes it into left/right wheel speeds |

## Wiring

### Hand controller (ESP32 Dev Module + MPU6050)

| MPU6050 | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### Car (ESP32 Dev Module + motor driver)

| Signal | ESP32 GPIO |
|---|---|
| IN1 (left forward) | 12 (boot pin, see note below) |
| IN2 (left backward) | 14 |
| IN3 (right forward) | 27 |
| IN4 (right backward) | 26 |
| ENA (left speed) | 13 |
| ENB (right speed) | 25 |

Change the car pins in the `#define` lines at the top of `gyro_car.ino`.

**GPIO 12 note:** GPIO 12 is a boot pin. If it is pulled HIGH while the ESP32 starts, the ESP32 won't boot. With a normal L298N it's fine, but if the car ever fails to boot or upload, unplug the IN1 wire, power on, then plug it back (or move IN1 to GPIO 33).

**Pins to avoid on ESP32:** 0, 1, 3 (USB serial / boot), 6–11 (connected to flash, will crash), 12 (boot pin: usable with care, see note above).

## How it controls

| Hand movement | Car |
|---|---|
| Tilt forward | Drive forward |
| Tilt back | Reverse |
| Tilt left / right | Turn left / right |
| Hold flat | Stop |

- **Mixing:** left wheel = throttle + turn, right wheel = throttle − turn, so you can curve while moving.
- **Deadzone:** small wobbles (under 40) are ignored.
- **Failsafe:** if no signal arrives for 0.5 s, the car brakes.

## Settings you can tweak

| Setting | File | Default | Meaning |
|---|---|---|---|
| `DEADZONE` | gyro_remote | 40 | How much tilt is ignored |
| `-11000, 11000` in `map()` | gyro_remote | about 45° | Tilt needed for full speed (smaller number = more sensitive) |
| `maxspeed` | gyro_car | 225 | Top PWM speed (max 255) |

## Compile

The Arduino IDE board is **ESP32 Dev Module** (ESP32 board package 3.x). No extra libraries are needed.

From the command line:
```
"C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile -b esp32:esp32:esp32 gyro_remote
"C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe" compile -b esp32:esp32:esp32 gyro_car
```

Both compiled with no errors on 11 Nov 2026.

If you use the older ESP32 board package (2.x), change the `onReceive` line in `gyro_car.ino` to:
```cpp
void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
```

## Testing steps

1. **Hand unit only:** upload `gyro_remote`, open the Serial Monitor at 115200 and tilt the sensor. The numbers should read about 0 when flat and reach ±255 when tilted.
   - Forward/back controlled by left/right tilt? Swap `ax` and `ay` in `loop()`.
   - Direction backwards? Add a minus: `-tiltToSpeed(ax)`.
2. **Car, wheels off the ground:** upload `gyro_car` and tilt the hand unit. If a wheel spins the wrong way, swap that motor's two wires.
3. **Failsafe:** unplug the hand unit. The car should stop within half a second.
4. **On the floor.**

## Status

- [x] Both sketches written and compiling
- [x] Final car pins chosen
- [ ] Tested on hardware
