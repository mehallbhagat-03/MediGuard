#define BLYNK_PRINT Serial

// ======================================================
// BLYNK
// ======================================================

#define BLYNK_TEMPLATE_ID   "TMPL3ghtGSzC"
#define BLYNK_TEMPLATE_NAME "MEDIGUARD"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"


// ======================================================
// LIBRARIES
// ======================================================

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <RTClib.h>
#include <ESP32Servo.h>
#include <U8g2lib.h>


// ======================================================
// WIFI
// ======================================================

char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";


// ======================================================
// PIN DEFINITIONS
// ======================================================

// ---------------- MEDICINE 1 ----------------

#define SERVO1_PIN 18
#define IR1_PIN    32


// ---------------- MEDICINE 2 ----------------

#define SERVO2_PIN 19
#define IR2_PIN    33


// ---------------- COMMON ----------------

#define BUZZER_PIN  25
#define RED_LED_PIN 26


// ---------------- EMERGENCY ----------------

#define BLUE_LED_PIN     4
#define HELP_BUTTON_PIN 13


// ---------------- AWAY MODE ----------------

#define AWAY_BUTTON_PIN 14


// ---------------- OLED ----------------

#define OLED_SDA 21
#define OLED_SCL 22


// ======================================================
// OLED
// ======================================================

U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
  U8G2_R0,
  U8X8_PIN_NONE
);


// ======================================================
// RTC
// ======================================================

RTC_DS3231 rtc;


// ======================================================
// SERVOS
// ======================================================

Servo servo1;
Servo servo2;

const int SERVO_CLOSED_ANGLE = 0;
const int SERVO_OPEN_ANGLE   = 90;


// ======================================================
// IR SENSOR
// ======================================================

// Change LOW to HIGH if your IR sensor detects a hand
// when its output is HIGH.

const int IR_DETECTED = LOW;


// ======================================================
// MEDICINE TIMES
// ======================================================

// Medicine 1 = 22:53

const int MED1_HOUR   =22;
const int MED1_MINUTE = 53;


// Medicine 2 = 22:54

const int MED2_HOUR   = 22;
const int MED2_MINUTE = 54;


// ======================================================
// MEDICINE TIMINGS
// ======================================================

const unsigned long FIRST_ALERT_TIME  = 10000;  // 10 sec
const unsigned long WAIT_TIME         = 10000;  // 10 sec
const unsigned long SECOND_ALERT_TIME = 5000;   // 5 sec
const unsigned long CLOSE_DELAY       = 5000;   // 5 sec
const unsigned long IR_START_DELAY    = 2000;   // Wait 2 sec after lid opens before IR sensing


// ======================================================
// EMERGENCY TIMING
// ======================================================

const unsigned long MIN_EMERGENCY_TIME = 10000;


// ======================================================
// MEDICINE STATES
// ======================================================

enum MedicineState
{
  MED_IDLE,
  MED_FIRST_ALERT,
  MED_WAITING,
  MED_SECOND_ALERT,
  MED_TAKEN,
  MED_MISSED,
  MED_AWAY
};


// Medicine 1 state

MedicineState medicine1State = MED_IDLE;


// Medicine 2 state

MedicineState medicine2State = MED_IDLE;


// ======================================================
// MEDICINE STATE TIMERS
// ======================================================

unsigned long medicine1StateStart = 0;
unsigned long medicine2StateStart = 0;


// ======================================================
// DAILY TRIGGER PROTECTION
// ======================================================

// Medicine 1

int med1LastDay   = -1;
int med1LastMonth = -1;
int med1LastYear  = -1;


// Medicine 2

int med2LastDay   = -1;
int med2LastMonth = -1;
int med2LastYear  = -1;


// ======================================================
// EMERGENCY VARIABLES
// ======================================================

bool emergencyActive = false;

bool lastHelpButtonState = HIGH;

unsigned long emergencyStartTime = 0;


// ======================================================
// AWAY MODE VARIABLES
// ======================================================

bool awayMode = false;

bool lastAwayButtonState = HIGH;


// ======================================================
// BLYNK RECONNECT TIMER
// ======================================================

unsigned long lastBlynkCheck = 0;


// ======================================================
// BUTTON DEBOUNCE
// ======================================================

unsigned long lastHelpButtonPress = 0;
unsigned long lastAwayButtonPress = 0;

const unsigned long BUTTON_DEBOUNCE_TIME = 250;


// ======================================================
// OLED - NORMAL SCREEN
// ======================================================

