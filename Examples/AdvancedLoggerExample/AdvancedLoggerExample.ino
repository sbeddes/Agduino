#include <Ardulogger.h>

Ardulogger logger(10); // Chip Select pin

float temperature;
float humidity;
float lightLevel;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!logger.begin()) {
    Serial.println("Logger failed to initialize.");
    while (1);
  }

  logger.datafile("ADVANCED.CSV");
  logger.setPrecision(3);

  logger.data("Temp_C", temperature);
  logger.data("Humidity_%", humidity);
  logger.data("Light_Lux", lightLevel);

  logger.comment("Logging session started.");
}

void loop() {
  // Simulate sensor values
  temperature = 20.0 + random(-500, 500) / 100.0;
  humidity = 50.0 + random(-1000, 1000) / 100.0;
  lightLevel = 100.0 + random(-1000, 1000) / 10.0;

  logger.datalog();
  Serial.println("Data logged.");

  delay(10000);
}
