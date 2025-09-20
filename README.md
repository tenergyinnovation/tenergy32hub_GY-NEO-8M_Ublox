Tenergy32Hub GPS GY-NEO6MV2 (Ublox) Reader
=========================================

ภาพรวม
-------
โปรเจกต์นี้เป็นเฟิร์มแวร์สำหรับบอร์ด Tenergy32Hub (ESP32) เพื่ออ่านข้อมูล GPS จากโมดูล GY-NEO6MV2 (Ublox) แล้วแสดงผลทั้งบน Serial Monitor และ OLED (SSD1306) โดยใช้ไลบรารี TinyGPS++ เป็นตัวแยกวิเคราะห์ NMEA อย่างทนทาน และใช้ API ของไลบรารี `tenergy32hub` สำหรับการควบคุม LED, BEEP และ OLED display

จุดประสงค์
---------
- อ่านตำแหน่ง (latitude/longitude), เวลา (UTC -> ปรับเป็น UTC+7), จำนวนดาวเทียม, และ HDOP
- แสดงสถานะบน OLED และ Serial
- ให้สัญญาณ LED ตามสถานะ GPS เพื่อการสังเกตที่รวดเร็ว
- มี logic ทนทานต่อข้อมูล NMEA ผิดพลาด, timeouts และการตรวจสอบ interface

ฮาร์ดแวร์ (Wiring)
------------------
- Tenergy32Hub (ESP32)
- GY-NEO6MV2 (Ublox GPS)
- SSD1306 OLED (I2C)

การเชื่อมต่อหลัก (ตามค่าในโค้ด):
- GPS TX -> ESP32 GPIO27 (รับข้อมูล)  (GPS_TX -> ESP32 RX)
- GPS RX -> ESP32 GPIO26 (ส่งข้อมูล)  (GPS_RX -> ESP32 TX)
- OLED SDA -> GPIO21, SCL -> GPIO22 (ตามการตั้งค่า lib บอร์ด)

ไฟล์สำคัญ
---------
- `src/main.cpp` - โค้ดหลักของโปรเจกต์: gpsTask (FreeRTOS), TinyGPS++ feed, การแสดงผล OLED และ logic LED
- `lib/tenergy32hub/` - ไลบรารีท้องถิ่นสำหรับการควบคุมบอร์ด (LED, Beep, OLED helper, blink API)
- `platformio.ini` - การตั้งค่าโปรเจกต์ PlatformIO และ dependencies (TinyGPSPlus)

พฤติกรรม LED (กฎที่ใช้ใน firmware v1.2)
-----------------------------------------
- Priority: ถ้า `gpsTimedOut == true` (ไม่มีข้อมูลจาก GPS เกิน 30 วินาที)
  - Blue LED: OFF
  - Red LED: กระพริบเร็ว (period = 100 ms)

- ถ้า `gpsTimedOut == false` (ข้อมูลยังมา)
  - ถ้า `satellites > 0` (มีดาวเทียมที่จับได้)
    - Blue LED: กระพริบช้า (period = 1000 ms)
  - ถ้า `satellites == 0` (ยังหาดาวเทียมไม่เจอ)
    - Blue LED: กระพริบเร็ว (period = 100 ms)
  - Red LED:
    - ถ้า `gpsInterfaceOk == false` (ยังไม่เคยได้รับข้อมูลจาก GPS)
      - Red LED: ติดค้าง (ON)
    - ถ้า `gpsInterfaceOk == true` (interface ทำงานปกติ)
      - Red LED: OFF

หมายเหตุ
- การกระพริบ LED ถูกควบคุมผ่าน API ของ `tenergy32hub`:
  - `mcu.blinkBlueLED(intervalMillis)` และ `mcu.blinkRedLED(intervalMillis)` — รับค่าระยะเวลารวมของรอบการกระพริบ (period)
  - เรียก API เหล่านี้เฉพาะเมื่อ desired interval เปลี่ยน เพื่อหลีกเลี่ยงการเรียก Ticker ซ้ำ ๆ

