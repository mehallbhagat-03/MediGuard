# MediGuard

## Smart IoT-Based Medicine Reminder and Emergency Alert System

MediGuard is an ESP32-based smart medicine reminder system designed to help users manage scheduled medication reminders and provide an emergency alert mechanism.

The system combines automated medicine compartments, IR-based medicine detection, an OLED display, RTC-based scheduling, Wi-Fi connectivity, and Blynk IoT notifications.

---

## Problem Statement

Forgetting scheduled medication can lead to missed doses, especially for elderly users and people who require regular medication.

MediGuard provides an automated reminder system that alerts the user at scheduled medication times and detects whether the medicine has been taken.

The system also provides an emergency help button and an Away Mode for situations where scheduled reminders should be temporarily skipped.

---

## Key Features

- Automated medicine compartment opening using servo motors
- Two independent medicine compartments
- RTC-based scheduled medicine reminders
- IR sensor-based medicine-taking detection
- Buzzer and LED medication alerts
- OLED display for system status
- ESP32 Wi-Fi connectivity
- Blynk IoT notifications
- Medicine Taken notification
- Medicine Not Taken notification
- Emergency Alert notification
- Away Mode
- Medicine status monitoring through Blynk
- Emergency help button
- Real-time system status display

---

## System Working

### Normal Medicine Reminder

1. The RTC continuously keeps track of the current time.
2. At the scheduled medicine time, the corresponding compartment opens.
3. The buzzer and red LED alert the user.
4. The OLED displays the medicine reminder.
5. The IR sensor detects whether the user takes the medicine.
6. If the medicine is detected as taken, the alert stops and the system records the status.
7. If the medicine is not taken within the defined reminder sequence, the system records it as not taken and closes the compartment.

### Away Mode

Away Mode can be activated using a physical button.

When Away Mode is active:

- Scheduled medicine reminders are suppressed.
- Medicine compartments remain closed.
- The buzzer does not activate for scheduled reminders.
- IR sensors are ignored for scheduled medicine detection.
- The medicine status is recorded as skipped due to Away Mode.

### Emergency Mode

The user can press the emergency help button to activate an emergency alert.

When activated:

- Blue LED turns ON.
- Buzzer turns ON.
- Red LED turns OFF.
- OLED displays an emergency message.
- Blynk sends an emergency notification.

The emergency condition remains active until the help button is pressed again after the minimum emergency duration.

---

## Hardware Components

- ESP32 Development Board
- SG90 Servo Motors × 2
- IR Sensors × 2
- DS3231 RTC Module
- OLED Display
- Buzzer
- Red LED
- Blue LED
- Push Buttons
- Resistors
- Medicine compartments
- External 5V supply for servo motors

---

## Software and Technologies

- Arduino IDE
- Embedded C/C++
- ESP32
- Blynk IoT
- I2C communication
- RTC-based scheduling

---

## Pin Configuration

| Component | ESP32 GPIO |
|---|---:|
| Servo 1 | GPIO 18 |
| IR Sensor 1 | GPIO 32 |
| Servo 2 | GPIO 19 |
| IR Sensor 2 | GPIO 33 |
| Buzzer | GPIO 25 |
| Red LED | GPIO 26 |
| Blue LED | GPIO 4 |
| Emergency Button | GPIO 13 |
| Away Mode Button | GPIO 14 |
| OLED SDA | GPIO 21 |
| OLED SCL | GPIO 22 |

---

## Blynk Integration

MediGuard uses Blynk IoT for remote notifications and medicine status monitoring.

The system supports notifications for:

- Medicine Taken
- Medicine Not Taken
- Emergency Alert
- Away Mode ON
- Away Mode OFF

The mobile dashboard also displays the current status of:

- Medicine 1
- Medicine 2

using Blynk virtual datastreams.

---

## Medicine Status

The Blynk dashboard uses:

- V0 → Medicine 1 Status
- V1 → Medicine 2 Status

Possible displayed states include:

- READY
- TAKEN
- NOT TAKEN
- SKIPPED - AWAY

---

## Project Structure

```text
MediGuard/
│
├── MediGuard_Complete.ino
└── README.md
