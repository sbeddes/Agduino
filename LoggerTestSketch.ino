#include <Logger.h>

// Set up the Logger using SD chip select pin 10
Logger logger(10);

void setup() {
  Serial.begin(9600);

  // Initialize the RTC and SD card
  if (!logger.begin()) {
    Serial.println("Logger failed to initialize.");
    while (1);
  }

  // Set the log filename
  logger.datafile("greenhouse.csv");

  // Check if file exists (to conditionally write a header)
  if (!logger.fileExists()) {
    // Write column headers (only if file doesn't already exist)
    logger.header({ "Moisture1", "Moisture2", "Temperature" });
  }

  // Set desired precision for float values (e.g., 3 decimals)
  logger.setPrecision(3);

  // Log a comment (prefixed with #) for debugging or notes
  logger.comment("System startup");

  // Simulate collecting data from sensors
  float moisture1 = 35.4567;
  float moisture2 = 38.8912;
  float temperature = 22.351;

  // Queue the data values for logging
  logger.data(moisture1);      // Add first moisture reading
  logger.data(moisture2);      // Add second moisture reading
  logger.data(temperature);    // Add temperature reading

  // Write timestamped row: Timestamp, Moisture1, Moisture2, Temperature
  logger.datalog();

  // Example: adding a string data value
  logger.data("OK");
  logger.data("N/A");
  logger.datalog();

  // Flush isn't required here (datalog() closes file), but could be added for safety
  // logger.flush(); // <-- optional in future implementation

  // Print last line written to verify
  String lastLine = logger.readLastLine();
  Serial.println("Last log entry:");
  Serial.println(lastLine);
}

void loop() {
  // Nothing in loop for this test
}
