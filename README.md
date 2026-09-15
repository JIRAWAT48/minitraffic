# Smart Traffic & Pedestrian Control System (ESP32 + MQTT)

ระบบไฟจราจรอัจฉริยะและควบคุมทางข้ามคนเดินถนนด้วย ESP32 สามารถตรวจจับปริมาณการสัญจรของรถยนต์ ตรวจจับผู้ข้ามถนน และบันทึกการทำผิดกฎจราจร (ฝ่าสัญญาณไฟแดง / จอดรถทับทางม้าลาย) พร้อมแสดงผลผ่านจอ LCD และส่งข้อมูลสถิติไปยัง MQTT Broker แบบ Real-time

---

## 📌 ฟีเจอร์การทำงาน

* **Smart Pedestrian Crossing:** ควบคุมสัญญาณไฟจราจรและไฟคนข้ามอัตโนมัติเมื่อเซ็นเซอร์ตรวจพบคนรอข้ามถนน
* **Vehicle Counter:** นับจำนวนรถเข้า-ออก พร้อมคำนวณปริมาณรถทั้งหมดในระบบ
* **Violation Detection:** ตรวจจับและนับจำนวนการฝ่าสัญญาณไฟแดง และการจอดทับทางม้าลาย
* **LCD Display:** แสดงข้อความแจ้งเตือน "Reverse car!" บนหน้าจอ LCD I2C เมื่อพบรถจอดทับทางม้าลาย
* **MQTT Data Telemetry:** คำนวณและส่งข้อมูลสถิติจราจรไปยัง MQTT Broker ทุกๆ 5 วินาทีในรูปแบบ JSON Format

---

## 🛠️ อุปกรณ์ที่ใช้ (Hardware)

1. บอร์ด **ESP32**
2. เซ็นเซอร์อินฟราเรด (IR Obstacle Sensor) x 8 ตัว
3. หลอดไฟ LED สัญญาณจราจร (เขียว, เหลือง, แดง)
4. จอแสดงผล **LCD 16x2 พร้อม I2C Adapter (PCF8574)**
5. สายสัญญาณและวงจรต่อร่วม

---

## 🔌 การต่อสายและ Pinout Mapping

| อุปกรณ์ / หน้าที่ | อุปกรณ์ตัวที่ | GPIO Pin (ESP32) | ประเภท (Mode) |
| --- | --- | --- | --- |
| **IR Sensor** (คนข้ามถนน) | IR_SENSOR1 | 13 | INPUT |
|  | IR_SENSOR2 | 14 | INPUT |
| **IR Sensor** (นับจำนวนรถ) | IR_CAR_GO (รถไป) | 39 | INPUT |
|  | IR_CAR_BACK (รถกลับ) | 34 | INPUT |
| **IR Sensor** (ตรวจจับฝ่าไฟแดง) | IR_RED_LIGHT_1 | 18 | INPUT |
|  | IR_RED_LIGHT_2 | 19 | INPUT |
| **IR Sensor** (ตรวจจับทับทางม้าลาย) | IR_ZEBRA_1 | 16 | INPUT |
|  | IR_ZEBRA_2 | 17 | INPUT |
| **ไฟจราจรรถยนต์** | GREEN_CAR | 25 | OUTPUT |
|  | YELLOW_CAR | 26 | OUTPUT |
|  | RED_CAR | 27 | OUTPUT |
| **ไฟจราจรคนข้าม** | GREEN_PEDESTRIAN | 32 | OUTPUT |
|  | RED_PEDESTRIAN | 33 | OUTPUT |
| **LCD 16x2 (I2C)** | SDA | 21 (Default) | I2C Data |
|  | SCL | 22 (Default) | I2C Clock |

---

## 📚 ไลบรารีที่ต้องใช้ (Dependencies)

สามารถติดตั้งผ่าน **Arduino Library Manager** ได้ทันที:

1. **PubSubClient** (โดย Nick O'Leary) - สำหรับการเชื่อมต่อ MQTT
2. **LiquidCrystal_PCF8574** (โดย Matthias Hertel) - สำหรับควบคุมจอ LCD ผ่าน I2C

---

## 📡 โครงสร้าง MQTT Topics & Data Payload

ระบบใช้ Public Broker: `test.mosquitto.org:1883`

| Topic | ทิศทาง | รายละเอียด Data Payload (JSON) |
| --- | --- | --- |
| `Nine/traffic_data` | Publish | `{"data":{"totalCars": <จำนวนรถทั้งหมด>}}` |
| `Nine/traffic_violations` | Publish | `{"data":{"redLightViolations": <จำนวนครั้งที่ฝ่าไฟแดง>}}` |
| `Nine/red_light_violation_ratio` | Publish | `{"data":{"redLightViolationRatio": <เปอร์เซ็นต์การฝ่าไฟแดง>}}` |
| `Nine/led/status` | Subscribe | สำหรับรับคำสั่งควบคุมภายนอก |

---

## 🚀 การติดตั้งและใช้งาน

1. ติดตั้ง **Arduino IDE** และเพิ่มบอร์ด ESP32 ใน Board Manager
2. ติดตั้ง Library ตามรายการในหัวข้อ Dependencies
3. เปิดไฟล์ซอร์สโค้ด และแก้ไขข้อมูลการเชื่อมต่อ WiFi ในโค้ด:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

```


4. ตรวจสอบ I2C Address ของจอ LCD (ค่าเริ่มต้นในโค้ดคือ `0x27`)
5. เลือก Board เป็น **ESP32 Dev Module** และเลือก Port ให้ถูกต้อง
6. ทำการ **Compile & Upload** โค้ดลงบอร์ด ESP32
7. เปิด **Serial Monitor** ที่ Baud rate `115200` เพื่อดูสถานะการทำงาน
