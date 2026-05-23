#define VBATPIN A13

int led = LED_BUILTIN;

void setup() {
  // Some boards work best if we also make a serial connection
  Serial.begin(115200);

  // set LED to be an output pin
  pinMode(led, OUTPUT);
}

void loop() {
  // Say hi!
  Serial.println("Hello!");
  
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                // wait for a half second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(500);                // wait for a half second
  float measuredvbat = analogReadMilliVolts(VBATPIN);
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat /= 1000; // convert to volts!
  Serial.print("VBat: " ); Serial.println(measuredvbat);
}