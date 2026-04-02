# Ultrasonic Radar — ESP32-CAM

A real-time radar system built with an ESP32-CAM, a servo motor, and an HC-SR04 ultrasonic sensor. The ESP32-CAM hosts a live web dashboard accessible from any browser on the same WiFi network, showing a sweeping radar display with distance readings and object detection alerts.



## What it does

- Sweeps a servo motor from 0° to 180° continuously
- Measures distance at each angle using the HC-SR04 ultrasonic sensor
- Serves a live radar dashboard over WiFi — no app needed, just a browser
- Displays a classic green radar sweep animation with detected object dots
- Shows real-time distance (cm) and angle (degrees)
- Triggers a red alert when an object is detected within 80 cm

---

## Hardware

| Part | Quantity |
|---|---|
| AI Thinker ESP32-CAM | 1 |
| SG92R servo motor (or SG90) | 1 |
| HC-SR04 ultrasonic sensor | 1 |
| Breadboard | 1 |
| 10kΩ resistor | 1 |
| 20kΩ resistor | 1 |
| USB cable (for ESP32-CAM power) | 1 |
| External 5V supply or phone charger (for servo + sensor power) | 1 |
| Jumper wires | several |

---

## Wiring



### Pin definitions

| Signal | ESP32-CAM Pin |
|---|---|
| Servo signal | IO12 |
| HC-SR04 Trig | IO13 |
| HC-SR04 Echo | IO14|



---

## Software setup

This project uses [PlatformIO](https://platformio.org/) in VS Code.

1. Clone the repository:
   ```bash
   git clone https://github.com/kamarxch/ultrasonic_radar.git
   ```

2. Create the file `include/secrets.h` with your WiFi credentials:
   ```cpp
   #define WIFI_SSID     "YOUR_WIFI_NAME"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   ```



## Dependencies

Managed automatically by PlatformIO via `platformio.ini`:

- [ESP32Servo](https://github.com/madhephaestus/ESP32Servo) `^0.13.0`
- WebServer (built-in with ESP32 Arduino framework)
- WiFi (built-in with ESP32 Arduino framework)

---

## License

MIT
