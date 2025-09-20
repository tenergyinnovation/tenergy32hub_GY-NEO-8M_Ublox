/***********************************************************************
 * Project      :     smartbuilding360hub GPS GY-NEO6MV2 Ublox Reader
 * Description  :     GPS coordinate reading system using GY-NEO6MV2 Ublox module
 *                    with tenergy32hub ESP32 board
 * Hardware     :     tenergy32hub + GY-NEO6MV2 Ublox GPS Module
 *                    GPS Connections:
 *                    - GPS TX --> ESP32 GPIO 27 (RX)
 *                    - GPS RX --> ESP32 GPIO 26 (TX)
 * Author       :     Tenergy Innovation Co., Ltd.
 * Date         :     29/07/2025
 * Revision     :     1.0
 * Rev1.0       :     Original GPS implementation
 * website      :     http://www.tenergyinnovation.co.th
 * Email        :     uten.boonliam@tenergyinnovation.co.th
 * TEL          :     +66 89-140-7205
 ***********************************************************************/
#include <Arduino.h>
#include <tenergy32hub.h>
#include <esp_task_wdt.h>
#include <esp_system.h> // สำหรับ esp_read_mac

/**************************************/
/*          GPS Configuration         */
/**************************************/
#define GPS_RX_PIN 27      // ESP32 GPIO27 รับข้อมูลจาก GPS TX
#define GPS_TX_PIN 26      // ESP32 GPIO26 ส่งข้อมูลไป GPS RX
#define GPS_BAUD_RATE 9600 // ความเร็วการสื่อสาร GPS

// สร้าง HardwareSerial object สำหรับ GPS
HardwareSerial gpsSerial(1); // ใช้ Serial1

/**************************************/
/*          Firmware Version          */
/**************************************/
String version = "1.0"; // กำหนดเวอร์ชันของเฟิร์มแวร์

/**************************************/
/*          Header project            */
/**************************************/
// ฟังก์ชันสำหรับแสดงข้อมูล Header ของโปรเจกต์ผ่าน Serial Monitor
void header_print(void)
{
    Serial.printf("\r\n***********************************************************************\r\n");
    Serial.printf("* Project      :     smartbuilding360hub GPS GY-NEO6MV2 Ublox Reader\r\n");
    Serial.printf("* Description  :     GPS coordinate reading with tenergy32hub ESP32\r\n");
    Serial.printf("* Hardware     :     tenergy32hub + GY-NEO6MV2 Ublox GPS Module\r\n");
    Serial.printf("* GPS Wiring   :     GPS TX->GPIO27, GPS RX->GPIO26\r\n");
    Serial.printf("* Author       :     Tenergy Innovation Co., Ltd.\r\n");
    Serial.printf("* Date         :     29/07/2025\r\n");
    Serial.printf("* Revision     :     %s\r\n", version.c_str());
    Serial.printf("* Rev1.0       :     Original GPS implementation\r\n");
    Serial.printf("* website      :     http://www.tenergyinnovation.co.th\r\n");
    Serial.printf("* Email        :     uten.boonliam@tenergyinnovation.co.th\r\n");
    Serial.printf("* TEL          :     +66 89-140-7205\r\n");
    Serial.printf("***********************************************************************/\r\n");
}

/**************************************/
/*        define object variable      */
/**************************************/
Tenergy32Hub mcu; // สร้างอ็อบเจกต์ mcu สำหรับควบคุมบอร์ด tenergy32hub

/**************************************/
/*            GPIO define             */
/**************************************/
// GPS pins ได้กำหนดไว้ด้านบนแล้ว

/**************************************/
/*       Constant define value        */
/**************************************/
// กำหนดค่า timeout สำหรับ Watchdog Timer เป็น 10 วินาที
#define WDT_TIMEOUT 10

