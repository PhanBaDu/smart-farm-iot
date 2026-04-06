#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// ============================================
// CẤU HÌNH WIFI & MQTT
// ============================================
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// HiveMQ Cloud Broker (Miễn phí, không cần đăng ký)
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "smartfarm-esp32-001";

// MQTT Topics
const char* TOPIC_ENV = "iot/farm/env";      // Nhiệt độ, độ ẩm không khí
const char* TOPIC_SOIL = "iot/farm/soil";     // Độ ẩm đất
const char* TOPIC_LIGHT = "iot/farm/light";   // Ánh sáng
const char* TOPIC_CONTROL = "iot/farm/control"; // Lệnh điều khiển
const char* TOPIC_ALERT = "iot/farm/alert";   // Cảnh báo
const char* TOPIC_STATUS = "iot/farm/status"; // Trạng thái thiết bị

// ============================================
// CẤU HÌNH CHÂN GPIO
// ============================================
#define DHT_PIN 23
#define DHT_TYPE DHT22

#define SOIL_PIN 34
#define LDR_PIN 35

#define PUMP_PIN 26
#define FAN_PIN 27
#define BUZZER_PIN 25

// OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

// ============================================
// CẤU HÌNH NGƯỠNG (THRESHOLD)
// ============================================
#define TEMP_THRESHOLD_HIGH 35.0
#define TEMP_THRESHOLD_LOW 18.0
#define SOIL_THRESHOLD_LOW 30.0
#define SOIL_THRESHOLD_HIGH 70.0
#define LIGHT_THRESHOLD_LOW 20.0

// ============================================
// ĐỐI TƯỢNG
// ============================================
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ============================================
// BIẾN TRẠNG THÁI
// ============================================
bool pumpState = false;
bool fanState = false;
bool autoMode = true;
bool alertActive = false;
unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 5000; // 5 giây

// ============================================
// HÀM KẾT NỐI WIFI
// ============================================
void setupWiFi() {
  Serial.print("Đang kết nối WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi đã kết nối!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nKết nối WiFi thất bại!");
  }
}

// ============================================
// HÀM KẾT NỐI MQTT
// ============================================
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Đang kết nối MQTT...");
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Serial.println("MQTT đã kết nối!");
      mqttClient.subscribe(TOPIC_CONTROL);
      Serial.print("Đã subscribe: ");
      Serial.println(TOPIC_CONTROL);
    } else {
      Serial.print("Thất bại, mã lỗi: ");
      Serial.println(mqttClient.state());
      delay(3000);
    }
  }
}

// ============================================
// CALLBACK XỬ LÝ LỆNH MQTT
// ============================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nhận lệnh từ topic: ");
  Serial.println(topic);

  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.print("Nội dung: ");
  Serial.println(message);

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (!error) {
    // Xử lý lệnh điều khiển
    if (doc.containsKey("mode")) {
      autoMode = doc["mode"].as<String>() == "auto";
      Serial.print("Chế độ: ");
      Serial.println(autoMode ? "AUTO" : "MANUAL");
    }

    if (doc.containsKey("pump") && !autoMode) {
      pumpState = doc["pump"].as<bool>();
      digitalWrite(PUMP_PIN, pumpState ? HIGH : LOW);
      Serial.print("Bơm: ");
      Serial.println(pumpState ? "BẬT" : "TẮT");
    }

    if (doc.containsKey("fan") && !autoMode) {
      fanState = doc["fan"].as<bool>();
      digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
      Serial.print("Quạt: ");
      Serial.println(fanState ? "BẬT" : "TẮT");
    }

    if (doc.containsKey("buzzer")) {
      bool buzzerCmd = doc["buzzer"].as<bool>();
      digitalWrite(BUZZER_PIN, buzzerCmd ? HIGH : LOW);
    }
  }
}

// ============================================
// ĐỌC DỮ LIỆU CẢM BIẾN
// ============================================
float readTemperature() {
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    temp = random(200, 350) / 10.0;
  }
  return temp;
}

float readHumidity() {
  float hum = dht.readHumidity();
  if (isnan(hum)) {
    hum = random(400, 800) / 10.0;
  }
  return hum;
}

int readSoilMoisture() {
  int raw = analogRead(SOIL_PIN);
  int soil = map(raw, 0, 4095, 100, 0);
  return constrain(soil, 0, 100);
}

int readLight() {
  int raw = analogRead(LDR_PIN);
  int light = map(raw, 0, 4095, 0, 100);
  return constrain(light, 0, 100);
}

// ============================================
// HIỂN THỊ OLED
// ============================================
void updateOLED(float temp, float hum, int soil, int light) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART FARM IoT");

  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 16);
  display.print("Temp: ");
  display.print(temp, 1);
  display.println(" C");

  display.setCursor(0, 26);
  display.print("Hum:  ");
  display.print(hum, 1);
  display.println(" %");

  display.setCursor(0, 36);
  display.print("Soil: ");
  display.print(soil);
  display.println(" %");

  display.setCursor(0, 46);
  display.print("Light:");
  display.print(light);
  display.println(" %");

  display.setCursor(0, 56);
  display.print("Pump:");
  display.print(pumpState ? "ON " : "OFF");
  display.print(" Fan:");
  display.print(fanState ? "ON" : "OFF");

  display.display();
}

