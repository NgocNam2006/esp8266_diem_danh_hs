# Hệ Thống Điểm Danh Học Sinh Bằng RFID

Dự án sử dụng **ESP8266 + RC522 + Buzzer 3V**, kết nối Wi-Fi để điểm danh học sinh và lưu dữ liệu thời gian thực lên **Google Sheets** thông qua **Google Apps Script**.

## 1. Phần Cứng

- **ESP8266 (NodeMCU)**
- **RC522 RFID Reader**
- **Buzzer 3V**
- **Thẻ RFID (UID dạng HEX)**
- Dây kết nối MicroUSB
- Nguồn 5V

### Sơ đồ nối RC522 + Buzzer với ESP8266

| Thiết bị   | Chân        | ESP8266 Pin     |
|------------|-------------|-----------------|
| **RC522**  | SDA         | D8 (GPIO15)     |
|            | SCK         | D5 (GPIO14)     |
|            | MOSI        | D7 (GPIO13)     |
|            | MISO        | D6 (GPIO12)     |
|            | RST         | D1 (GPIO5)      |
|            | GND         | GND             |
|            | 3.3V        | 3.3V            |
| **Buzzer** | Chân +      | D2 (GPIO4)      |
|            | Chân –      | GND             |

---

## 2. Chức Năng

- Đọc thẻ RFID, trích xuất UID
- Gửi UID lên Google Script để kiểm tra học sinh (GET)
- Nhận thông tin học sinh (tên, lớp), và thời gian
- Gửi dữ liệu điểm danh lên Google Sheets (POST)
- Báo hiệu bằng buzzer:
  - Bíp ngắn: UID hợp lệ
  - Bíp dài: UID không tồn tại

---

## 3. Phần Mềm

### a. Arduino Code

- Viết bằng Arduino IDE
- Sử dụng các thư viện:
  - `ESP8266WiFi.h`
  - `MFRC522.h`
  - `ArduinoJson.h`
  - `ESP8266HTTPClient.h`
- Gửi và nhận dữ liệu với Google Apps Script qua Wi-Fi

### b. Google Apps Script

**Sheet "DanhSach"** chứa danh sách học sinh:

| RFID       | Họ và tên     | Lớp   |
|------------|----------------|--------|
| 04A1BC23   | Nguyễn Văn A   | 10A1  |

**Sheet "DiemDanh"** để lưu lịch sử điểm danh:

| RFID       | Họ và tên     | Lớp   | Thời gian điểm danh         |
|------------|----------------|--------|------------------------------|
| 04A1BC23   | Nguyễn Văn A   | 10A1  | 2025-04-19 08:15:23         |

---

## 4. Hướng Dẫn Sử Dụng

### Bước 1: Flash code Arduino lên ESP8266  
Thay `NAME_WIFI`, `PASS_WIFI` và `LINK_APP_SCRIPT` trong code bằng thông tin thật.

### Bước 2: Tạo Google Apps Script

1. Vào Google Sheets > Tiện ích > Trình chỉnh sửa Apps Script
2. Dán file `app_script.gs` vào
3. Triển khai > Triển khai dưới dạng ứng dụng web
4. Cho phép mọi người truy cập và copy link vào biến `LINK_APP_SCRIPT`

---
## 5. Liên Hệ

- **Email**: [hnn.inf.77@gmail.com](mailto:hnn.inf.77@gmail.com)
- **GitHub**: [Ngocnamm2006](https://github.com/ngocnamm06)
- **Facebook**: [Hồ Ngọc Nam](https://facebook.com/namdz.pro.2006)
- **Discord**: [HuYuNan](https://discord.gg/QMfA6kVY)
--- 