/**************************************/
/*        define global variable      */
/**************************************/
String gpsData = "";         // เก็บข้อมูล GPS ที่อ่านได้
bool gpsDataReady = false;   // สถานะว่ามีข้อมูล GPS ใหม่หรือไม่
unsigned long lastGPSUpdate = 0; // เวลาที่อัพเดท GPS ครั้งล่าสุด

// ตัวแปรสำหรับเก็บข้อมูล GPS ที่แยกแล้ว
String latitude = "";
String longitude = "";
String gpsTime = "";
String gpsDate = "";
String satellites = "";
String hdop = "";
bool gpsFixed = false;


/**************************************/
/*           define function          */
/**************************************/

/***********************************************************************
 * FUNCTION:    parseGPSData
 * DESCRIPTION: แยกข้อมูล NMEA sentence จาก GPS
 * PARAMETERS:  sentence - NMEA sentence string
 * RETURNED:    nothing
 ***********************************************************************/
void parseGPSData(String sentence) {
    if (sentence.startsWith("$GPGGA") || sentence.startsWith("$GNGGA")) {
        // GPGGA sentence contains: time, lat, lon, fix quality, satellites, hdop, altitude
        int commaIndex[15];
        int commaCount = 0;
        
        // หา position ของ comma ทั้งหมด
        for (int i = 0; i < sentence.length() && commaCount < 15; i++) {
            if (sentence.charAt(i) == ',') {
                commaIndex[commaCount] = i;
                commaCount++;
            }
        }
        
        if (commaCount >= 6) {
            // Time (hhmmss.ss)
            gpsTime = sentence.substring(commaIndex[0] + 1, commaIndex[1]);
            
            // Latitude
            String latStr = sentence.substring(commaIndex[1] + 1, commaIndex[2]);
            String latDir = sentence.substring(commaIndex[2] + 1, commaIndex[3]);
            if (latStr.length() > 0) {
                latitude = latStr + latDir;
            }
            
            // Longitude  
            String lonStr = sentence.substring(commaIndex[3] + 1, commaIndex[4]);
            String lonDir = sentence.substring(commaIndex[4] + 1, commaIndex[5]);
            if (lonStr.length() > 0) {
                longitude = lonStr + lonDir;
            }
            
            // Fix quality (0 = invalid, 1 = GPS fix, 2 = DGPS fix)
            String fixQuality = sentence.substring(commaIndex[5] + 1, commaIndex[6]);
            gpsFixed = (fixQuality.toInt() > 0);
            
            // Number of satellites
            if (commaCount > 6) {
                satellites = sentence.substring(commaIndex[6] + 1, commaIndex[7]);
            }
            
            // HDOP (Horizontal Dilution of Precision)
            if (commaCount > 7) {
                hdop = sentence.substring(commaIndex[7] + 1, commaIndex[8]);
            }
        }
    }
    else if (sentence.startsWith("$GPRMC") || sentence.startsWith("$GNRMC")) {
        // GPRMC sentence contains: time, status, lat, lon, speed, course, date
        int commaIndex[12];
        int commaCount = 0;
        
        // หา position ของ comma ทั้งหมด
        for (int i = 0; i < sentence.length() && commaCount < 12; i++) {
            if (sentence.charAt(i) == ',') {
                commaIndex[commaCount] = i;
                commaCount++;
            }
        }
        
        if (commaCount >= 9) {
            // Date (ddmmyy)
            gpsDate = sentence.substring(commaIndex[8] + 1, commaIndex[9]);
        }
    }
}

