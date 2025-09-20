#include "Tenergy32Hub_GY_NEO_8M.h"

Tenergy32Hub_GY_NEO_8M::Tenergy32Hub_GY_NEO_8M() {
  gpsSerial = nullptr;
}

void Tenergy32Hub_GY_NEO_8M::begin(HardwareSerial& serial, unsigned long baudRate) {
  gpsSerial = &serial;
  gpsSerial->begin(baudRate);
  nmeaBuffer = "";
}

void Tenergy32Hub_GY_NEO_8M::begin(int rxPin, int txPin, unsigned long baudRate) {
  gpsSerial = new HardwareSerial(2);
  gpsSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
  nmeaBuffer = "";
}

bool Tenergy32Hub_GY_NEO_8M::available() {
  return gpsSerial && gpsSerial->available();
}

String Tenergy32Hub_GY_NEO_8M::readNMEA() {
  if (!gpsSerial) return "";
  
  while (gpsSerial->available()) {
    char c = gpsSerial->read();
    if (c == '\n') {
      String sentence = nmeaBuffer;
      nmeaBuffer = "";
      return sentence;
    } else if (c != '\r') {
      nmeaBuffer += c;
    }
  }
  return "";
}

GPSData Tenergy32Hub_GY_NEO_8M::readGPS() {
  GPSData data;
  data.valid = false;
  
  String sentence = readNMEA();
  if (sentence.length() > 0) {
    if (sentence.startsWith("$GPGGA")) {
      parseGPGGA(sentence, data);
    } else if (sentence.startsWith("$GPRMC")) {
      parseGPRMC(sentence, data);
    }
  }
  
  return data;
}

bool Tenergy32Hub_GY_NEO_8M::parseGPGGA(String sentence, GPSData& data) {
  // Parse GPGGA sentence: $GPGGA,time,lat,latDir,lon,lonDir,quality,numSat,hdop,alt,altUnit,geoid,geoidUnit,dgpsAge,dgpsId*checksum
  int commaIndex[14];
  int commaCount = 0;
  
  // Find all comma positions
  for (int i = 0; i < sentence.length() && commaCount < 14; i++) {
    if (sentence.charAt(i) == ',') {
      commaIndex[commaCount++] = i;
    }
  }
  
  if (commaCount < 14) return false;
  
  // Check fix quality
  String quality = sentence.substring(commaIndex[5] + 1, commaIndex[6]);
  if (quality == "0") return false; // No fix
  
  // Parse latitude
  String latStr = sentence.substring(commaIndex[1] + 1, commaIndex[2]);
  String latDir = sentence.substring(commaIndex[2] + 1, commaIndex[3]);
  if (latStr.length() > 0) {
    data.latitude = convertToDecimalDegrees(latStr, latDir);
  }
  
  // Parse longitude
  String lonStr = sentence.substring(commaIndex[3] + 1, commaIndex[4]);
  String lonDir = sentence.substring(commaIndex[4] + 1, commaIndex[5]);
  if (lonStr.length() > 0) {
    data.longitude = convertToDecimalDegrees(lonStr, lonDir);
  }
  
  // Parse satellites
  String satStr = sentence.substring(commaIndex[6] + 1, commaIndex[7]);
  if (satStr.length() > 0) {
    data.satellites = satStr.toInt();
  }
  
  // Parse altitude
  String altStr = sentence.substring(commaIndex[8] + 1, commaIndex[9]);
  if (altStr.length() > 0) {
    data.altitude = altStr.toFloat();
  }
  
  data.valid = true;
  return true;
}