void showNormalScreen(DateTime now)
{
  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB08_tr);

  oled.drawStr(20, 12, "MEDIGUARD");

  oled.setFont(u8g2_font_6x10_tr);

  char dateText[20];

  sprintf(
    dateText,
    "%02d/%02d/%04d",
    now.day(),
    now.month(),
    now.year()
  );

  oled.drawStr(35, 30, dateText);


  char timeText[20];

  sprintf(
    timeText,
    "%02d:%02d:%02d",
    now.hour(),
    now.minute(),
    now.second()
  );

  oled.drawStr(35, 45, timeText);

  oled.drawStr(42, 60, "READY");

  oled.sendBuffer();
}


// ======================================================
// OLED - PILL TIME
// ======================================================

void showPillTimeScreen()
{
  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB10_tr);

  oled.drawStr(25, 18, "PILL TIME");

  oled.setFont(u8g2_font_6x10_tr);

  oled.drawStr(25, 38, "TAKE MEDICINE");

  oled.sendBuffer();
}


// ======================================================
// OLED - MEDICINE TAKEN
// ======================================================

void showMedicineTakenScreen()
{
  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB08_tr);

  oled.drawStr(15, 18, "MEDICINE");
  oled.drawStr(25, 35, "TAKEN");

  oled.setFont(u8g2_font_6x10_tr);

  oled.drawStr(20, 55, "THANK YOU");

  oled.sendBuffer();
}


// ======================================================
// OLED - MEDICINE NOT TAKEN
// ======================================================

void showMedicineMissedScreen()
{
  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB08_tr);

  oled.drawStr(15, 18, "MEDICINE");
  oled.drawStr(10, 35, "NOT TAKEN");

  oled.setFont(u8g2_font_6x10_tr);

  oled.drawStr(20, 55, "PLEASE CHECK");

  oled.sendBuffer();
}


// ======================================================
// OLED - AWAY MODE
// ======================================================

void showAwayScreen()
{
  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB08_tr);

  oled.drawStr(28, 20, "AWAY MODE");

  oled.setFont(u8g2_font_6x10_tr);

  oled.drawStr(20, 38, "REMINDERS");

  oled.drawStr(20, 52, "PAUSED");

  oled.sendBuffer();
}


// ======================================================
// OLED - EMERGENCY
// ======================================================

void showEmergencyScreen()
{
  oled.clearBuffer();

  oled.setFont(u8g2_font_ncenB08_tr);

  oled.drawStr(18, 20, "EMERGENCY");

  oled.drawStr(28, 38, "ALERT!");

  oled.setFont(u8g2_font_6x10_tr);

  oled.drawStr(18, 58, "HELP REQUESTED");

  oled.sendBuffer();
}


// ======================================================
// BLYNK - MEDICINE TAKEN
// ======================================================

void sendMedicineTakenNotification(int medicineNumber)
{
  if (!Blynk.connected())
  {
    Serial.println(
      "Blynk not connected - taken notification skipped."
    );

    return;
  }

  char message[80];

  sprintf(
    message,
    "Medicine %d has been taken.",
    medicineNumber
  );

  Serial.println(
    "Sending medicine taken notification..."
  );

  Blynk.logEvent(
    "medicine_taken",
    message
  );

  for (int i = 0; i < 5; i++)
  {
    Blynk.run();
    delay(20);
  }

  Serial.println(
    "Medicine taken notification sent."
  );
}


// ======================================================
// BLYNK - MEDICINE NOT TAKEN
// ======================================================

void sendMedicineMissedNotification(int medicineNumber)
{
  if (!Blynk.connected())
  {
    Serial.println(
      "Blynk not connected - missed notification skipped."
    );

    return;
  }

  char message[80];

  sprintf(
    message,
    "Medicine %d was not taken.",
    medicineNumber
  );

  Serial.println(
    "Sending medicine NOT TAKEN notification..."
  );

  Blynk.logEvent(
    "medicine_not_taken",
    message
  );

  for (int i = 0; i < 5; i++)
  {
    Blynk.run();
    delay(20);
  }

  Serial.println(
    "Medicine not taken notification sent."
  );
}


// ======================================================
// BLYNK - EMERGENCY
// ======================================================

void sendEmergencyNotification()
{
  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "SENDING EMERGENCY ALERT"
  );

  Serial.println(
    "================================"
  );


  if (!Blynk.connected())
  {
    Serial.println(
      "Blynk not connected - emergency notification failed."
    );

    return;
  }


  Blynk.logEvent(
    "emergency_alert",
    "EMERGENCY ALERT - Help requested!"
  );


  for (int i = 0; i < 5; i++)
  {
    Blynk.run();
    delay(20);
  }


  Serial.println(
    "Emergency notification sent."
  );
}