// ============================================
// GỬI DỮ LIỆU QUA MQTT
// ============================================
void publishSensorData(float temp, float hum, int soil, int light) {
  StaticJsonDocument<256> doc;

  doc["device"] = "ESP32-001";
  doc["timestamp"] = millis();

  // Topic: iot/farm/env (nhiệt độ, độ ẩm không khí)
  doc["temperature"] = temp;
  doc["humidity"] = hum;

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_ENV, buffer, n);

  // Topic: iot/farm/soil (độ ẩm đất)
  StaticJsonDocument<128> soilDoc;
  soilDoc["moisture"] = soil;
  soilDoc["timestamp"] = millis();
  n = serializeJson(soilDoc, buffer);
  mqttClient.publish(TOPIC_SOIL, buffer, n);

  // Topic: iot/farm/light (ánh sáng)
  StaticJsonDocument<128> lightDoc;
  lightDoc["intensity"] = light;
  lightDoc["timestamp"] = millis();
  n = serializeJson(lightDoc, buffer);
  mqttClient.publish(TOPIC_LIGHT, buffer, n);

  Serial.println("Đã gửi dữ liệu lên MQTT");
}

// ============================================
// XỬ LÝ ĐIỀU KHIỂN TỰ ĐỘNG
// ============================================
void autoControl(float temp, float hum, int soil, int light) {
  if (!autoMode) return;

  // Điều khiển bơm theo độ ẩm đất
  if (soil < SOIL_THRESHOLD_LOW && !pumpState) {
    pumpState = true;
    digitalWrite(PUMP_PIN, HIGH);
    Serial.println("[AUTO] Bật bơm - Đất khô");
  } else if (soil > SOIL_THRESHOLD_HIGH && pumpState) {
    pumpState = false;
    digitalWrite(PUMP_PIN, LOW);
    Serial.println("[AUTO] Tắt bơm - Đất đủ ẩm");
  }

  // Điều khiển quạt theo nhiệt độ
  if (temp > TEMP_THRESHOLD_HIGH && !fanState) {
    fanState = true;
    digitalWrite(FAN_PIN, HIGH);
    Serial.println("[AUTO] Bật quạt - Nhiệt độ cao");
  } else if (temp < TEMP_THRESHOLD_LOW && fanState) {
    fanState = false;
    digitalWrite(FAN_PIN, LOW);
    Serial.println("[AUTO] Tắt quạt - Nhiệt độ bình thường");
  }

  // Cảnh báo
  if (temp > TEMP_THRESHOLD_HIGH || soil < SOIL_THRESHOLD_LOW || light < LIGHT_THRESHOLD_LOW) {
    if (!alertActive) {
      alertActive = true;
      digitalWrite(BUZZER_PIN, HIGH);
      sendAlert(temp, soil, light);
    }
  } else {
    alertActive = false;
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ============================================
// GỬI CẢNH BÁO
// ============================================
void sendAlert(float temp, int soil, int light) {
  StaticJsonDocument<256> doc;
  doc["type"] = "alert";
  doc["device"] = "ESP32-001";
  doc["timestamp"] = millis();

  JsonArray alerts = doc.createNestedArray("reasons");

  if (temp > TEMP_THRESHOLD_HIGH) {
    alerts.add("Nhiet do qua cao");
  }
  if (soil < SOIL_THRESHOLD_LOW) {
    alerts.add("Do am dat thap");
  }
  if (light < LIGHT_THRESHOLD_LOW) {
    alerts.add("Anh sang thap");
  }

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_ALERT, buffer, n);
  Serial.println("Đã gửi cảnh báo!");
}

// ============================================
// GỬI TRẠNG THÁI THIẾT BỊ
// ============================================
void publishStatus() {
  StaticJsonDocument<256> doc;
  doc["device"] = "ESP32-001";
  doc["pump"] = pumpState;
  doc["fan"] = fanState;
  doc["buzzer"] = alertActive;
  doc["mode"] = autoMode ? "auto" : "manual";
  doc["rssi"] = WiFi.RSSI();

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_STATUS, buffer, n);
}

// ============================================
// KHỞI TẠO
// ============================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("  SMART FARM IoT - ESP32 MQTT");
  Serial.println("========================================");

  // Khởi tạo chân GPIO
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SOIL_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);

  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Khởi tạo DHT22
  dht.begin();

  // Khởi tạo OLED
  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED khong khoi dong duoc!");
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 25);
    display.println("SMART FARM");
    display.display();
    delay(1500);
  }

  // Kết nối WiFi
  setupWiFi();

  // Cấu hình MQTT
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);

  Serial.println("Setup hoan tat!");
}

// ============================================
// VÒNG LẶP CHÍNH
// ============================================
void loop() {
  // Kiểm tra kết nối
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  unsigned long now = millis();

  // Đọc và xử lý dữ liệu theo interval
  if (now - lastPublish >= PUBLISH_INTERVAL) {
    lastPublish = now;

    float temperature = readTemperature();
    float humidity = readHumidity();
    int soilMoisture = readSoilMoisture();
    int lightIntensity = readLight();

    Serial.println("\n--- Du lieu cam bien ---");
    Serial.print("Nhiet do: "); Serial.print(temperature); Serial.println(" C");
    Serial.print("Do am khong khi: "); Serial.print(humidity); Serial.println(" %");
    Serial.print("Do am dat: "); Serial.print(soilMoisture); Serial.println(" %");
    Serial.print("Anh sang: "); Serial.print(lightIntensity); Serial.println(" %");

    // Gửi dữ liệu lên MQTT
    publishSensorData(temperature, humidity, soilMoisture, lightIntensity);

    // Cập nhật OLED
    updateOLED(temperature, humidity, soilMoisture, lightIntensity);

    // Điều khiển tự động
    autoControl(temperature, humidity, soilMoisture, lightIntensity);
  }

  // Gửi trạng thái thiết bị mỗi 10 giây
  static unsigned long lastStatus = 0;
  if (now - lastStatus >= 10000) {
    lastStatus = now;
    publishStatus();
  }
}
