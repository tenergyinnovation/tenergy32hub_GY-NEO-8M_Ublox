# Tenergy32Hub GY-NEO-8M GPS Module Library

An Arduino library for interfacing with the GY-NEO-8M GPS module (U-blox chipset) designed for ESP32 and Arduino boards. This library provides easy-to-use functions for reading GPS data, parsing NMEA sentences, and configuring the GPS module.

## Features

- ✅ **Easy GPS data reading** - Simple interface to get latitude, longitude, altitude, speed, and more
- ✅ **NMEA parsing** - Automatic parsing of GPGGA and GPRMC sentences
- ✅ **Multiple initialization methods** - Use hardware serial or software serial
- ✅ **GPS configuration** - Enable/disable specific NMEA sentences
- ✅ **Real-time data** - Access to satellite count, time, date, and fix quality
- ✅ **Example sketches** - Basic and advanced usage examples included

## Hardware Compatibility

- **GPS Module**: GY-NEO-8M (U-blox NEO-8M chipset)
- **Microcontrollers**: ESP32, Arduino Uno, Arduino Nano, Arduino Mega, etc.
- **Communication**: UART (Hardware/Software Serial)

## Installation

### Arduino Library Manager
1. Open Arduino IDE
2. Go to `Tools` > `Manage Libraries`
3. Search for "Tenergy32Hub GY-NEO-8M"
4. Click Install

### Manual Installation
1. Download this repository as ZIP
2. In Arduino IDE: `Sketch` > `Include Library` > `Add .ZIP Library`
3. Select the downloaded ZIP file

## Hardware Connections

| GY-NEO-8M Pin | Arduino/ESP32 Pin | Description |
|---------------|-------------------|-------------|
| VCC           | 3.3V or 5V        | Power supply |
| GND           | GND               | Ground |
| RX            | GPIO 17 (or any)  | GPS receives data |
| TX            | GPIO 16 (or any)  | GPS transmits data |

## Quick Start

```cpp
#include <Tenergy32Hub_GY_NEO_8M.h>

Tenergy32Hub_GY_NEO_8M gps;

void setup() {
  Serial.begin(115200);
  
  // Initialize GPS with custom pins (ESP32)
  gps.begin(16, 17, 9600); // RX, TX, baud rate
  
  Serial.println("GPS initialized!");
}

void loop() {
  if (gps.available()) {
    GPSData data = gps.readGPS();
    
    if (data.valid) {
      Serial.print("Latitude: ");
      Serial.println(data.latitude, 6);
      Serial.print("Longitude: ");
      Serial.println(data.longitude, 6);
      Serial.print("Satellites: ");
      Serial.println(data.satellites);
    }
  }
  delay(1000);
}
```

## API Reference

### Class: `Tenergy32Hub_GY_NEO_8M`

#### Initialization Methods

```cpp
// Method 1: Use existing HardwareSerial
void begin(HardwareSerial& serial, unsigned long baudRate = 9600);

// Method 2: Create new serial connection (ESP32)
void begin(int rxPin, int txPin, unsigned long baudRate = 9600);
```

#### Data Reading Methods

```cpp
// Check if GPS data is available
bool available();

// Read parsed GPS data
GPSData readGPS();

// Read raw NMEA sentence
String readNMEA();
```

#### Configuration Methods

```cpp
// Enable/disable GPGGA sentences (position data)
void enableGGA(bool enable = true);

// Enable/disable GPRMC sentences (recommended minimum data)
void enableRMC(bool enable = true);

// Change GPS module baud rate
void setBaudRate(unsigned long baudRate);
```

### Struct: `GPSData`

```cpp
struct GPSData {
  bool valid;        // true if GPS has valid fix
  float latitude;    // Latitude in decimal degrees
  float longitude;   // Longitude in decimal degrees
  float altitude;    // Altitude in meters
  float speed;       // Speed in km/h
  int satellites;    // Number of satellites in use
  String time;       // Time in HH:MM:SS format
  String date;       // Date in DD/MM/YY format
};
```

## Examples

### Basic GPS Reading
See `examples/BasicGPS/BasicGPS.ino` for a simple GPS data reading example.

### Advanced Features
See `examples/AdvancedGPS/AdvancedGPS.ino` for advanced features including:
- Raw NMEA sentence display
- GPS quality assessment
- Google Maps link generation
- Interactive mode switching

## Troubleshooting

### GPS Not Getting Fix
- Ensure GPS module has clear view of sky
- Wait 30-60 seconds for cold start
- Check antenna connection
- Verify power supply (3.3V or 5V)

### No Data Received
- Check wiring connections
- Verify baud rate (default: 9600)
- Ensure RX/TX pins are not swapped
- Check if pins support serial communication

### Compilation Errors
- Ensure Arduino IDE has ESP32/Arduino board support installed
- Check that all required libraries are included
- Verify board selection matches your hardware

## Technical Specifications

- **GPS Chipset**: U-blox NEO-8M
- **Supported Protocols**: NMEA 0183
- **Update Rate**: Up to 10 Hz
- **Accuracy**: 2.5m CEP (Circular Error Probable)
- **Cold Start**: ~29 seconds
- **Warm Start**: ~1 second
- **Operating Voltage**: 3.3V - 5V
- **Current Consumption**: ~45mA (tracking)

## License

This library is released under the MIT License. See LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Support

- **Issues**: [GitHub Issues](https://github.com/tenergyinnovation/tenergy32hub_GY-NEO-8M_Ublox/issues)
- **Documentation**: This README and example files
- **Community**: Feel free to open discussions for questions

## Changelog

### v1.0.0 (Initial Release)
- Basic GPS data reading functionality
- NMEA sentence parsing (GPGGA, GPRMC)
- Hardware and software serial support
- GPS module configuration options
- Comprehensive examples and documentation

---

**Created by Tenergy Innovation**  
*Making GPS integration simple and reliable*