// ======================================================
// BLYNK - AWAY MODE ON
// ======================================================

void sendAwayModeOnNotification()
{
  if (!Blynk.connected())
  {
    Serial.println(
      "Blynk not connected - Away Mode ON notification skipped."
    );

    return;
  }


  Serial.println(
    "Sending Away Mode ON notification..."
  );


  Blynk.logEvent(
    "away_mode_on",
    "MEDIGUARD: Away Mode activated. Medicine reminders are paused."
  );


  for (int i = 0; i < 5; i++)
  {
    Blynk.run();
    delay(20);
  }


  Serial.println(
    "Away Mode ON notification sent."
  );
}


// ======================================================
// BLYNK - AWAY MODE OFF
// ======================================================

void sendAwayModeOffNotification()
{
  if (!Blynk.connected())
  {
    Serial.println(
      "Blynk not connected - Away Mode OFF notification skipped."
    );

    return;
  }


  Serial.println(
    "Sending Away Mode OFF notification..."
  );


  Blynk.logEvent(
    "away_mode_off",
    "MEDIGUARD: Away Mode deactivated. Medicine reminders resumed."
  );


  for (int i = 0; i < 5; i++)
  {
    Blynk.run();
    delay(20);
  }


  Serial.println(
    "Away Mode OFF notification sent."
  );
}


// ======================================================
// BLYNK - MEDICINE STATUS DATASTREAMS
// ======================================================

// V0 = Medicine 1 Status
// V1 = Medicine 2 Status
// These show the latest status only.

void updateMedicineStatus(int medicineNumber, const char* status)
{
  if (!Blynk.connected())
  {
    Serial.println("Blynk not connected - status update skipped.");
    return;
  }

  if (medicineNumber == 1)
  {
    Blynk.virtualWrite(V0, status);
  }
  else if (medicineNumber == 2)
  {
    Blynk.virtualWrite(V1, status);
  }

  Serial.print("Medicine ");
  Serial.print(medicineNumber);
  Serial.print(" status -> ");
  Serial.println(status);
}


// ======================================================
// SERVO 1 OPEN
// ======================================================

void openMedicine1()
{
  servo1.write(
    SERVO_OPEN_ANGLE
  );

  Serial.println(
    "Medicine 1 compartment OPEN."
  );
}


// ======================================================
// SERVO 1 CLOSE
// ======================================================

void closeMedicine1()
{
  servo1.write(
    SERVO_CLOSED_ANGLE
  );

  Serial.println(
    "Medicine 1 compartment CLOSED."
  );
}


// ======================================================
// SERVO 2 OPEN
// ======================================================

void openMedicine2()
{
  servo2.write(
    SERVO_OPEN_ANGLE
  );

  Serial.println(
    "Medicine 2 compartment OPEN."
  );
}


// ======================================================
// SERVO 2 CLOSE
// ======================================================

void closeMedicine2()
{
  servo2.write(
    SERVO_CLOSED_ANGLE
  );

  Serial.println(
    "Medicine 2 compartment CLOSED."
  );
}


// ======================================================
// START MEDICINE 1
// ======================================================

void startMedicine1()
{
  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "MEDICINE 1 TIME!"
  );

  Serial.println(
    "================================"
  );


  // -----------------------------------------------
  // AWAY MODE
  // -----------------------------------------------

  if (awayMode)
  {
    Serial.println(
      "AWAY MODE ACTIVE."
    );

    Serial.println(
      "Medicine 1 reminder skipped."
    );


    medicine1State =
      MED_AWAY;


    medicine1StateStart =
      millis();


    updateMedicineStatus(1, "SKIPPED - AWAY");


    showAwayScreen();


    return;
  }


  // -----------------------------------------------
  // NORMAL MEDICINE REMINDER
  // -----------------------------------------------

  openMedicine1();


  digitalWrite(
    RED_LED_PIN,
    HIGH
  );


  digitalWrite(
    BUZZER_PIN,
    HIGH
  );


  showPillTimeScreen();


  medicine1State =
    MED_FIRST_ALERT;


  medicine1StateStart =
    millis();
}


// ======================================================
// START MEDICINE 2
// ======================================================

