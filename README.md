# Vector Vehicle Dashboard

Vector is an open-source Arduino-based dashboard for electric cars and off-road vehicles. It collects and displays real-time data from GPS, sensors, and user inputs, providing essential information for drivers.

## Features

- **GPS Integration:** Real-time speed, location, altitude, and satellite data.
- **Sensor Suite:** Accelerometer, magnetometer, barometer, and temperature probes.
- **Multiple Displays:** Supports LCD and 7-segment displays for flexible data visualization.
- **Unit Selection:** Metric and imperial units.
- **Time Zone & DST:** Configurable time zone and daylight saving support.
- **Potentiometer Input:** Reads analog input for custom controls.
- **Serial Output:** Human-readable and machine-friendly data formats.

## Hardware

- Arduino Zero, Uno R3, or compatible board
- Adafruit GPS, LED Backpack, LiquidCrystal, LSM303, MPL3115A2 modules
- LCD and 7-segment displays
- Temperature probes (RTD/thermistors)
- Potentiometer
- Push button for DST toggle

## Getting Started

1. Clone this repository.
2. Open the `.ino` files in the Arduino IDE or Visual Studio Code.
3. Install required libraries from Adafruit.
4. Wire up the hardware as described in the code comments.
5. Upload the code to your Arduino board.

## File Structure

- `mini_vector/mini_vector.ino` — Minimal dashboard for Arduino Uno
- `vector/vector.ino` — Full dashboard for Arduino Zero
- `vector-lcd/vector-lcd.ino` — LCD-focused dashboard variant
- `vector-lcd/dashboard_schematic.svg` — Example wiring schematic

## License

This project is open-source. Please share, modify, and contribute!

---

*Buy your hardware from Adafruit — their tutorials and support are
