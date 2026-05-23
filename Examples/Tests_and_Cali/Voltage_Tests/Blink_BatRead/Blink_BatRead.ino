#include <Wire.h>
#include <Ardulogger.h>

#define VBATPIN A13
#define VREGPIN A4

Ardulogger logger(33); // SD card CS pin

int led = LED_BUILTIN;
float vbat = 0.0;   // this variable is what Ardulogger will log
float vreg = 0.0;

unsigned long time_a = 0;
unsigned long time_b = 0;
unsigned long time_c = 0;

int toggle = 1;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(led, OUTPUT);

  // Set file name BEFORE logger.begin()
  logger.datafile("Regulator_test.CSV");

  if (!logger.begin()) {
    Serial.println("Logger failed to initialize.");
    while (1);
  }

  logger.data("Battery_Voltage", vbat);
  logger.data("Regulator_Voltage",vreg);

  logger.comment("Regulator voltage test started");
}

void loop() {
  time_a = millis();

  // Log battery voltage every 60 seconds
  if ((time_a - time_b) > 60000) {
    vbat = analogReadMilliVolts(VBATPIN);
    vbat *= 2;    // battery divider divides by 2, so multiply back
    vbat /= 1000; // convert mV to V

    vreg = analogRead(VREGPIN);

    logger.datalog();

    Serial.print("Battery voltage: ");
    Serial.print(vbat);
    Serial.print("  Regulator voltage: ");
    Serial.print(vreg);
    Serial.println(" V");

    time_b = millis();
  }

  // Blink LED every 500 ms
  if ((time_a - time_c) > 1000) {
    if(vbat < 4){
      if (toggle) {
        digitalWrite(led, HIGH);
        toggle = 0;
      } else {
        digitalWrite(led, LOW);
        toggle = 1;
      }
    } else {
      digitalWrite(led, HIGH);
    }
    time_c = millis();
    // Serial.println("LED Toggled");
  }
}