การทำงานภายในหลัก
-------------------
- `gpsTask` (FreeRTOS task):
  - ถูกรันเป็นลูปและเรียก `vTaskDelay(pdMS_TO_TICKS(50))` (ประมาณ 50 ms) ต่อรอบ
  - ในแต่ละรอบจะอ่านทุกไบต์ที่อยู่ใน `gpsSerial` และ feed ให้ `tinyGPS.encode(c)` ทันที
  - เมื่อ TinyGPS++ รายงานข้อมูลที่อัปเดต (location/satellites/hdop), task จะอัปเดตตัวแปร global (`latitude`, `longitude`, `satellites`, `hdop`, `gpsTime`, `gpsDate`) และตั้ง `lastGPSUpdate = millis()` และ `gpsTimedOut = false` และ `gpsInterfaceOk = true`

- main `loop()`:
  - อัปเดต OLED ทุก 2 วินาที (แสดงตำแหน่ง เวลา จำนวนดาวเทียม HDOP และข้อความสถานะ)
  - ตรวจสอบ `gpsTimedOut`: ถ้า `millis() - lastGPSUpdate > GPS_TIMEOUT_MS` (ค่าปัจจุบัน 30,000 ms) จะตั้ง `gpsTimedOut = true` และแจ้งเตือน
  - ควบคุม LED ตามกฎที่ระบุด้านบน โดยใช้ `currentBlueBlinkInterval` และ `currentRedBlinkInterval` เพื่อเรียก blink API เฉพาะเมื่อมีการเปลี่ยนแปลง

ทำไม `gpsTimedOut` เป็น true?
-----------------------------
- `gpsTimedOut` จะเป็น true หากไม่มีการอัปเดต `lastGPSUpdate` เป็นเวลามากกว่า `GPS_TIMEOUT_MS` (ปัจจุบันตั้ง 30 วินาที)
- สาเหตุที่ทำให้ `lastGPSUpdate` ไม่ถูกอัปเดตได้แก่:
  - GPS module หยุดส่งข้อมูล (module ปิด, ขาดไฟจ่าย, antenna หัก/ไม่เชื่อมต่อ)
  - สายเชื่อมต่อผิดพินหรือการเชื่อมต่อไม่แน่น
  - Baud rate ผิด (ไม่ตรงกับ GPS module)
  - TinyGPS++ ไม่ได้รับประโยค NMEA ที่สมบูรณ์ (เช่น checksum ผิด) จึงไม่อัปเดตสถานะ
  - หาก task ถูกกีดกันถูก block โดย task อื่นนาน ๆ (ไม่ค่อยเป็นไปได้ในโค้ดปัจจุบัน)

การคอมไพล์และแฟลช
------------------
1. ติดตั้ง PlatformIO และเปิด workspace นี้
2. ในโฟลเดอร์โปรเจกต์ รัน:

```bash
pio run         # build
pio run --target upload  # upload to connected device (ต้องต่อบอร์ด)
```

การดีบัก (ข้อเสนอแนะ)
--------------------
- ถ้าพบว่า OLED ไม่อัปเดต ให้เปิด Serial Monitor และดู log ว่า `gpsTimedOut` ถูกสลับหรือไม่ และ TinyGPS++ ได้รายงาน `satellites` หรือ `location` หรือไม่
- เพิ่ม Serial prints ใน `gpsTask` เมื่อได้รับประโยค NMEA ที่ valid (ผมสามารถเพิ่มให้ได้)
- ตรวจสอบสาย wiring, และลองเช็คว่า GPS module ส่งข้อมูล NMEA ด้วยการต่อสาย TX ของ GPS ไปยัง USB-to-Serial แล้วดู raw data

