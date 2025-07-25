#include "DHT.h" // Include the DHT library

#define DHTPIN 27       // Define the pin where the DHT11 sensor is connected
#define DHTTYPE DHT11   // Specify the type of DHT sensor (DHT11 or DHT22)

// Pins for the MQ-7 sensor
#define MQ7_ANALOG_PIN 35 // ADC1_CH7 corresponds to Pin 35

// Initialize the DHT sensor.
// If using an ESP32, ensure the pin is compatible and has a pull-up resistor if necessary.
DHT dht(DHTPIN, DHTTYPE);

// Variable for the MQ-7 sensor reading
int mq7_rawValue = 0; // Declare globally for use in loop()

void setup() {
  Serial.begin(9600); // Start serial communication at 115200 baud (faster for ESP32)
  Serial.println(F("--- Sensor Readings Start ---")); // Print a start message

  // Initialize DHT sensor
  dht.begin();
  Serial.println(F("DHT11 sensor initialized."));

  Serial.println(F("MQ-7 sensor connected to Pin 35."));
  Serial.println(F("Please allow several minutes for the MQ-7 to warm up. Initial readings may not be accurate."));
  Serial.println(F("-----------------------------"));
}

void loop() {
  // Wait 1 second between readings (DHT11 shouldn't be read too quickly)
  delay(1000);

  // Read humidity
  float h = dht.readHumidity();
  // Read temperature in Celsius
  float t = dht.readTemperature();

  // Read raw value from MQ-7 sensor
  mq7_rawValue = analogRead(MQ7_ANALOG_PIN);

  // Check if DHT readings failed
  if (isnan(h) || isnan(t)) { // Corrected the extra parenthesis here
    Serial.println(F("Error reading from DHT sensor!"));
    // You might want to return here or just skip printing DHT values
    // and continue with MQ-7 if you prefer.
  } else {
    // Print DHT and MQ-7 values in a concise format
    Serial.print("|");
    Serial.print(h); // Humidity
    Serial.print("|");
    Serial.print(t); // Temperature
    Serial.print("|");
    Serial.print(mq7_rawValue); // MQ-7 raw value
    Serial.println("|");
  }
}