/***********************************************************************
 * FUNCTION:    readGPSData
 * DESCRIPTION: อ่านและประมวลผลข้อมูลจาก GPS module
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
void readGPSData() {
    static String currentSentence = "";
    
    while (gpsSerial.available()) {
        char c = gpsSerial.read();
        
        if (c == '\n') {
            // จบ sentence แล้ว, ทำการประมวลผล
            if (currentSentence.length() > 0) {
                parseGPSData(currentSentence);
                gpsDataReady = true;
                lastGPSUpdate = millis();
            }
            currentSentence = "";
        }
        else if (c != '\r') {
            // เก็บตัวอักษรใน sentence
            currentSentence += c;
        }
    }
}

/***********************************************************************
 * FUNCTION:    displayGPSInfoOLED
 * DESCRIPTION: แสดงข้อมูล GPS บน OLED ตามที่ต้องการ
 *              1. จำนวนดาวเทียมที่จับได้
 *              2. Latitude/Longitude
 *              3. คุณภาพสัญญาณ (HDOP)
 *              4. ข้อมูลนำไปใช้ได้หรือไม่
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
void displayGPSInfoOLED() {
    char line1[22], line2[22], line3[22], line4[22];
    
    // เตรียมข้อมูลสำหรับแสดงผล
    String satCount = satellites.length() > 0 ? satellites : "0";
    String signalQuality = "";
    
    // ประเมินคุณภาพสัญญาณจาก HDOP
    if (hdop.length() > 0) {
        float hdopValue = hdop.toFloat();
        if (hdopValue <= 1.0) {
            signalQuality = "Excellent";
        } else if (hdopValue <= 2.0) {
            signalQuality = "Good";
        } else if (hdopValue <= 5.0) {
            signalQuality = "Moderate";
        } else if (hdopValue <= 10.0) {
            signalQuality = "Fair";
        } else {
            signalQuality = "Poor";
        }
    } else {
        signalQuality = "Unknown";
    }
    
    if (gpsFixed) {
        // เมื่อมีการจับสัญญาณ GPS แล้ว
        snprintf(line1, sizeof(line1), "GPS:OK Sat:%s", satCount.c_str());
        
        // แสดง Latitude (ย่อให้พอดี)
        String latDisplay = latitude.length() > 12 ? latitude.substring(0, 12) : latitude;
        snprintf(line2, sizeof(line2), "Lat:%s", latDisplay.c_str());
        
        // แสดง Longitude (ย่อให้พอดี)
        String lonDisplay = longitude.length() > 12 ? longitude.substring(0, 12) : longitude;
        snprintf(line3, sizeof(line3), "Lon:%s", lonDisplay.c_str());
        
        // แสดงคุณภาพสัญญาณและสถานะ
        snprintf(line4, sizeof(line4), "%s Ready!", signalQuality.c_str());
        
        // เปิด LED สีฟ้าเมื่อจับสัญญาณได้
        mcu.setBlueLED(true);
        mcu.setRedLED(false);
        
    } else {
        // เมื่อยังไม่จับสัญญาณได้
        snprintf(line1, sizeof(line1), "GPS:Searching...");
        snprintf(line2, sizeof(line2), "Satellites:%s", satCount.c_str());
        snprintf(line3, sizeof(line3), "Signal:%s", signalQuality.c_str());
        snprintf(line4, sizeof(line4), "Data: Not Ready");
        
        // เปิด LED สีแดงเมื่อยังไม่พร้อม
        mcu.setBlueLED(false);
        mcu.setRedLED(true);
    }
    
    // แสดงผลบน OLED
    mcu.displayOLEDLines(line1, line2, line3, line4);
}

/***********************************************************************
 * FUNCTION:    displayGPSInfo
 * DESCRIPTION: แสดงข้อมูล GPS บน Serial Monitor และ OLED (ฟังก์ชันเดิม)
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
void displayGPSInfo() {
    Serial.println("=== GPS Information ===");
    Serial.printf("GPS Status: %s\n", gpsFixed ? "FIXED" : "NO FIX");
    Serial.printf("Time: %s\n", gpsTime.c_str());
    Serial.printf("Date: %s\n", gpsDate.c_str());
    Serial.printf("Latitude: %s\n", latitude.c_str());
    Serial.printf("Longitude: %s\n", longitude.c_str());
    Serial.printf("Satellites: %s\n", satellites.c_str());
    Serial.printf("HDOP: %s\n", hdop.c_str());
    Serial.println("========================");
    
    // แสดงบน OLED
    char line1[22], line2[22], line3[22], line4[22];
    
    if (gpsFixed) {
        snprintf(line1, sizeof(line1), "GPS: FIXED (%s)", satellites.c_str());
        snprintf(line2, sizeof(line2), "Time: %s", gpsTime.substring(0, 6).c_str());
        snprintf(line3, sizeof(line3), "Lat: %s", latitude.substring(0, 10).c_str());
        snprintf(line4, sizeof(line4), "Lon: %s", longitude.substring(0, 10).c_str());
    } else {
        snprintf(line1, sizeof(line1), "GPS: Searching...");
        snprintf(line2, sizeof(line2), "Satellites: %s", satellites.c_str());
        snprintf(line3, sizeof(line3), "Time: %s", gpsTime.substring(0, 8).c_str());
        snprintf(line4, sizeof(line4), "Status: NO FIX");
    }
    
    mcu.displayOLEDLines(line1, line2, line3, line4);
}


/***********************************************************************
 * FUNCTION:    setup
 * DESCRIPTION: setup process
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
// ฟังก์ชัน setup() จะถูกเรียกเมื่อบอร์ดเริ่มทำงาน
void setup()
{
    Serial.begin(115200); // เริ่มต้น Serial ที่ baudrate 115200
    header_print();       // แสดงข้อมูล Header ของโปรเจกต์

    // เริ่มต้นการทำงานของบอร์ด tenergy32hub
    mcu.begin();           
    mcu.displayOLEDInfo(); 
    vTaskDelay(2000);      // หน่วงเวลา 2 วินาที

    // ทดสอบ LED
    mcu.setBlueLED(true); // เปิด LED สีน้ำเงิน
    mcu.setRedLED(true);  // เปิด LED สีแดง

    // เริ่มต้น GPS Serial communication
    Serial.println("Initializing GPS Module...");
    gpsSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    
    // แสดงข้อความเริ่มต้น GPS
    mcu.displayOLED("GPS Initializing...");
    vTaskDelay(1000);
    
    Serial.printf("GPS Serial initialized on pins RX:%d TX:%d at %d baud\n", 
                  GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD_RATE);
    Serial.println("Waiting for GPS data...");
    Serial.println("OLED Display Information:");
    Serial.println("- Line 1: GPS Status & Satellite count");
    Serial.println("- Line 2: Latitude coordinate");
    Serial.println("- Line 3: Longitude coordinate"); 
    Serial.println("- Line 4: Signal quality & Data status");
    Serial.println("LED Indicators:");
    Serial.println("- Blue LED: GPS Fixed (data ready)");
    Serial.println("- Red LED: GPS Searching (data not ready)");
    
    // เริ่มต้น Watchdog Timer
    esp_task_wdt_init(WDT_TIMEOUT, true); 
    esp_task_wdt_add(NULL);               
    
    // บี๊บสัญญาณเริ่มต้นสำเร็จ
    mcu.beep(2, 200);

    mcu.setBlueLED(false); // ปิด LED สีน้ำเงิน
    mcu.setRedLED(false);  // ปิด LED สีแดง
}

/***********************************************************************
 * FUNCTION:    loop
 * DESCRIPTION: loop process
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
// ฟังก์ชัน loop() จะทำงานวนซ้ำตลอดเวลา

void loop()
{
    static unsigned long lastDebugTime = 0;
    static unsigned long lastOLEDUpdate = 0;
    static int messageCount = 0;
    
    // อ่านและประมวลผลข้อมูล GPS
    readGPSData();
    
    // อ่านและนับข้อมูล raw GPS data
    while (gpsSerial.available()) {
        char c = gpsSerial.read();
        // Serial.print(c); // comment out เพื่อลดการแสดงผล raw data
        messageCount++;
    }
    
    unsigned long currentTime = millis();
    
    // อัพเดทข้อมูลบน OLED ทุก 2 วินาที
    if (currentTime - lastOLEDUpdate > 2000) {
        displayGPSInfoOLED();
        lastOLEDUpdate = currentTime;
    }
    
    // แสดงข้อมูลสำหรับ debug ทุก 10 วินาที
    if (currentTime - lastDebugTime > 10000) {
        Serial.println("\n=== GPS Status Debug ===");
        Serial.printf("Uptime: %lu seconds\n", currentTime / 1000);
        Serial.printf("Messages received: %d\n", messageCount);
        Serial.printf("GPS Connection: %s\n", gpsSerial ? "OK" : "FAILED");
        Serial.printf("GPS Status: %s\n", gpsFixed ? "FIXED" : "NO FIX");
        Serial.printf("Satellites: %s\n", satellites.c_str());
        Serial.printf("HDOP: %s\n", hdop.c_str());
        Serial.printf("Latitude: %s\n", latitude.c_str());
        Serial.printf("Longitude: %s\n", longitude.c_str());
        Serial.printf("Time: %s\n", gpsTime.c_str());
        Serial.printf("Date: %s\n", gpsDate.c_str());
        Serial.println("========================\n");
        
        lastDebugTime = currentTime;
        messageCount = 0; // รีเซ็ตตัวนับ
    }
    
    // ตรวจสอบปุ่มกด
    if (mcu.readSW1()) {
        Serial.println("SW1 Pressed - GPS Detailed Info");
        displayGPSInfo(); // แสดงข้อมูลแบบละเอียดใน Serial
        mcu.beep(1, 100);
        delay(500); // debounce
    }
    
    if (mcu.readSW2()) {
        Serial.println("SW2 Pressed - System restart");
        mcu.beep(3, 100);
        ESP.restart();
    }
    
    delay(100); // หน่วงเวลาเล็กน้อยเพื่อไม่ให้ CPU ทำงานหนักเกินไป
    esp_task_wdt_reset(); // รีเซ็ต Watchdog Timer
}

void loop_org()
{
    // อ่านข้อมูลจาก GPS module
    readGPSData();
    
    // ตรวจสอบว่ามีข้อมูล GPS ใหม่หรือไม่
    if (gpsDataReady) {
        displayGPSInfo();
        gpsDataReady = false;
    }
    
    // ตรวจสอบว่าไม่ได้รับข้อมูล GPS มานานเกินไป (30 วินาที)
    unsigned long currentTime = millis();
    if (currentTime - lastGPSUpdate > 30000 && lastGPSUpdate > 0) {
        Serial.println("Warning: No GPS data received for 30 seconds");
        mcu.displayOLEDLines("GPS Warning!", "No data for 30s", "Check connections", "and antenna");
        mcu.beep(1, 500);
    }
    
    // แสดงข้อมูลสถานะทุก 5 วินาที หากไม่มี GPS fix
    static unsigned long lastStatusUpdate = 0;
    if (!gpsFixed && (currentTime - lastStatusUpdate > 5000)) {
        Serial.printf("GPS Status: Searching for satellites... (Uptime: %lu seconds)\n", currentTime / 1000);
        lastStatusUpdate = currentTime;
        
        // แสดง raw GPS data ที่รับได้ (สำหรับ debug)
        Serial.println("--- Raw GPS Data (last 5 seconds) ---");
        while (gpsSerial.available()) {
            Serial.write(gpsSerial.read());
        }
        Serial.println("--- End Raw Data ---");
    }
    
    // ตรวจสอบสถานะปุ่มกด
    if (mcu.readSW1()) {
        Serial.println("SW1 Pressed - Forcing GPS info display");
        displayGPSInfo();
        mcu.beep(1, 100);
        delay(500); // debounce
    }
    
    if (mcu.readSW2()) {
        Serial.println("SW2 Pressed - System restart");
        mcu.beep(3, 100);
        ESP.restart();
    }
    
    delay(100); // หน่วงเวลาเล็กน้อยเพื่อไม่ให้ CPU ทำงานหนักเกินไป
    esp_task_wdt_reset(); // รีเซ็ต Watchdog Timer
}