Changelog (สำคัญ)
-----------------
- v1.2 (29/07/2025)
  - ผนวก TinyGPS++ เป็น parser หลัก
  - ปรับเวลาเป็น UTC+7 (ไทย)
  - ปรับ logic LED ตามจำนวนดาวเทียมและ timeout
  - ปรับปรุงการอัปเดต OLED ให้ไม่ค้างเมื่อสถานะเปลี่ยน

License
-------
โปรเจกต์นี้ใช้โค้ดตัวอย่าง/ไลบรารีสาธารณะ (TinyGPS++ และไลบรารีของบอร์ด) โปรดตรวจสอบ license ของไลบรารีที่ใช้งานเมื่อแจกจ่าย

ผู้ติดต่อ
---------
Tenergy Innovation Co., Ltd.
Email: uten.boonliam@tenergyinnovation.co.th

*** End of README ***
Tenergy32Hub + GY-NEO6MV2 (Ublox) GPS Reader

Overview

This project reads GPS data from a GY-NEO6MV2 (Ublox) module using an ESP32-based Tenergy32Hub board. The firmware uses TinyGPS++ as the primary NMEA parser and displays status on an SSD1306 OLED. The code runs the GPS UART reader in a FreeRTOS task and updates both Serial and OLED outputs.

Wiring

- GPS TX -> ESP32 GPIO27 (GPS_RX_PIN)
- GPS RX -> ESP32 GPIO26 (GPS_TX_PIN)
- SSD1306 OLED -> I2C SDA=21, SCL=22 (Tenergy32Hub)

Build & Upload

Requirements:
- PlatformIO (recommended)
- ESP32 toolchain (PlatformIO will provide via platform)

Commands (from project root):

```bash
# build
pio run

# upload (select proper environment/port)
pio run --target upload
```

Runtime behavior

- GPS UART: Serial1 at 9600 baud (RX=27, TX=26)
- Parser: TinyGPS++ is the sole NMEA parser (added via lib_deps)
- Reader: `gpsTask` runs as a FreeRTOS task and feeds incoming UART bytes into TinyGPS++
- Display: OLED shows 4 lines:
  1. GPS status & satellite count
  2. Latitude (decimal degrees)
  3. Longitude (decimal degrees)
  4. Time/Date (HH:MM:SS DD/MM/YY) when available; otherwise signal quality or "Ready" message
- LEDs: Blue = GPS fixed, Red = searching/not ready
  - Blue LED behavior: blinks slowly (1s period: 0.5s on/off) when GPS fix is present, blinks fast (0.1s period: 0.05s on/off) while searching
  - Red LED behavior: lights steady when GPS interface is unavailable

Timeout / Diagnostics

- The firmware detects if no GPS data is received for 30 seconds (GPS_TIMEOUT_MS = 30000).
  - On first timeout transition the device will:
    - Show an OLED warning ("GPS Warning! No data for 30s")
    - Emit a single beep
    - Set an internal `gpsTimedOut` flag to avoid repeated notifications
  - When data returns TinyGPS++ updates clear the timeout and normal display resumes

Boot self-test

- At boot the firmware runs a quick self-test:
  - Initializes OLED (via `mcu.displayOLEDInfo()`)
  - Verifies GPS serial object is available
  - Shows result on OLED and produces beep patterns to indicate failures/success

Buttons

- SW1: Show detailed GPS info on Serial and beep
- SW2: Restart device

Notes & Next steps

- TinyGPS++ provides robust parsing and is now the canonical parser in this firmware.
- The previous custom NMEA parser was removed to simplify maintenance and rely on the battle-tested TinyGPS++ library.
- Possible future enhancements:
  - Show satellite SNR/PRN list if needed (TinyGPS++ exposes some satellite info)
  - Log events (timeout/restore) to SD card or remote server
  - Add smoothing/filtering of coordinates before display

License & Contact

See project files for license information. For questions contact: uten.boonliam@tenergyinnovation.co.th

Firmware

- FW_VERSION: 1.2 (TinyGPS++ parser, Thailand timezone UTC+7, LED behavior updated)
