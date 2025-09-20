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
 * Revision     :     1.2
 * Rev1.0       :     Original GPS implementation
 * Rev1.1       :     Update LED behavior for GPS fix status
 * Rev1.2       :     - Add timeout handling for GPS signal loss
 *                    - Use TinyGPS++ for robust NMEA parsing
 *                    - Add RTOS support for better task management by gpsTask function
 *                    - Thailand timezone UTC+7, LED behavior updated
 * website      :     http://www.tenergyinnovation.co.th
 * Email        :     uten.boonliam@tenergyinnovation.co.th
 * TEL          :     +66 89-140-7205
 ***********************************************************************/
#include <Arduino.h>
#include <tenergy32hub.h>
#include <esp_task_wdt.h>
#include <esp_system.h> // สำหรับ esp_read_mac
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
// TinyGPS++ for robust NMEA parsing
#include <TinyGPS++.h>

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
const char *FW_VERSION = "1.2"; // กำหนดเวอร์ชันของเฟิร์มแวร์

/**************************************/
/*          Header project            */
/**************************************/
// ฟังก์ชันสำหรับแสดงข้อมูล Header ของโปรเจกต์ผ่าน Serial Monitor
void header_print(void)
{
    Serial.printf("\r\n***********************************************************************\r\n");
    Serial.printf("* Project      :     smartbuilding360hub GPS GY-NEO6MV2 Ublox Reader\r\n");
    Serial.printf("* Description  :     GPS reader using TinyGPS++ on Tenergy32Hub (UTC+7 Thailand)\r\n");
    Serial.printf("* Hardware     :     tenergy32hub + GY-NEO6MV2 Ublox GPS Module\r\n");
    Serial.printf("* GPS Wiring   :     GPS TX->GPIO27, GPS RX->GPIO26\r\n");
    Serial.printf("* Author       :     Tenergy Innovation Co., Ltd.\r\n");
    Serial.printf("* Date         :     29/07/2025\r\n");
    Serial.printf("* Revision     :     %s\r\n", FW_VERSION);
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
// Parsing state (TinyGPS++ based)
volatile bool gpsDataReady = false;       // flag: new GPS data available
volatile unsigned long lastGPSUpdate = 0; // last GPS update timestamp

// Parsed numeric values (populated from TinyGPS++)
double latitude = 0.0;  // decimal degrees
double longitude = 0.0; // decimal degrees
String gpsTime = "";
String gpsDate = "";
int satellites = 0;
float hdop = 0.0;
bool gpsFixed = false;
// Timeout detection
static const unsigned long GPS_TIMEOUT_MS = 30000; // 30s
bool gpsTimedOut = false;
// GPS interface status
bool gpsInterfaceOk = false;

// Track current blue blink interval so we only call blink API when it changes
uint32_t currentBlueBlinkInterval = 0;
// Track current red blink interval so we only call blink API when it changes
uint32_t currentRedBlinkInterval = 0;

// Track previous states to force immediate OLED update when they change
bool lastGpsFixedState = false;
bool lastGpsTimedOutState = false;

// Task handle for GPS reader
TaskHandle_t gpsTaskHandle = NULL;

// TinyGPS++ instance
TinyGPSPlus tinyGPS;

/**************************************/
/*           define function          */
/**************************************/

// NOTE: Custom NMEA parsing removed. TinyGPS++ (tinyGPS) is now the sole parser.
// All NMEA sentence parsing and validation is delegated to TinyGPS++.

/***********************************************************************
 * FUNCTION:    readGPSData
 * DESCRIPTION: อ่านและประมวลผลข้อมูลจาก GPS module
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
// GPS UART reading task - feeds all bytes to TinyGPS++ and updates state
void gpsTask(void *pv)
{
    (void)pv;
    for (;;)
    {
        // read all available bytes and feed to TinyGPS++ parser
        while (gpsSerial.available())
        {
            char c = (char)gpsSerial.read();
            tinyGPS.encode(c);
        }

        // Update numeric state from TinyGPS++ when new data available
        if (tinyGPS.location.isUpdated() || tinyGPS.satellites.isUpdated() || tinyGPS.hdop.isUpdated())
        {
            if (tinyGPS.location.isValid())
            {
                latitude = tinyGPS.location.lat();
                longitude = tinyGPS.location.lng();
                gpsFixed = true;
            }
            else
            {
                gpsFixed = false;
            }
            if (tinyGPS.satellites.isValid())
            {
                satellites = tinyGPS.satellites.value();
            }
            if (tinyGPS.hdop.isValid())
            {
                hdop = tinyGPS.hdop.hdop();
            }
            // time/date (RMC) - convert to Thailand timezone (UTC+7)
            if (tinyGPS.time.isValid() && tinyGPS.date.isValid())
            {
                // get UTC components
                int hh = tinyGPS.time.hour();
                int mm = tinyGPS.time.minute();
                int ss = tinyGPS.time.second();
                int dd = tinyGPS.date.day();
                int mo = tinyGPS.date.month();
                int yy = tinyGPS.date.year(); // full year
                // apply timezone offset +7 hours
                int add = 7;
                hh += add;
                // handle overflow days
                if (hh >= 24)
                {
                    hh -= 24;
                    // naive day increment - adjust month/year if overflow
                    dd += 1;
                    // days per month (not handling leap-year Feb perfectly for 2100 etc.)
                    int mdays = 31;
                    if (mo == 4 || mo == 6 || mo == 9 || mo == 11)
                        mdays = 30;
                    else if (mo == 2)
                    {
                        // leap year
                        int y = yy;
                        bool leap = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
                        mdays = leap ? 29 : 28;
                    }
                    if (dd > mdays)
                    {
                        dd = 1;
                        mo += 1;
                        if (mo > 12)
                        {
                            mo = 1;
                            yy += 1;
                        }
                    }
                }
                // format HHMMSS and DDMMYY (two-digit year)
                char tbuf[16];
                char dbuf[16];
                snprintf(tbuf, sizeof(tbuf), "%02d%02d%02d", hh, mm, ss);
                snprintf(dbuf, sizeof(dbuf), "%02d%02d%02d", dd, mo, yy % 100);
                gpsTime = String(tbuf);
                gpsDate = String(dbuf);
            }

            gpsDataReady = true;
            lastGPSUpdate = millis();
            // clear timeout status when data returns
            gpsTimedOut = false;
            // interface ok once we have data
            gpsInterfaceOk = true;
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Boot self-test: check OLED init and GPS serial presence
void bootSelfTest()
{
    // Try display info (library handles I2C init)
    mcu.displayOLEDInfo();
    vTaskDelay(pdMS_TO_TICKS(500));
    // quick GPS serial check
    if (!gpsSerial)
    {
        mcu.displayOLEDLines("SELF-TEST:", "GPS Serial Failed", "Check wiring", "");
        Serial.println("SELF-TEST: GPS Serial Failed, Check wiring");
        mcu.beep(3);
    }
    else
    {
        mcu.displayOLEDLines("SELF-TEST:", "OK: OLED & GPS", "Booting...", "");
        Serial.println("SELF-TEST: OK OLED & GPS");
        // mcu.beep(1);
    }
    vTaskDelay(pdMS_TO_TICKS(800)); 
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
void displayGPSInfoOLED()
{
    char line1[22], line2[22], line3[22], line4[22];

    // satellite count string
    char satBuf[8];
    snprintf(satBuf, sizeof(satBuf), "%d", satellites);

    // evaluate signal quality from hdop (numeric)
    const char *signalQuality;
    if (hdop > 0.0f)
    {
        if (hdop <= 1.0f)
            signalQuality = "Excellent";
        else if (hdop <= 2.0f)
            signalQuality = "Good";
        else if (hdop <= 5.0f)
            signalQuality = "Moderate";
        else if (hdop <= 10.0f)
            signalQuality = "Fair";
        else
            signalQuality = "Poor";
    }
    else
    {
        signalQuality = "Unknown";
    }

    if (gpsFixed)
    {
        // Show GPS OK with satellite count
        snprintf(line1, sizeof(line1), "GPS:OK Sat:%s", satBuf);
        // Latitude/Longitude trimmed to fit display
        char latBuf[20], lonBuf[20];
        snprintf(latBuf, sizeof(latBuf), "%.6f", latitude);
        snprintf(lonBuf, sizeof(lonBuf), "%.6f", longitude);
        // place into lines (keep width)
        snprintf(line2, sizeof(line2), "Lat:%s", latBuf);
        snprintf(line3, sizeof(line3), "Lon:%s", lonBuf);
        // show time/date if available in human readable form HH:MM:SS and DD/MM/YY
        if (gpsTime.length() >= 6 && gpsDate.length() >= 6)
        {
            char tdisplay[22];
            // gpsTime expected as HHMMSS or HHMM
            unsigned int hh = 0, mm = 0, ss = 0;
            if (gpsTime.length() >= 6)
            {
                hh = (gpsTime.charAt(0) - '0') * 10 + (gpsTime.charAt(1) - '0');
                mm = (gpsTime.charAt(2) - '0') * 10 + (gpsTime.charAt(3) - '0');
                ss = (gpsTime.charAt(4) - '0') * 10 + (gpsTime.charAt(5) - '0');
            }
            else if (gpsTime.length() >= 4)
            {
                hh = (gpsTime.charAt(0) - '0') * 10 + (gpsTime.charAt(1) - '0');
                mm = (gpsTime.charAt(2) - '0') * 10 + (gpsTime.charAt(3) - '0');
            }
            // gpsDate expected as DDMMYY
            unsigned int dd = 0, mo = 0, yy = 0;
            if (gpsDate.length() >= 6)
            {
                dd = (gpsDate.charAt(0) - '0') * 10 + (gpsDate.charAt(1) - '0');
                mo = (gpsDate.charAt(2) - '0') * 10 + (gpsDate.charAt(3) - '0');
                yy = (gpsDate.charAt(4) - '0') * 10 + (gpsDate.charAt(5) - '0');
            }
            snprintf(tdisplay, sizeof(tdisplay), "%02u:%02u:%02u %02u/%02u/%02u", hh, mm, ss, dd, mo, yy);
            snprintf(line4, sizeof(line4), "%s", tdisplay);
        }
        else
        {
            snprintf(line4, sizeof(line4), "%s Ready!", signalQuality);
        }
        // LED state is handled centrally in loop() to avoid conflicting updates
    }
    else
    {
        snprintf(line1, sizeof(line1), "GPS:Searching...");
        snprintf(line2, sizeof(line2), "Sat:%s", satBuf);
        snprintf(line3, sizeof(line3), "Signal:%s", signalQuality);
        snprintf(line4, sizeof(line4), "Data: Not Ready");
        // LED state is handled centrally in loop() to avoid conflicting updates
    }

    mcu.displayOLEDLines(line1, line2, line3, line4);
}

/***********************************************************************
 * FUNCTION:    displayGPSInfo
 * DESCRIPTION: แสดงข้อมูล GPS บน Serial Monitor และ OLED (ฟังก์ชันเดิม)
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
// Legacy-compatible stub: main loop used to call readGPSData();
// Now the actual reader is running in gpsTask background.
void readGPSData()
{
    // no-op: parsing handled in gpsTask
}
void displayGPSInfo()
{
    Serial.println("=== GPS Information ===");
    Serial.printf("GPS Status: %s\n", gpsFixed ? "FIXED" : "NO FIX"); //Fixed หมายถึงได้ตำแหน่งแล้ว, NO FIX หมายถึงยังไม่ได้ตำแหน่ง
    Serial.printf("Time: %s\n", gpsTime.c_str());
    Serial.printf("Date: %s\n", gpsDate.c_str());
    Serial.printf("Latitude: %.6f\n", latitude);
    Serial.printf("Longitude: %.6f\n", longitude);
    Serial.printf("Satellites: %d\n", satellites);
    Serial.printf("HDOP: %.2f\n", hdop);
    Serial.println("========================");

    // also update OLED with more readable values
    displayGPSInfoOLED();
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

    vTaskDelay(2000); // หน่วงเวลา 2 วินาที

    // ทดสอบ LED
    mcu.setBlueLED(false); // เปิด LED สีน้ำเงิน
    mcu.setRedLED(false); // ปิด LED สีแดง

    // เริ่มต้น GPS Serial communication
    Serial.println("Initializing GPS Module...");
    gpsSerial.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    vTaskDelay(1000); // wait a bit for serial to stabilize


    // perform boot self-test (OLED init and GPS serial check)
    bootSelfTest();


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
    mcu.beep(2);

    mcu.setBlueLED(false); // ปิด LED สีน้ำเงิน
    mcu.setRedLED(false);  // ปิด LED สีแดง
    // start gpsTask if not started
    if (gpsTaskHandle == NULL)
    {
        xTaskCreatePinnedToCore(gpsTask, "gpsTask", 4096, NULL, 2, &gpsTaskHandle, 1);
    }
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
    while (gpsSerial.available())
    {
        char c = gpsSerial.read();
        // Serial.print(c); // comment out เพื่อลดการแสดงผล raw data
        messageCount++;
    }

    unsigned long currentTime = millis();

    // อัพเดทข้อมูลบน OLED ทุก 2 วินาที
    if (currentTime - lastOLEDUpdate > 2000)
    {
        // if timed out, show warning instead
        if (gpsTimedOut)
        {
            mcu.displayOLEDLines("GPS Warning:", "No data >30s", "Check antenna", "and wiring");
            mcu.setBlueLED(false);
            mcu.setRedLED(true);
        }
        else
        {
            displayGPSInfoOLED();
        }
        lastOLEDUpdate = currentTime;
    }

    // แสดงข้อมูลสำหรับ debug ทุก 10 วินาที
    if (currentTime - lastDebugTime > 10000)
    {
        Serial.println("\n=== GPS Status Debug ===");
        Serial.printf("Uptime: %lu seconds\n", currentTime / 1000);
        Serial.printf("Messages received: %d\n", messageCount);
        Serial.printf("GPS Connection: %s\n", gpsSerial.available() ? "OK" : "FAILED");
        Serial.printf("GPS Status: %s\n", gpsFixed ? "FIXED" : "NO FIX");
        Serial.printf("Satellites: %d\n", satellites);
        Serial.printf("HDOP: %.2f\n", hdop);
        Serial.printf("Latitude: %.6f\n", latitude);
        Serial.printf("Longitude: %.6f\n", longitude);
        Serial.printf("Time: %s\n", gpsTime.c_str());
        Serial.printf("Date: %s\n", gpsDate.c_str());
        Serial.println("========================\n");

        lastDebugTime = currentTime;
        messageCount = 0; // รีเซ็ตตัวนับ
    }

    // Check GPS timeout (transition detection)
    if (!gpsTimedOut && lastGPSUpdate > 0 && (currentTime - lastGPSUpdate > GPS_TIMEOUT_MS))
    {
        // first time we notice timeout: notify
        gpsTimedOut = true;
        Serial.println("Warning: No GPS data received for 30 seconds");
        mcu.displayOLEDLines("GPS Warning!", "No data for 30s", "Check connections", "and antenna");
        mcu.beep(1, 500);
    }

    // LED behavior using library blink functions
    // Priority: gpsTimedOut overrides satellite-based blue blinking
    if (gpsTimedOut)
    {
        // When timed out: Blue OFF, Red blink fast (100 ms period)
        if (currentBlueBlinkInterval != 0)
        {
            currentBlueBlinkInterval = 0;
            mcu.blinkBlueLED(0); // stop blue blinking
            mcu.setBlueLED(false);
        }

        uint32_t desiredRedInterval = 100u; // 100 ms period
        if (desiredRedInterval != currentRedBlinkInterval)
        {
            currentRedBlinkInterval = desiredRedInterval;
            mcu.blinkRedLED(currentRedBlinkInterval);
        }
    }
    else
    {
        // Not timed out: control blue LED based on satellite count
        uint32_t desiredBlueInterval = (satellites > 0) ? 1000u : 100u;
        if (desiredBlueInterval != currentBlueBlinkInterval)
        {
            currentBlueBlinkInterval = desiredBlueInterval;
            mcu.blinkBlueLED(currentBlueBlinkInterval);
        }

        // Ensure red LED is not blinking when interface OK
        if (!gpsInterfaceOk)
        {
            // interface not OK -> stop any red blink and force ON
            if (currentRedBlinkInterval != 0)
            {
                currentRedBlinkInterval = 0;
                mcu.blinkRedLED(0);
            }
            mcu.setRedLED(true);
        }
        else
        {
            // interface OK -> make sure red is off and not blinking
            if (currentRedBlinkInterval != 0)
            {
                currentRedBlinkInterval = 0;
                mcu.blinkRedLED(0);
            }
            mcu.setRedLED(false);
        }
    }

    // Force immediate OLED update when gpsFixed or gpsTimedOut state changes
    if (gpsFixed != lastGpsFixedState || gpsTimedOut != lastGpsTimedOutState)
    {
        // immediate update
        if (gpsTimedOut)
        {
            mcu.displayOLEDLines("GPS Warning:", "No data >30s", "Check antenna", "and wiring");
        }
        else
        {
            displayGPSInfoOLED();
        }
        lastGpsFixedState = gpsFixed;
        lastGpsTimedOutState = gpsTimedOut;
    }

    // ตรวจสอบปุ่มกด
    if (mcu.readSW1())
    {
        Serial.println("SW1 Pressed - GPS Detailed Info");
        displayGPSInfo(); // แสดงข้อมูลแบบละเอียดใน Serial
        mcu.beep(1, 100);
        delay(500); // debounce
    }

    if (mcu.readSW2())
    {
        Serial.println("SW2 Pressed - System restart");
        mcu.beep(3, 100);
        ESP.restart();
    }

    delay(100);           // หน่วงเวลาเล็กน้อยเพื่อไม่ให้ CPU ทำงานหนักเกินไป
    esp_task_wdt_reset(); // รีเซ็ต Watchdog Timer
}

void loop_org()
{
    // อ่านข้อมูลจาก GPS module
    readGPSData();

    // ตรวจสอบว่ามีข้อมูล GPS ใหม่หรือไม่
    if (gpsDataReady)
    {
        displayGPSInfo();
        gpsDataReady = false;
    }

    // ตรวจสอบว่าไม่ได้รับข้อมูล GPS มานานเกินไป (30 วินาที)
    unsigned long currentTime = millis();
    if (currentTime - lastGPSUpdate > 30000 && lastGPSUpdate > 0)
    {
        Serial.println("Warning: No GPS data received for 30 seconds");
        mcu.displayOLEDLines("GPS Warning!", "No data for 30s", "Check connections", "and antenna");
        mcu.beep(1, 500);
    }

    // แสดงข้อมูลสถานะทุก 5 วินาที หากไม่มี GPS fix
    static unsigned long lastStatusUpdate = 0;
    if (!gpsFixed && (currentTime - lastStatusUpdate > 5000))
    {
        Serial.printf("GPS Status: Searching for satellites... (Uptime: %lu seconds)\n", currentTime / 1000);
        lastStatusUpdate = currentTime;

        // แสดง raw GPS data ที่รับได้ (สำหรับ debug)
        Serial.println("--- Raw GPS Data (last 5 seconds) ---");
        while (gpsSerial.available())
        {
            Serial.write(gpsSerial.read());
        }
        Serial.println("--- End Raw Data ---");
    }

    // ตรวจสอบสถานะปุ่มกด
    if (mcu.readSW1())
    {
        Serial.println("SW1 Pressed - Forcing GPS info display");
        displayGPSInfo();
        mcu.beep(1, 100);
        delay(500); // debounce
    }

    if (mcu.readSW2())
    {
        Serial.println("SW2 Pressed - System restart");
        mcu.beep(3, 100);
        ESP.restart();

        // start gpsTask if not started
        if (gpsTaskHandle == NULL)
        {
            xTaskCreatePinnedToCore(gpsTask, "gpsTask", 4096, NULL, 2, &gpsTaskHandle, 1);
        }
    }

    delay(100);           // หน่วงเวลาเล็กน้อยเพื่อไม่ให้ CPU ทำงานหนักเกินไป
    esp_task_wdt_reset(); // รีเซ็ต Watchdog Timer
}