void startMedicine2()
{
  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "MEDICINE 2 TIME!"
  );

  Serial.println(
    "================================"
  );


  // -----------------------------------------------
  // AWAY MODE
  // -----------------------------------------------

  if (awayMode)
  {
    Serial.println(
      "AWAY MODE ACTIVE."
    );

    Serial.println(
      "Medicine 2 reminder skipped."
    );


    medicine2State =
      MED_AWAY;


    medicine2StateStart =
      millis();


    updateMedicineStatus(2, "SKIPPED - AWAY");


    showAwayScreen();


    return;
  }


  // -----------------------------------------------
  // NORMAL MEDICINE REMINDER
  // -----------------------------------------------

  openMedicine2();


  digitalWrite(
    RED_LED_PIN,
    HIGH
  );


  digitalWrite(
    BUZZER_PIN,
    HIGH
  );


  showPillTimeScreen();


  medicine2State =
    MED_FIRST_ALERT;


  medicine2StateStart =
    millis();
}


// ======================================================
// MEDICINE 1 TAKEN
// ======================================================

void medicine1Taken()
{
  Serial.println(
    "MEDICINE 1 DETECTED AS TAKEN!"
  );


  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  digitalWrite(
    RED_LED_PIN,
    LOW
  );


  showMedicineTakenScreen();


  updateMedicineStatus(1, "TAKEN");


  sendMedicineTakenNotification(1);


  medicine1State =
    MED_TAKEN;


  medicine1StateStart =
    millis();
}


// ======================================================
// MEDICINE 2 TAKEN
// ======================================================

void medicine2Taken()
{
  Serial.println(
    "MEDICINE 2 DETECTED AS TAKEN!"
  );


  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  digitalWrite(
    RED_LED_PIN,
    LOW
  );


  showMedicineTakenScreen();


  updateMedicineStatus(2, "TAKEN");


  sendMedicineTakenNotification(2);


  medicine2State =
    MED_TAKEN;


  medicine2StateStart =
    millis();
}


// ======================================================
// MEDICINE 1 MISSED
// ======================================================

void medicine1Missed()
{
  Serial.println(
    "MEDICINE 1 NOT TAKEN!"
  );


  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  digitalWrite(
    RED_LED_PIN,
    LOW
  );


  showMedicineMissedScreen();


  updateMedicineStatus(1, "NOT TAKEN");


  sendMedicineMissedNotification(1);


  closeMedicine1();


  medicine1State =
    MED_MISSED;


  medicine1StateStart =
    millis();
}


// ======================================================
// MEDICINE 2 MISSED
// ======================================================

void medicine2Missed()
{
  Serial.println(
    "MEDICINE 2 NOT TAKEN!"
  );


  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  digitalWrite(
    RED_LED_PIN,
    LOW
  );


  showMedicineMissedScreen();


  updateMedicineStatus(2, "NOT TAKEN");


  sendMedicineMissedNotification(2);


  closeMedicine2();


  medicine2State =
    MED_MISSED;


  medicine2StateStart =
    millis();
}


// ======================================================
// CHECK MEDICINE 1 TIME
// ======================================================

void checkMedicine1Time(DateTime now)
{
  if (
    now.hour() == MED1_HOUR &&
    now.minute() == MED1_MINUTE
  )
  {
    if (
      now.day() != med1LastDay ||
      now.month() != med1LastMonth ||
      now.year() != med1LastYear
    )
    {
      med1LastDay =
        now.day();

      med1LastMonth =
        now.month();

      med1LastYear =
        now.year();


      startMedicine1();
    }
  }
}


// ======================================================
// CHECK MEDICINE 2 TIME
// ======================================================

void checkMedicine2Time(DateTime now)
{
  if (
    now.hour() == MED2_HOUR &&
    now.minute() == MED2_MINUTE
  )
  {
    if (
      now.day() != med2LastDay ||
      now.month() != med2LastMonth ||
      now.year() != med2LastYear
    )
    {
      med2LastDay =
        now.day();

      med2LastMonth =
        now.month();

      med2LastYear =
        now.year();


      startMedicine2();
    }
  }
}


// ======================================================
// HANDLE MEDICINE 1
// ======================================================

