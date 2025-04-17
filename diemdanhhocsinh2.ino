#include <ESP8266WiFi.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// Wi-Fi cấu hình
const char* ssid = "NAME_WIFI";        
const char* password = "PASS_WIFI";

// Google Apps Script URL
const char* serverName = "LINK_APP_SCRIPT";

// Cấu hình chân RC522
#define SS_PIN 15  // D8
#define RST_PIN 5  // D1
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Chân Buzzer
#define BUZZER_PIN 4  // D2

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

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Lấy mã UID
  String rfid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    rfid += String(mfrc522.uid.uidByte[i], HEX);
  }
  rfid.toUpperCase();

  String name = getNameFromRFID(rfid);  // Lấy tên từ Google Sheets

  sendToGoogleSheets(rfid, name);  // Gửi dữ liệu (server sẽ tự thêm thời gian)

  // Buzzer phản hồi
  if (name == "Unknown") {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Serial log
  Serial.println("====== Quét thành công ======");
  Serial.print("UID: ");
  Serial.println(rfid);
  Serial.print("Họ và tên: ");
  Serial.println(name);
  Serial.println("=============================");

  delay(3000);
}

// Hàm lấy tên từ Google Sheets qua Apps Script
String getNameFromRFID(String rfid) {
  WiFiClient client;
  HTTPClient http;

  String url = String(serverName) + "?rfid=" + rfid;
  http.begin(client, url);

  int httpResponseCode = http.GET();
  String payload = "Unknown";

  if (httpResponseCode == 200) {
    payload = http.getString();
    payload.trim();  // Xóa khoảng trắng dư nếu có
  } else {
    Serial.print("❌ Lỗi GET tên từ Sheets: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return payload;
}

// Gửi dữ liệu điểm danh đến Google Sheets (POST) – KHÔNG GỬI timestamp
void sendToGoogleSheets(String rfid, String name) {
  WiFiClient client;
  HTTPClient http;

  StaticJsonDocument<256> doc;
  doc["rfid"] = rfid;
  doc["name"] = name;

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
