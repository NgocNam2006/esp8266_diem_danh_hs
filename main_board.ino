#include <ESP8266WiFi.h>
#include <MFRC522.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x3F, 16, 2);

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

  digitalWrite(BUZZER_PIN, HIGH);
  delay(user.name == "Unknown" ? 500 : 300);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("===== ✅ Đã quét =====");
  Serial.print("⏰ Thời gian: "); Serial.println(currentTime);
  Serial.print("🆔 UID: "); Serial.println(rfid);
  Serial.print("👤 Họ tên: "); Serial.println(user.name);
  Serial.print("🏫 Lớp: "); Serial.println(user.className);
  Serial.println("=====================");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(user.name.length() > 16 ? user.name.substring(0, 16) : user.name);
  lcd.setCursor(0, 1);
  String line2 = user.className + " " + currentTime;
  lcd.print(line2.length() > 16 ? line2.substring(0, 16) : line2);

  delay(3000);
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
  } else if (httpResponseCode == HTTP_CODE_MOVED_PERMANENTLY || httpResponseCode == HTTP_CODE_FOUND) {
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

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("📡 Server phản hồi: " + response);
  } else {
    Serial.print("❌ Gửi lỗi: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}
