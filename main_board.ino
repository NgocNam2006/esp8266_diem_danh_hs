#include <ESP8266WiFi.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "NAME_WIFI";        
const char* password = "PASS_WIFI";
const char* serverName = "LINK_APP_SCRIPT";

#define SS_PIN 15
#define RST_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

#define BUZZER_PIN 4

struct UserInfo {
  String name;
  String className;
};

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối đến Wi-Fi...");
  }
  Serial.println("✅ Kết nối Wi-Fi thành công!");

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("✅ RC522 đã khởi tạo!");
}

void ensureWiFiConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🔄 Wi-Fi bị ngắt. Đang kết nối lại...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Đã kết nối lại Wi-Fi!");
    } else {
      Serial.println("\n❌ Không thể kết nối lại Wi-Fi!");
    }
  }
}

void loop() {
  ensureWiFiConnected();

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String rfid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    rfid += String(mfrc522.uid.uidByte[i], HEX);
  }
  rfid.toUpperCase();

  UserInfo user = getUserInfoFromRFID(rfid);
  String currentTime = getCurrentTime();
  sendToGoogleSheets(rfid, user.name, user.className);

  if (user.name == "Unknown") {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
  }

  Serial.println("====== Quét thành công ======");
  Serial.print("Thời gian: ");
  Serial.println(currentTime);
  Serial.print("UID: ");
  Serial.println(rfid);
  Serial.print("Họ và tên: ");
  Serial.println(user.name);
  Serial.print("Lớp: ");
  Serial.println(user.className);
  Serial.println("=============================");

  delay(3000);
}

UserInfo getUserInfoFromRFID(String rfid) {
  WiFiClient client;
  HTTPClient http;

  String url = String(serverName) + "?rfid=" + rfid;
  http.begin(client, url);

  int httpResponseCode = http.GET();
  UserInfo info;
  info.name = "Unknown";
  info.className = "";

  if (httpResponseCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      info.name = doc["name"] | "Unknown";
      info.className = doc["class"] | "";
    }
  } else {
    Serial.print("❌ Lỗi GET thông tin từ Sheets: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return info;
}

String getCurrentTime() {
  WiFiClient client;
  HTTPClient http;

  String url = String(serverName) + "?action=getTime";
  http.begin(client, url);

  int httpResponseCode = http.GET();
  String payload = "Unknown";

  if (httpResponseCode == 200) {
    payload = http.getString();
    payload.trim();
  } else {
    Serial.print("❌ Lỗi GET thời gian từ Sheets: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return payload;
}

void sendToGoogleSheets(String rfid, String name, String className) {
  WiFiClient client;
  HTTPClient http;

  StaticJsonDocument<256> doc;
  doc["rfid"] = rfid;
  doc["name"] = name;
  doc["class"] = className;

  String jsonString;
  serializeJson(doc, jsonString);

  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsonString);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("📡 Phản hồi server: " + response);
  } else {
    Serial.print("❌ Lỗi gửi dữ liệu: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
