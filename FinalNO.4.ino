#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

// กำหนดขาเชื่อมต่อของเซ็นเซอร์และไฟจราจร
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

// ข้อมูล WiFi และ MQTT
const char* ssid = "Ji_0ny";
const char* password = "0918485958nn";
const char* mqtt_server = " test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_Client = "clientId-Bnarf9sOlR";
const char* mqtt_username = "";
const char* mqtt_password = "";

// ตั้งค่าการเชื่อมต่อ WiFi และ MQTT
WiFiClient espClient;
PubSubClient client(espClient);

int carGoCount = 0, carBackCount = 0, totalCars = 0;
int redLightViolations = 0, zebraCrossViolations = 0;
char msg[100];
String DataString;

// LCD object initialization (PCF8574 module)
LiquidCrystal_PCF8574 lcd(0x27); // Adjust the I2C address if needed

void setup() {
    Serial.begin(115200);

    int inputs[] = {IR_SENSOR1, IR_SENSOR2, IR_CAR_GO, IR_CAR_BACK, IR_RED_LIGHT_1, IR_RED_LIGHT_2, IR_ZEBRA_1, IR_ZEBRA_2};
    int outputs[] = {GREEN_CAR, YELLOW_CAR, RED_CAR, GREEN_PEDESTRIAN, RED_PEDESTRIAN};

    for (int pin : inputs) pinMode(pin, INPUT);
    for (int pin : outputs) pinMode(pin, OUTPUT);

    resetTrafficLights();
    
    // เชื่อมต่อ WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    // ตั้งค่า MQTT
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    reconnect();

    // Initialize LCD
    lcd.begin(16, 2); // Initialize a 16x2 LCD
    lcd.setBacklight(255);
    lcd.print("Traffic System");
    delay(2000);
    lcd.clear();
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    if (digitalRead(IR_CAR_GO) == LOW) updateCarCount(&carGoCount, "ไป");
    if (digitalRead(IR_CAR_BACK) == LOW) updateCarCount(&carBackCount, "กลับ");
    if (digitalRead(IR_SENSOR1) == LOW || digitalRead(IR_SENSOR2) == LOW) activatePedestrianCrossing();
    
    static long lastMsg = 0;
    long now = millis();
    if (now - lastMsg > 5000) {
        lastMsg = now;

        // คำนวณอัตราส่วนการฝ่าสัญญาณไฟแดง
        float redLightViolationRatio = (totalCars > 0) ? ((float)redLightViolations / totalCars) * 100 : 0.0;

        // ส่งข้อมูลจำนวนรถทั้งหมด
        DataString = "{\"data\":{\"totalCars\":" + String(totalCars) + "}}";
        DataString.toCharArray(msg, 100);
        Serial.println("Publishing MQTT message:");
        Serial.println(msg);
        client.publish("Nine/traffic_data", msg);

        // ส่งข้อมูลจำนวนการฝ่าสัญญาณไฟแดง
        DataString = "{\"data\":{\"redLightViolations\":" + String(redLightViolations) + "}}";
        DataString.toCharArray(msg, 100);
        Serial.println("Publishing MQTT message:");
        Serial.println(msg);
        client.publish("Nine/traffic_violations", msg);

        // ส่งอัตราส่วนของรถที่ฝ่าไฟแดง
        DataString = "{\"data\":{\"redLightViolationRatio\":" + String(redLightViolationRatio, 2) + "}}";
        DataString.toCharArray(msg, 100);
        Serial.println("Publishing MQTT message:");
        Serial.println(msg);
        client.publish("Nine/red_light_violation_ratio", msg);
    }

    delay(500);
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
    while (millis() - startTime < 5000) {
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

        // If zebra crossing violation, show message on LCD
        if (message == "รถจอดทับทางม้าลาย") {
            lcd.clear();
            lcd.print("Reverse car!");
            delay(2000);  // Display the message for 2 seconds
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

void reconnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
            Serial.println("connected");
            client.subscribe("Nine/led/status");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
}
