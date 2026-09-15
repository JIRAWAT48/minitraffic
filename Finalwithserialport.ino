#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

// กำหนดขาเซ็นเซอร์และไฟ
#define IR_SENSOR1 13
#define IR_SENSOR2 14
#define IR_CAR_GO 39
#define IR_CAR_BACK 34
#define IR_RED_LIGHT_1 18
#define IR_RED_LIGHT_2 19
#define IR_ZEBRA_1 16
#define IR_ZEBRA_2 17
#define GREEN_CAR 25
#define YELLOW_CAR 26
#define RED_CAR 27
#define GREEN_PEDESTRIAN 32
#define RED_PEDESTRIAN 33

// ตัวแปรนับจำนวน
int carGoCount = 0, carBackCount = 0, totalCars = 0;
int redLightViolations = 0, zebraCrossViolations = 0;

LiquidCrystal_PCF8574 lcd(0x27);

void setup() {
    Serial.begin(115200);

    int inputs[] = {IR_SENSOR1, IR_SENSOR2, IR_CAR_GO, IR_CAR_BACK, IR_RED_LIGHT_1, IR_RED_LIGHT_2, IR_ZEBRA_1, IR_ZEBRA_2};
    int outputs[] = {GREEN_CAR, YELLOW_CAR, RED_CAR, GREEN_PEDESTRIAN, RED_PEDESTRIAN};

    for (int pin : inputs) pinMode(pin, INPUT);
    for (int pin : outputs) pinMode(pin, OUTPUT);

    resetTrafficLights();

    lcd.begin(16, 2);
    lcd.setBacklight(255);
    lcd.print("Traffic System");
    delay(2000);
    lcd.clear();

    Serial.println("{\"status\":\"ระบบเริ่มทำงาน\"}");
}

void loop() {
    if (digitalRead(IR_CAR_GO) == LOW) updateCarCount(&carGoCount, "ไป");
    if (digitalRead(IR_CAR_BACK) == LOW) updateCarCount(&carBackCount, "กลับ");
    if (digitalRead(IR_SENSOR1) == LOW || digitalRead(IR_SENSOR2) == LOW) activatePedestrianCrossing();
    delay(500);

    // ส่งข้อมูลสรุปทุกๆ 5 วินาที
    static unsigned long lastReport = 0;
    if (millis() - lastReport >= 5000) {
        sendSummary();
        lastReport = millis();
    }
}

void updateCarCount(int* count, String direction) {
    (*count)++;
    totalCars = carGoCount + carBackCount;

    Serial.printf("จำนวนรถที่วิ่ง%s: %d\n", direction.c_str(), *count);
    Serial.printf("จำนวนรถทั้งหมด: %d\n", totalCars);
    delay(500);
}

void activatePedestrianCrossing() {
    changeTrafficLight(GREEN_CAR, LOW, YELLOW_CAR, HIGH);
    delay(4000);
    changeTrafficLight(YELLOW_CAR, LOW, RED_CAR, HIGH);
    changeTrafficLight(RED_PEDESTRIAN, LOW, GREEN_PEDESTRIAN, HIGH);
    Serial.println("ไฟคนข้ามเป็นสีเขียว - ห้ามรถผ่าน!");

    unsigned long startTime = millis();
    while (millis() - startTime < 10000) {
        checkViolations(IR_RED_LIGHT_1, IR_RED_LIGHT_2, &redLightViolations, "รถฝ่าไฟแดง");
        checkViolations(IR_ZEBRA_1, IR_ZEBRA_2, &zebraCrossViolations, "รถจอดทับทางม้าลาย");
    }

    changeTrafficLight(GREEN_PEDESTRIAN, LOW, RED_PEDESTRIAN, HIGH);
    delay(500);
    resetTrafficLights();
}

void checkViolations(int sensor1, int sensor2, int* count, String message) {
    if (digitalRead(sensor1) == LOW || digitalRead(sensor2) == LOW) {
        (*count)++;
        Serial.printf("** %s! จำนวน: %d\n", message.c_str(), *count);
        delay(500);

        if (message == "รถจอดทับทางม้าลาย") {
            lcd.clear();
            lcd.print("Reverse car!");
            delay(2000);
            lcd.clear();
        }
    }
}

void resetTrafficLights() {
    digitalWrite(GREEN_CAR, HIGH);
    digitalWrite(YELLOW_CAR, LOW);
    digitalWrite(RED_CAR, LOW);
    digitalWrite(GREEN_PEDESTRIAN, LOW);
    digitalWrite(RED_PEDESTRIAN, HIGH);
}

void changeTrafficLight(int off1, int state1, int on2, int state2) {
    digitalWrite(off1, state1);
    digitalWrite(on2, state2);
}

void sendSummary() {
    float ratio = 0;
    if (totalCars > 0) {
        ratio = ((float)redLightViolations / totalCars) * 100.0;
    }

    String jsonData = String("{\"summary\":{") +
                      "\"totalCars\":" + totalCars + "," +
                      "\"redLightViolations\":" + redLightViolations + "," +
                      "\"violationRatio\":" + String(ratio, 2) +
                      "}}";
    Serial.println(jsonData);
}
