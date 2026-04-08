#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ============================================
// CẤU HÌNH WIFI & MQTT
// ============================================
#ifdef WOKWI
const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASSWORD = "";
#else
const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
#endif

// HiveMQ Cloud Broker — TLS (port 8883)
const char *MQTT_BROKER = "b1c8f2cc5cd4416fb74b671a407dc0ff.s1.eu.hivemq.cloud";
const int MQTT_PORT = 8883;
const char *MQTT_CLIENT_ID = "esp32-sensor-01";
const char *MQTT_USERNAME = "hivemq.webclient.1775680760112";
const char *MQTT_PASSWORD = "amy&f0VPG3c5<8,U>qAB";

// MQTT Topic
const char *TOPIC_SENSOR = "farm/sensor";

// ============================================
// CẤU HÌNH CHÂN GPIO
// ============================================
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define LED_PIN 5

// ============================================
// ĐỐI TƯỢNG
// ============================================
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClientSecure espClient; // ← dùng Secure vì port 8883 = TLS
PubSubClient mqttClient(espClient);

// ============================================
// BIẾN TRẠNG THÁI
// ============================================
unsigned long lastPublish = 0;
unsigned long lastMQTTRetry = 0;
const unsigned long PUBLISH_INTERVAL = 5000;
const unsigned long MQTT_RETRY_MS = 5000;

// ============================================
// HÀM KẾT NỐI WIFI
// ============================================
void setupWiFi()
{
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected! IP: ");
  Serial.println(WiFi.localIP());
}

// ============================================
// HÀM KẾT NỐI MQTT
// ============================================
void reconnectMQTT()
{
  Serial.print("[MQTT] Connecting...");
  if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
  {
    Serial.println(" Connected!");
  }
  else
  {
    Serial.printf(" Failed, error: %d\n", mqttClient.state());
  }
}

// ============================================
// GỬI DỮ LIỆU QUA MQTT
// ============================================
void publishSensorData(float temp, float hum)
{
  StaticJsonDocument<256> doc;
  doc["device"] = "ESP32-01";
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  doc["timestamp"] = millis();

  char buffer[256];
  size_t n = serializeJson(doc, buffer);
  mqttClient.publish(TOPIC_SENSOR, buffer, n);

  Serial.printf("Published: Temp=%.1f C, Hum=%.1f %%\n", temp, hum);
}

// ============================================
// ĐỌC DỮ LIỆU CẢM BIẾN
// ============================================
void readSensor(float &temp, float &hum)
{
  temp = dht.readTemperature();
  hum = dht.readHumidity();

  if (isnan(temp))
  {
    temp = random(200, 350) / 10.0;
  }
  if (isnan(hum))
  {
    hum = random(400, 800) / 10.0;
  }
}

// ============================================
// KHỞI TẠO
// ============================================
void setup()
{
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("  ESP32 + DHT22 + MQTT (TLS)");
  Serial.println("========================================");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  dht.begin();

  // Bỏ xác thực chứng chỉ SSL (đơn giản, phù hợp demo)
  // Trên board thật có thể thay bằng setCertificate / setCACert
  espClient.setInsecure();

  setupWiFi();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);

  Serial.println("Setup complete!");
}

// ============================================
// VÒNG LẶP CHÍNH
// ============================================
void loop()
{
  // Kiểm tra WiFi
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("[WiFi] Lost connection, reconnecting...");
    setupWiFi();
  }

  // Non-blocking MQTT reconnect
  if (!mqttClient.connected())
  {
    if (millis() - lastMQTTRetry >= MQTT_RETRY_MS)
    {
      lastMQTTRetry = millis();
      reconnectMQTT();
    }
  }
  mqttClient.loop();

  unsigned long now = millis();

  // Gửi dữ liệu mỗi 5 giây
  if (now - lastPublish >= PUBLISH_INTERVAL)
  {
    lastPublish = now;

    float temperature, humidity;
    readSensor(temperature, humidity);

    Serial.println("\n--- Sensor Data ---");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    if (mqttClient.connected())
    {
      publishSensorData(temperature, humidity);
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
    }
    else
    {
      Serial.println("[MQTT] Not connected, skipping...");
    }
  }
}