void handleMedicine1()
{
  unsigned long elapsed =
    millis() - medicine1StateStart;


  // ====================================================
  // FIRST 10 SECONDS
  // ====================================================

  if (
    medicine1State == MED_FIRST_ALERT
  )
  {
    // Ignore the IR sensor for 2 seconds while the lid finishes opening.
    // This prevents the moving lid/flap from being detected as a hand.
    if (
      elapsed >= IR_START_DELAY &&
      digitalRead(IR1_PIN) == IR_DETECTED
    )
    {
      Serial.println(
        "HAND DETECTED - MEDICINE 1"
      );

      medicine1Taken();

      return;
    }


    if (
      elapsed >= FIRST_ALERT_TIME
    )
    {
      Serial.println(
        "Medicine 1 first alert finished."
      );


      digitalWrite(
        BUZZER_PIN,
        LOW
      );


      showPillTimeScreen();


      medicine1State =
        MED_WAITING;


      medicine1StateStart =
        millis();
    }
  }


  // ====================================================
  // WAITING 10 SECONDS
  // ====================================================

  else if (
    medicine1State == MED_WAITING
  )
  {
    if (
      digitalRead(IR1_PIN) == IR_DETECTED
    )
    {
      Serial.println(
        "HAND DETECTED - MEDICINE 1"
      );

      medicine1Taken();

      return;
    }


    if (
      elapsed >= WAIT_TIME
    )
    {
      Serial.println(
        "Medicine 1 entering second alert."
      );


      digitalWrite(
        BUZZER_PIN,
        HIGH
      );


      showPillTimeScreen();


      medicine1State =
        MED_SECOND_ALERT;


      medicine1StateStart =
        millis();
    }
  }


  // ====================================================
  // SECOND 5 SECOND ALERT
  // ====================================================

  else if (
    medicine1State == MED_SECOND_ALERT
  )
  {
    if (
      digitalRead(IR1_PIN) == IR_DETECTED
    )
    {
      Serial.println(
        "HAND DETECTED - MEDICINE 1"
      );

      medicine1Taken();

      return;
    }


    if (
      elapsed >= SECOND_ALERT_TIME
    )
    {
      medicine1Missed();
    }
  }


  // ====================================================
  // MEDICINE 1 TAKEN
  // ====================================================

  else if (
    medicine1State == MED_TAKEN
  )
  {
    if (
      elapsed >= CLOSE_DELAY
    )
    {
      closeMedicine1();


      medicine1State =
        MED_IDLE;


      Serial.println(
        "Medicine 1 returned to IDLE."
      );
    }
  }


  // ====================================================
  // MEDICINE 1 MISSED
  // ====================================================

  else if (
    medicine1State == MED_MISSED
  )
  {
    if (
      elapsed >= CLOSE_DELAY
    )
    {
      medicine1State =
        MED_IDLE;


      Serial.println(
        "Medicine 1 returned to IDLE."
      );
    }
  }


  // ====================================================
  // MEDICINE 1 AWAY
  // ====================================================

  else if (
    medicine1State == MED_AWAY
  )
  {
    // Nothing physical happens.

    // The medicine was skipped because
    // Away Mode was active.

    // Return to IDLE after a short time.

    if (
      elapsed >= 1000
    )
    {
      medicine1State =
        MED_IDLE;
    }
  }
}


// ======================================================
// HANDLE MEDICINE 2
// ======================================================

void handleMedicine2()
{
  unsigned long elapsed =
    millis() - medicine2StateStart;


  // ====================================================
  // FIRST 10 SECONDS
  // ====================================================

  if (
    medicine2State == MED_FIRST_ALERT
  )
  {
    // Ignore the IR sensor for 2 seconds while the lid finishes opening.
    // This prevents the moving lid/flap from being detected as a hand.
    if (
      elapsed >= IR_START_DELAY &&
      digitalRead(IR2_PIN) == IR_DETECTED
    )
    {
      Serial.println(
        "HAND DETECTED - MEDICINE 2"
      );

      medicine2Taken();

      return;
    }


    if (
      elapsed >= FIRST_ALERT_TIME
    )
    {
      Serial.println(
        "Medicine 2 first alert finished."
      );


      digitalWrite(
        BUZZER_PIN,
        LOW
      );


      showPillTimeScreen();


      medicine2State =
        MED_WAITING;


      medicine2StateStart =
        millis();
    }
  }


  // ====================================================
  // WAITING 10 SECONDS
  // ====================================================

  else if (
    medicine2State == MED_WAITING
  )
  {
    if (
      digitalRead(IR2_PIN) == IR_DETECTED
    )
    {
      Serial.println(
        "HAND DETECTED - MEDICINE 2"
      );

      medicine2Taken();

      return;
    }


    if (
      elapsed >= WAIT_TIME
    )
    {
      Serial.println(
        "Medicine 2 entering second alert."
      );


      digitalWrite(
        BUZZER_PIN,
        HIGH
      );


      showPillTimeScreen();


      medicine2State =
        MED_SECOND_ALERT;


      medicine2StateStart =
        millis();
    }
  }


  // ====================================================
  // SECOND 5 SECOND ALERT
  // ====================================================

  else if (
    medicine2State == MED_SECOND_ALERT
  )
  {
    if (
      digitalRead(IR2_PIN) == IR_DETECTED
    )
    {
      Serial.println(
        "HAND DETECTED - MEDICINE 2"
      );

      medicine2Taken();

      return;
    }


    if (
      elapsed >= SECOND_ALERT_TIME
    )
    {
      medicine2Missed();
    }
  }


  // ====================================================
  // MEDICINE 2 TAKEN
  // ====================================================

  else if (
    medicine2State == MED_TAKEN
  )
  {
    if (
      elapsed >= CLOSE_DELAY
    )
    {
      closeMedicine2();


      medicine2State =
        MED_IDLE;


      Serial.println(
        "Medicine 2 returned to IDLE."
      );
    }
  }


  // ====================================================
  // MEDICINE 2 MISSED
  // ====================================================

  else if (
    medicine2State == MED_MISSED
  )
  {
    if (
      elapsed >= CLOSE_DELAY
    )
    {
      medicine2State =
        MED_IDLE;


      Serial.println(
        "Medicine 2 returned to IDLE."
      );
    }
  }


  // ====================================================
  // MEDICINE 2 AWAY
  // ====================================================

  else if (
    medicine2State == MED_AWAY
  )
  {
    if (
      elapsed >= 1000
    )
    {
      medicine2State =
        MED_IDLE;
    }
  }
}


