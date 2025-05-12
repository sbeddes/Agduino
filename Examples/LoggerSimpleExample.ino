#include <Ardulogger.h>

Logger logger(10);  // SD card CS pin

float moisture = 0.0;
float temperature = 0.0;

void setup() {
  Serial.begin(9600);
  logger.begin();
  logger.datafile("test.csv"); // Could use any type of text file 

  // Auto-bind variables (no ampersands)
  logger.data("Moisture", moisture);
  logger.data("Temperature", temperature);
}

void loop() {
  // Simulated sensor readings
  moisture = analogRead(A0) * 0.1;
  temperature = analogRead(A1) * 0.01;

  logger.datalog();  // Logs timestamp + moisture + temperature
  delay(60000);      // Log every minute
}
