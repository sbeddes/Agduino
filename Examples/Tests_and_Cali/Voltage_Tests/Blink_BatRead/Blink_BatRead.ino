
#include <Wire.h>
#include <Ardulogger.h>

#define VBATPIN A13
#define VREGPIN A4

Ardulogger logger(33); // SD card CS pin

int led = LED_BUILTIN;

float vbat = 0.0;   // Battery voltage in volts
float vreg = 0.0;   // Regulator voltage in volts

unsigned long time_a = 0;
unsigned long time_b = 0;
unsigned long time_c = 0;

int toggle = 1;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(led, OUTPUT);

  // ADC setup
  analogReadResolution(12);
  analogSetPinAttenuation(VBATPIN, ADC_11db);
  analogSetPinAttenuation(VREGPIN, ADC_11db);

  // Set file name BEFORE logger.begin()
  logger.datafile("Regulator_test.CSV");

  if (!logger.begin()) {
    Serial.println("Logger failed to initialize.");
    while (1);
  }

  logger.data("Battery_Voltage", vbat);
  logger.data("Regulator_Voltage", vreg);

  logger.comment("Regulator voltage test started");
}

void loop() {
  time_a = millis();

  // Log battery and regulator voltage every 60 seconds
  if ((time_a - time_b) > 60000) {

    // VBAT has a built-in divider, so multiply by 2
    vbat = analogReadMilliVolts(VBATPIN);
    vbat *= 2.0;
    vbat /= 1000.0;

    // Regulator output connected directly to A4
    vreg = analogReadMilliVolts(VREGPIN);
    vreg /= 1000.0;

    logger.datalog();

    Serial.print("Battery voltage: ");
    Serial.print(vbat, 3);
    Serial.print(" V  Regulator voltage: ");
    Serial.print(vreg, 3);
    Serial.println(" V");

    time_b = millis();
  }

  // Blink LED every 1 second if battery is below 4 V
  if ((time_a - time_c) > 1000) {
    if (vbat < 4.0) {
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
  }
}