/*
  Basic GPS Example for Tenergy32Hub with GY-NEO-8M GPS Module
  
  This example demonstrates how to read GPS data from the GY-NEO-8M module
  using the Tenergy32Hub library.
  
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

// GPIO pins for GPS module (adjust as needed)
const int GPS_RX_PIN = 16;  // Connect to GPS TX
const int GPS_TX_PIN = 17;  // Connect to GPS RX

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(115200);
  Serial.println("Tenergy32Hub GY-NEO-8M GPS Example");
  Serial.println("===================================");
  
  // Initialize GPS module
  gps.begin(GPS_RX_PIN, GPS_TX_PIN, 9600);
  
  Serial.println("GPS module initialized");
  Serial.println("Waiting for GPS fix...");
  Serial.println();
}

void loop() {
  // Check if GPS data is available
  if (gps.available()) {
    // Read GPS data
    GPSData data = gps.readGPS();
    
    // Display GPS information if valid
    if (data.valid) {
      Serial.println("=== GPS DATA ===");
      Serial.print("Latitude: ");
      Serial.println(data.latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(data.longitude, 6);
      Serial.print("Altitude: ");
      Serial.print(data.altitude);
      Serial.println(" m");
      Serial.print("Speed: ");
      Serial.print(data.speed);
      Serial.println(" km/h");
      Serial.print("Satellites: ");
      Serial.println(data.satellites);
      Serial.print("Time: ");
      Serial.println(data.time);
      Serial.print("Date: ");
      Serial.println(data.date);
      Serial.println();
    }
  }
  
  // Small delay to prevent overwhelming the serial output
  delay(1000);
}