// ======================================================
// HANDLE EMERGENCY BUTTON
// ======================================================

void handleHelpButton()
{
  bool currentButtonState =
    digitalRead(HELP_BUTTON_PIN);


  // Detect new press

  if (
    currentButtonState == LOW &&
    lastHelpButtonState == HIGH &&
    millis() - lastHelpButtonPress >= BUTTON_DEBOUNCE_TIME
  )
  {
    lastHelpButtonPress =
      millis();


    // ==================================================
    // FIRST PRESS
    // ==================================================

    if (!emergencyActive)
    {
      emergencyActive =
        true;


      emergencyStartTime =
        millis();


      Serial.println();
      Serial.println(
        "################################"
      );

      Serial.println(
        "EMERGENCY ALERT ACTIVATED"
      );

      Serial.println(
        "################################"
      );


      // Blue LED ON

      digitalWrite(
        BLUE_LED_PIN,
        HIGH
      );


      // Buzzer ON

      digitalWrite(
        BUZZER_PIN,
        HIGH
      );


      // Red LED OFF

      digitalWrite(
        RED_LED_PIN,
        LOW
      );


      // OLED

      showEmergencyScreen();


      // Blynk

      sendEmergencyNotification();
    }


    // ==================================================
    // SECOND PRESS
    // ==================================================

    else
    {
      unsigned long emergencyDuration =
        millis() - emergencyStartTime;


      if (
        emergencyDuration >=
        MIN_EMERGENCY_TIME
      )
      {
        emergencyActive =
          false;


        Serial.println();
        Serial.println(
          "################################"
        );

        Serial.println(
          "EMERGENCY ALERT CLEARED"
        );

        Serial.println(
          "################################"
        );


        // Blue LED OFF

        digitalWrite(
          BLUE_LED_PIN,
          LOW
        );


        // Buzzer OFF

        digitalWrite(
          BUZZER_PIN,
          LOW
        );


        // Restore appropriate screen

        if (awayMode)
        {
          showAwayScreen();
        }
        else if (
          medicine1State != MED_IDLE ||
          medicine2State != MED_IDLE
        )
        {
          showPillTimeScreen();
        }
        else
        {
          DateTime now =
            rtc.now();

          digitalWrite(
            RED_LED_PIN,
            LOW
          );

          showNormalScreen(now);
        }
      }
      else
      {
        Serial.println(
          "Emergency must remain active "
          "for at least 10 seconds."
        );
      }
    }
  }


  lastHelpButtonState =
    currentButtonState;
}


// ======================================================
// HANDLE AWAY BUTTON
// ======================================================

