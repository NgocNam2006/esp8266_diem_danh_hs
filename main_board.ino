#include <ESP8266WiFi.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Wi-Fi
const char* ssid = "NAME_WIFI";
const char* password = "PASS_WIFI";

// Google Apps Script URL
const char* serverName = "LINK_APP_SCRIPT";

// RC522
#define SS_PIN 15  // D8
#define RST_PIN 5  // D1
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Buzzer
#define BUZZER_PIN 14 // D5

// Chống quét liên tục
String lastRFID = "";
unsigned long lastScanTime = 0;
const unsigned long debounceDelay = 5000; // 5 giây

struct UserInfo {
  String name;
  String className;
};

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Diem danh RFID");
  delay(2000);
  lcd.clear();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Đang kết nối Wi-Fi...");
  }
  Serial.println("✅ Wi-Fi OK");

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("✅ RC522 OK");
}

void ensureWiFiConnected() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("🔄 Wi-Fi ngắt. Kết nối lại...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
      delay(500);
      Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) Serial.println("\n✅ Wi-Fi đã kết nối lại");
    else Serial.println("\n❌ Không thể kết nối Wi-Fi");
  }
}

void loop() {
  ensureWiFiConnected();

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  String rfid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    rfid += String(mfrc522.uid.uidByte[i], HEX);
  }
  rfid.toUpperCase();

  // 🚫 Chống quét liên tục
  unsigned long currentTimeMs = millis();
  if (rfid == lastRFID && currentTimeMs - lastScanTime < debounceDelay) {
    Serial.println("⚠️ Bỏ qua: trùng UID và thời gian chưa đủ delay");
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }
  lastRFID = rfid;
  lastScanTime = currentTimeMs;

  UserInfo user = getUserInfoFromRFID(rfid);
  String currentTime = getCurrentTime();
  sendToGoogleSheets(rfid, user.name, user.className);

  if (user.name != "Unknown") {
    tone(BUZZER_PIN, 1000);
    delay(300);
    noTone(BUZZER_PIN);
  }

  Serial.println("===== ✅ Đã quét =====");
  Serial.println("⏰ Thời gian: " + currentTime);
  Serial.println("🆔 UID: " + rfid);
  Serial.println("👤 Họ tên: " + user.name);
  Serial.println("🏫 Lớp: " + user.className);
  Serial.println("=====================");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(fitLCD(user.name));
  lcd.setCursor(0, 1);
  lcd.print(fitLCD(user.className + " " + currentTime));

  delay(1000);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  unsigned long startWait = millis();
  Serial.println("⏳ Chờ rút thẻ...");
  while ((mfrc522.PICC_IsNewCardPresent() || mfrc522.PICC_ReadCardSerial()) && millis() - startWait < 5000) {
    delay(200);
  }

  forceResetRC522(); // Reset mạnh
}

void forceResetRC522() {
  Serial.println("🔄 Force reset RC522...");
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  for (int i = 0; i < 3; i++) {
    mfrc522.PCD_Reset();
    delay(100);
  }
  mfrc522.PCD_Init();
  delay(100);
  Serial.println("✅ RC522 đã reset hoàn toàn");
}

UserInfo getUserInfoFromRFID(String rfid) {
  WiFiClientSecure client;
  client.setInsecure();
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
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      info.name = doc["name"] | "Unknown";
      info.className = doc["class"] | "";
    }
  } 
  //Nếu mạch esp của bạn ổn định thì hãy comment phần này để không tốn thời gian chuyển hướng
  else if (httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY || httpResponseCode == HTTP_CODE_FOUND) {
    String redirectUrl = http.getLocation();
    http.end();
    http.begin(client, redirectUrl);
    httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String payload = http.getString();
      StaticJsonDocument<256> doc;
      if (deserializeJson(doc, payload) == DeserializationError::Ok) {
        info.name = doc["name"] | "Unknown";
        info.className = doc["class"] | "";
      }
    }
  }

  http.end();
  return info;
}

String getCurrentTime() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = String(serverName) + "?action=getTime";
  http.begin(client, url);
  int httpResponseCode = http.GET();
  String payload = "Unknown";

  if (httpResponseCode == 200) {
    payload = http.getString();
    payload.trim();
  } 
  else if (httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY || httpResponseCode == HTTP_CODE_FOUND) {
    String redirectUrl = http.getLocation();
    http.end();
    http.begin(client, redirectUrl);
    httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      payload = http.getString();
      payload.trim();
    }
  }

  http.end();
  return payload;
}

void sendToGoogleSheets(String rfid, String name, String className) {
  WiFiClientSecure client;
  client.setInsecure();
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

  // phần này báo phản hồi và lỗi của server về serial monitor nếu không cần thì hãy xóa
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("📡 Server phản hồi: " + response);
  } else {
    Serial.print("❌ Gửi lỗi: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

String fitLCD(String text) {
  return text.length() > 16 ? text.substring(0, 16) : text;
}
