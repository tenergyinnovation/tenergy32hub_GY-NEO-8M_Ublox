/*
  Advanced GPS Example for Tenergy32Hub with GY-NEO-8M GPS Module
  
  This example demonstrates advanced features including:
  - Raw NMEA sentence reading
  - GPS configuration
  - Multiple GPS data processing
  
  Hardware Connections:
  - GY-NEO-8M VCC -> 3.3V or 5V
  - GY-NEO-8M GND -> GND
  - GY-NEO-8M RX  -> GPIO 17 (or any available pin)
  - GY-NEO-8M TX  -> GPIO 16 (or any available pin)
  
  Created by Tenergy Innovation
  https://github.com/tenergyinnovation/tenergy32hub_GY-NEO-8M_Ublox
*/

#include <Tenergy32Hub_GY_NEO_8M.h>

// Create GPS object
Tenergy32Hub_GY_NEO_8M gps;

// GPIO pins for GPS module
const int GPS_RX_PIN = 16;  // Connect to GPS TX
const int GPS_TX_PIN = 17;  // Connect to GPS RX

unsigned long lastDisplayTime = 0;
const unsigned long DISPLAY_INTERVAL = 2000; // Display every 2 seconds

void setup() {
  Serial.begin(115200);
  Serial.println("Tenergy32Hub GY-NEO-8M Advanced GPS Example");
  Serial.println("============================================");
  
  // Initialize GPS module
  gps.begin(GPS_RX_PIN, GPS_TX_PIN, 9600);
  
  // Configure GPS module
  Serial.println("Configuring GPS module...");
  delay(1000);
  
  // Enable specific NMEA sentences
  gps.enableGGA(true);  // Enable GGA (position data)
  gps.enableRMC(true);  // Enable RMC (recommended minimum data)
  
  Serial.println("GPS module configured");
  Serial.println("Waiting for GPS data...");
  Serial.println();
  
  // Print headers
  Serial.println("Option 1: Raw NMEA sentences");
  Serial.println("Option 2: Parsed GPS data");
  Serial.println("Send '1' for raw NMEA or '2' for parsed data");
  Serial.println();
}

void loop() {
  // Check for user input to switch display mode
  if (Serial.available()) {
    char input = Serial.read();
    if (input == '1') {
      displayRawNMEA();
    } else if (input == '2') {
      displayParsedData();
    }
  }
  
  // Default: display parsed data every 2 seconds
  if (millis() - lastDisplayTime >= DISPLAY_INTERVAL) {
    displayParsedData();
    lastDisplayTime = millis();
  }
  
  delay(100);
}

void displayRawNMEA() {
  Serial.println("=== RAW NMEA MODE ===");
  Serial.println("Press '2' to switch to parsed data mode");
  Serial.println();
  
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) { // Show for 10 seconds
    if (gps.available()) {
      String nmea = gps.readNMEA();
      if (nmea.length() > 0) {
        Serial.println(nmea);
      }
    }
    
    // Check for mode switch
    if (Serial.available()) {
      char input = Serial.read();
      if (input == '2') break;
    }
    
    delay(10);
  }
}

void displayParsedData() {
  static int validReadings = 0;
  static int totalReadings = 0;
  
  if (gps.available()) {
    GPSData data = gps.readGPS();
    totalReadings++;
    
    if (data.valid) {
      validReadings++;
      
      Serial.println("=== PARSED GPS DATA ===");
      Serial.printf("Reading: %d/%d valid\n", validReadings, totalReadings);
      Serial.printf("Coordinates: %.6f, %.6f\n", data.latitude, data.longitude);
      Serial.printf("Altitude: %.2f m\n", data.altitude);
      Serial.printf("Speed: %.2f km/h\n", data.speed);
      Serial.printf("Satellites: %d\n", data.satellites);
      
      if (data.time.length() > 0) {
        Serial.printf("Time: %s\n", data.time.c_str());
      }
      if (data.date.length() > 0) {
        Serial.printf("Date: %s\n", data.date.c_str());
      }
      
      // Calculate GPS quality indicator
      String quality = "Unknown";
      if (data.satellites >= 4) {
        if (data.satellites >= 8) {
          quality = "Excellent";
        } else if (data.satellites >= 6) {
          quality = "Good";
        } else {
          quality = "Fair";
        }
      } else {
        quality = "Poor";
      }
      Serial.printf("GPS Quality: %s\n", quality.c_str());
      
      // Show Google Maps link
      Serial.printf("Google Maps: https://maps.google.com/?q=%.6f,%.6f\n", 
                   data.latitude, data.longitude);
      
      Serial.println();
    } else {
      Serial.printf("Waiting for GPS fix... (%d/%d readings processed)\n", 
                   validReadings, totalReadings);
    }
  }
}