void handleAwayButton()
{
  bool currentButtonState =
    digitalRead(AWAY_BUTTON_PIN);


  // Detect a new button press

  if (
    currentButtonState == LOW &&
    lastAwayButtonState == HIGH &&
    millis() - lastAwayButtonPress >= BUTTON_DEBOUNCE_TIME
  )
  {
    lastAwayButtonPress =
      millis();


    // ================================================
    // TURN AWAY MODE ON
    // ================================================

    if (!awayMode)
    {
      awayMode =
        true;


      Serial.println();
      Serial.println(
        "================================"
      );

      Serial.println(
        "AWAY MODE ACTIVATED"
      );

      Serial.println(
        "================================"
      );


      // ----------------------------------------------
      // If a medicine reminder is currently active,
      // stop it and close the compartment.
      // ----------------------------------------------

      if (
        medicine1State == MED_FIRST_ALERT ||
        medicine1State == MED_WAITING ||
        medicine1State == MED_SECOND_ALERT
      )
      {
        digitalWrite(
          BUZZER_PIN,
          LOW
        );

        digitalWrite(
          RED_LED_PIN,
          LOW
        );

        closeMedicine1();

        medicine1State =
          MED_AWAY;

        medicine1StateStart =
          millis();
      }


      if (
        medicine2State == MED_FIRST_ALERT ||
        medicine2State == MED_WAITING ||
        medicine2State == MED_SECOND_ALERT
      )
      {
        digitalWrite(
          BUZZER_PIN,
          LOW
        );

        digitalWrite(
          RED_LED_PIN,
          LOW
        );

        closeMedicine2();

        medicine2State =
          MED_AWAY;

        medicine2StateStart =
          millis();
      }


      // Show Away Mode

      showAwayScreen();


      // Send Blynk notification

      sendAwayModeOnNotification();
    }


    // ================================================
    // TURN AWAY MODE OFF
    // ================================================

    else
    {
      awayMode =
        false;


      Serial.println();
      Serial.println(
        "================================"
      );

      Serial.println(
        "AWAY MODE DEACTIVATED"
      );

      Serial.println(
        "================================"
      );


      // Send Blynk notification

      sendAwayModeOffNotification();


      // Return to normal screen

      DateTime now =
        rtc.now();


      if (
        medicine1State == MED_IDLE &&
        medicine2State == MED_IDLE
      )
      {
        showNormalScreen(now);
      }
      else
      {
        showPillTimeScreen();
      }
    }
  }


  lastAwayButtonState =
    currentButtonState;
}


// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);


  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "       MEDIGUARD STARTING"
  );

  Serial.println(
    "================================"
  );


  // ====================================================
  // PIN MODES
  // ====================================================

  pinMode(
    IR1_PIN,
    INPUT
  );

  pinMode(
    IR2_PIN,
    INPUT
  );


  pinMode(
    BUZZER_PIN,
    OUTPUT
  );


  pinMode(
    RED_LED_PIN,
    OUTPUT
  );


  pinMode(
    BLUE_LED_PIN,
    OUTPUT
  );


  // Emergency button

  pinMode(
    HELP_BUTTON_PIN,
    INPUT_PULLUP
  );


  // Away button

  pinMode(
    AWAY_BUTTON_PIN,
    INPUT_PULLUP
  );


  // ====================================================
  // INITIAL OUTPUT STATES
  // ====================================================

  digitalWrite(
    BUZZER_PIN,
    LOW
  );


  digitalWrite(
    RED_LED_PIN,
    LOW
  );


  digitalWrite(
    BLUE_LED_PIN,
    LOW
  );


  // ====================================================
  // I2C
  // ====================================================

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );


  // ====================================================
  // OLED
  // ====================================================

  oled.begin();


  oled.clearBuffer();


  oled.setFont(
    u8g2_font_ncenB08_tr
  );


  oled.drawStr(
    25,
    30,
    "MEDIGUARD"
  );


  oled.drawStr(
    32,
    48,
    "STARTING"
  );


  oled.sendBuffer();


  delay(1500);


  // ====================================================
  // RTC
  // ====================================================

  if (!rtc.begin())
  {
    Serial.println(
      "RTC NOT FOUND!"
    );


    oled.clearBuffer();


    oled.setFont(
      u8g2_font_ncenB08_tr
    );


    oled.drawStr(
      20,
      30,
      "RTC ERROR"
    );


    oled.sendBuffer();


    while (1)
    {
      delay(100);
    }
  }


  Serial.println(
    "RTC FOUND."
  );


  // ====================================================
  // RTC TIME
  // ====================================================

  // IMPORTANT:
  //
  // Do NOT automatically reset the RTC every boot.
  //
  // If the RTC time is wrong:
  //
  // 1. Uncomment the line below.
  // 2. Upload once.
  // 3. Comment the line again.
  // 4. Upload again.
  //
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));


  // ====================================================
  // SERVO 1
  // ====================================================

  servo1.setPeriodHertz(50);


  servo1.attach(
    SERVO1_PIN,
    500,
    2400
  );


  servo1.write(
    SERVO_CLOSED_ANGLE
  );


  // ====================================================
  // SERVO 2
  // ====================================================

  servo2.setPeriodHertz(50);


  servo2.attach(
    SERVO2_PIN,
    500,
    2400
  );


  servo2.write(
    SERVO_CLOSED_ANGLE
  );


  // ====================================================
  // WIFI
  // ====================================================

  Serial.println(
    "Connecting to WiFi..."
  );


  WiFi.begin(
    ssid,
    pass
  );


  int wifiAttempts = 0;


  while (
    WiFi.status() != WL_CONNECTED &&
    wifiAttempts < 30
  )
  {
    delay(500);

    Serial.print(".");

    wifiAttempts++;
  }


  Serial.println();


  if (
    WiFi.status() == WL_CONNECTED
  )
  {
    Serial.println(
      "WiFi connected!"
    );


    Serial.print(
      "IP Address: "
    );


    Serial.println(
      WiFi.localIP()
    );
  }
  else
  {
    Serial.println(
      "WiFi connection failed!"
    );
  }


  // ====================================================
  // BLYNK
  // ====================================================

  Blynk.config(
    BLYNK_AUTH_TOKEN
  );


  if (
    Blynk.connect(10000)
  )
  {
    Serial.println(
      "BLYNK CONNECTED!"
    );
  }
  else
  {
    Serial.println(
      "BLYNK CONNECTION FAILED!"
    );
  }


  // ====================================================
  // INITIAL MEDICINE STATUS
  // ====================================================

  updateMedicineStatus(1, "READY");
  updateMedicineStatus(2, "READY");


  // ====================================================
  // INITIAL OLED
  // ====================================================

  DateTime now =
    rtc.now();


  showNormalScreen(now);


  // ====================================================
  // READY
  // ====================================================

  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "       MEDIGUARD READY"
  );

  Serial.println(
    "================================"
  );


  Serial.println(
    "Medicine 1 : 22:53"
  );


  Serial.println(
    "Medicine 2 : 22:54"
  );


  Serial.println(
    "Emergency Button : GPIO13"
  );


  Serial.println(
    "Blue LED         : GPIO4"
  );


  Serial.println(
    "Away Button      : GPIO14"
  );


  Serial.println(
    "Away Mode        : OFF"
  );
}


