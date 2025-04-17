#include <ESP8266WiFi.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// Wi-Fi cấu hình
const char* ssid = "LINK_WIFI";        
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

  // Lấy tên người dùng từ RFID từ Google Sheets
  String name = getNameFromRFID(rfid);

  // Lấy thời gian hiện tại từ server
  String currentTime = getCurrentTime();

  // Gửi dữ liệu vào Google Sheets (RFID và tên người dùng)
  sendToGoogleSheets(rfid, name);

  // Phản hồi với buzzer tùy thuộc vào tên người dùng
  if (name == "Unknown") {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // In thông tin quét lên Serial log
  Serial.println("====== Quét thành công ======");
  Serial.print("Thời gian: ");
  Serial.println(currentTime);  // In thời gian hiện tại
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

  // URL yêu cầu đến Google Apps Script, truyền tham số rfid
  String url = String(serverName) + "?rfid=" + rfid;
  http.begin(client, url);

  int httpResponseCode = http.GET();
  String payload = "Unknown";

  // Nếu thành công, nhận tên từ payload
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

// Hàm lấy thời gian hiện tại từ server
String getCurrentTime() {
  WiFiClient client;
  HTTPClient http;

  // URL yêu cầu thời gian từ Google Apps Script
  String url = String(serverName) + "?action=getTime";
  http.begin(client, url);

  int httpResponseCode = http.GET();
  String payload = "Unknown";

  // Nếu thành công, nhận thời gian từ payload
  if (httpResponseCode == 200) {
    payload = http.getString();
    payload.trim();  // Xóa khoảng trắng dư nếu có
  } else {
    Serial.print("❌ Lỗi GET thời gian từ Sheets: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return payload;
}

// Gửi dữ liệu điểm danh đến Google Sheets (POST)
void sendToGoogleSheets(String rfid, String name) {
  WiFiClient client;
  HTTPClient http;

  // Tạo JSON để gửi lên server
  StaticJsonDocument<256> doc;
  doc["rfid"] = rfid;
  doc["name"] = name;

  String jsonString;
  serializeJson(doc, jsonString);

  // Gửi dữ liệu lên server
  http.begin(client, serverName);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsonString);

  // Kiểm tra phản hồi từ server
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("📡 Phản hồi server: " + response);
  } else {
    Serial.print("❌ Lỗi gửi dữ liệu: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
