#include <Ardulogger.h>

// Initialize logger with SD chip select pin
Ardulogger logger(10);

// Sensor variables to track
float vwc1 = 0.0;
float vwc2 = 0.0;
float airTemp = 0.0;

void setup() {
  Serial.begin(9600);

  // Initialize SD and RTC modules
  if (!logger.begin()) {
    Serial.println("Logger failed to initialize.");
    while (1);
  }

  // Set log file name
  logger.datafile("datalog.csv");

  // Bind variables to labels — only done once
  logger.data("RootZone1", vwc1);
  logger.data("RootZone2", vwc2);
  logger.data("AirTemp", airTemp);

  // Optional: set decimal precision (default is 2)
  logger.setPrecision(3);

  // Optional: add a comment with a timestamp
  logger.comment("Greenhouse logger started");

  // Optional: check if log file already exists
  if (logger.fileExists()) {
    Serial.println("Log file found.");
  } else {
    Serial.println("Log file will be created with header.");
  }

  // Optional: verify the last entry logged
  String last = logger.readLastLine();
  Serial.println("Last entry:");
  Serial.println(last);
}

void loop() {
  // Simulate reading sensors
  vwc1 = analogRead(A0) * 0.1;
  vwc2 = analogRead(A1) * 0.1;
  airTemp = analogRead(A2) * 0.01;

  // Log data: Timestamp, RootZone1, RootZone2, AirTemp
  logger.datalog();

  delay(60000);  // Wait one minute
}
