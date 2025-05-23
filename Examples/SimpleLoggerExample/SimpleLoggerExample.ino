#include <Ardulogger.h>

Ardulogger logger(10); // CS pin

float value = 3.14;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!logger.begin()) {
    Serial.println("Logger failed to initialize.");
    while (1);
  }

  logger.datafile("SIMPLE.CSV");
  logger.data("Value", value);
}

void loop() {
  logger.datalog();
  Serial.println("Data logged.");
  delay(5000);
}
