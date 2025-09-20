#ifndef TENERGY32HUB_GY_NEO_8M_H
#define TENERGY32HUB_GY_NEO_8M_H

#include <Arduino.h>
#include <HardwareSerial.h>

struct GPSData {
  bool valid;
  float latitude;
  float longitude;
  float altitude;
  float speed;
  int satellites;
  String time;
  String date;
};

class Tenergy32Hub_GY_NEO_8M {
  private:
    HardwareSerial* gpsSerial;
    String nmeaBuffer;
    
    bool parseGPGGA(String sentence, GPSData& data);
    bool parseGPRMC(String sentence, GPSData& data);
    float convertToDecimalDegrees(String coordinate, String direction);
    
  public:
    Tenergy32Hub_GY_NEO_8M();
    
    void begin(HardwareSerial& serial, unsigned long baudRate = 9600);
    void begin(int rxPin, int txPin, unsigned long baudRate = 9600);
    
    bool available();
    GPSData readGPS();
    String readNMEA();
    
    void enableGGA(bool enable = true);
    void enableRMC(bool enable = true);
    void setBaudRate(unsigned long baudRate);
};

#endif