// ======================================================
// LOOP
// ======================================================

void loop()
{
  // ====================================================
  // BLYNK
  // ====================================================

  if (
    Blynk.connected()
  )
  {
    Blynk.run();
  }


  // ====================================================
  // BLYNK RECONNECT
  // ====================================================

  if (
    millis() - lastBlynkCheck >= 10000
  )
  {
    lastBlynkCheck =
      millis();


    if (
      !Blynk.connected()
    )
    {
      Serial.println(
        "Trying to reconnect Blynk..."
      );


      Blynk.connect(3000);
    }
  }


  // ====================================================
  // EMERGENCY BUTTON
  // ====================================================

  handleHelpButton();


  // ====================================================
  // AWAY BUTTON
  // ====================================================

  handleAwayButton();


  // ====================================================
  // EMERGENCY ACTIVE
  // ====================================================

  if (emergencyActive)
  {
    // Keep emergency indication ON.

    digitalWrite(
      BLUE_LED_PIN,
      HIGH
    );


    digitalWrite(
      BUZZER_PIN,
      HIGH
    );


    delay(10);


    return;
  }


  // ====================================================
  // RTC
  // ====================================================

  DateTime now =
    rtc.now();


  // ====================================================
  // CHECK MEDICINE TIMES
  // ====================================================

  checkMedicine1Time(now);

  checkMedicine2Time(now);


  // ====================================================
  // HANDLE MEDICINE 1
  // ====================================================

  if (
    medicine1State != MED_IDLE
  )
  {
    handleMedicine1();
  }


  // ====================================================
  // HANDLE MEDICINE 2
  // ====================================================

  if (
    medicine2State != MED_IDLE
  )
  {
    handleMedicine2();
  }


  // ====================================================
  // NORMAL OLED
  // ====================================================

  if (
    awayMode
  )
  {
    // Keep showing Away Mode

    if (!emergencyActive)
    {
      showAwayScreen();
    }
  }
  else if (
    medicine1State == MED_IDLE &&
    medicine2State == MED_IDLE
  )
  {
    showNormalScreen(now);
  }


  // ====================================================
  // SMALL LOOP DELAY
  // ====================================================

  delay(20);
}