bool Tenergy32Hub_GY_NEO_8M::parseGPRMC(String sentence, GPSData& data) {
  // Parse GPRMC sentence: $GPRMC,time,status,lat,latDir,lon,lonDir,speed,course,date,magVar,magVarDir*checksum
  int commaIndex[11];
  int commaCount = 0;
  
  // Find all comma positions
  for (int i = 0; i < sentence.length() && commaCount < 11; i++) {
    if (sentence.charAt(i) == ',') {
      commaIndex[commaCount++] = i;
    }
  }
  
  if (commaCount < 11) return false;
  
  // Check status
  String status = sentence.substring(commaIndex[1] + 1, commaIndex[2]);
  if (status != "A") return false; // Not active
  
  // Parse time
  String timeStr = sentence.substring(commaIndex[0] + 1, commaIndex[1]);
  if (timeStr.length() >= 6) {
    data.time = timeStr.substring(0, 2) + ":" + timeStr.substring(2, 4) + ":" + timeStr.substring(4, 6);
  }
  
  // Parse date
  String dateStr = sentence.substring(commaIndex[8] + 1, commaIndex[9]);
  if (dateStr.length() >= 6) {
    data.date = dateStr.substring(0, 2) + "/" + dateStr.substring(2, 4) + "/" + dateStr.substring(4, 6);
  }
  
  // Parse latitude
  String latStr = sentence.substring(commaIndex[2] + 1, commaIndex[3]);
  String latDir = sentence.substring(commaIndex[3] + 1, commaIndex[4]);
  if (latStr.length() > 0) {
    data.latitude = convertToDecimalDegrees(latStr, latDir);
  }
  
  // Parse longitude
  String lonStr = sentence.substring(commaIndex[4] + 1, commaIndex[5]);
  String lonDir = sentence.substring(commaIndex[5] + 1, commaIndex[6]);
  if (lonStr.length() > 0) {
    data.longitude = convertToDecimalDegrees(lonStr, lonDir);
  }
  
  // Parse speed (knots to km/h)
  String speedStr = sentence.substring(commaIndex[6] + 1, commaIndex[7]);
  if (speedStr.length() > 0) {
    data.speed = speedStr.toFloat() * 1.852; // Convert knots to km/h
  }
  
  data.valid = true;
  return true;
}

float Tenergy32Hub_GY_NEO_8M::convertToDecimalDegrees(String coordinate, String direction) {
  if (coordinate.length() < 4) return 0.0;
  
  float degrees = 0.0;
  
  // Find decimal point
  int dotIndex = coordinate.indexOf('.');
  if (dotIndex > 2) {
    // Format: DDMM.MMMM or DDDMM.MMMM
    int degreeDigits = dotIndex - 2;
    degrees = coordinate.substring(0, degreeDigits).toFloat();
    float minutes = coordinate.substring(degreeDigits).toFloat();
    degrees += minutes / 60.0;
  }
  
  // Apply direction
  if (direction == "S" || direction == "W") {
    degrees = -degrees;
  }
  
  return degrees;
}

void Tenergy32Hub_GY_NEO_8M::enableGGA(bool enable) {
  if (!gpsSerial) return;
  
  if (enable) {
    gpsSerial->println("$PUBX,40,GGA,0,1,0,0*5A"); // Enable GGA
  } else {
    gpsSerial->println("$PUBX,40,GGA,0,0,0,0*5B"); // Disable GGA
  }
}

void Tenergy32Hub_GY_NEO_8M::enableRMC(bool enable) {
  if (!gpsSerial) return;
  
  if (enable) {
    gpsSerial->println("$PUBX,40,RMC,0,1,0,0*47"); // Enable RMC
  } else {
    gpsSerial->println("$PUBX,40,RMC,0,0,0,0*46"); // Disable RMC
  }
}

void Tenergy32Hub_GY_NEO_8M::setBaudRate(unsigned long baudRate) {
  if (!gpsSerial) return;
  
  // Send UBX command to change baud rate
  // This is a simplified implementation - full UBX protocol would require checksum calculation
  String command = "$PUBX,41,1,0007,0003," + String(baudRate) + ",0*";
  gpsSerial->println(command);
  delay(100);
  gpsSerial->begin(baudRate);
}