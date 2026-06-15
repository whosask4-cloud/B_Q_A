# 🚗 Hệ Thống Quản Lý Bãi Đỗ Xe Thông Minh Bằng AI (ESP32-CAM + YOLOv8)

Đây là tài liệu hướng dẫn cực kỳ chi tiết từ bước mua linh kiện, nối dây điện, cài đặt phần mềm cho đến khi đưa hệ thống vào chạy thực tế.

---

## 🛠 Phần 1: Chuẩn Bị Linh Kiện (Hardware)

Để mô hình bãi giữ xe có 2 cổng (1 cổng VÀO, 1 cổng RA) hoạt động độc lập, bạn cần mua đúng và đủ các linh kiện sau:

1. **Mạch xử lý hình ảnh (2 bộ):**
   - **Tên:** Mạch `ESP32-CAM` (Kèm sẵn Camera OV2640).
   - **Đế nạp:** **BẮT BUỘC** mua kèm đế nạp `ESP32-CAM-MB`. Đế nạp này giúp bạn cắm trực tiếp cáp MicroUSB vào máy tính để nạp code và cấp nguồn cực kỳ dễ dàng (không phải mua dây chuyển USB-to-TTL lằng nhằng).

2. **Cảm biến phát hiện xe (2 chiếc):**
   - **Tên:** Cảm biến khoảng cách Laser `VL53L0X`.
   - *Lý do chọn:* Đo khoảng cách bằng tia laser cực kỳ chính xác (tới từng mm), không bị nhiễu bởi ánh sáng mặt trời hay màu sắc của xe như các loại cảm biến hồng ngoại thông thường.

3. **Phụ kiện khác:**
   - **Dây nối (Jumper wires):** Mua 1 bó cáp cắm test board loại `Cái - Cái` (Female to Female) để cắm từ cảm biến vào ESP32.
   - **Nguồn cấp:** Cáp MicroUSB và 2 cục sạc điện thoại (5V-1A hoặc 5V-2A).

---

## 🔌 Phần 2: Sơ Đồ Cắm Dây (Wiring Diagram)

Mỗi mạch ESP32-CAM sẽ đi kèm với 1 cảm biến VL53L0X. Bạn dùng dây `Cái - Cái` cắm trực tiếp 4 chân của cảm biến vào các chân trần của ESP32-CAM theo đúng bảng sau:

| Chân trên VL53L0X | Chân trên ESP32-CAM | Chức năng (Ý nghĩa) |
| :---: | :---: | :--- |
| **VIN** (VCC) | **5V** (hoặc 3.3V) | Cấp nguồn điện cho cảm biến hoạt động. |
| **GND** | **GND** | Chân nối đất (âm). |
| **SCL** | **GPIO 14** | Chân xung nhịp (Clock) để truyền dữ liệu I2C. |
| **SDA** | **GPIO 13** | Chân dữ liệu (Data) để truyền khoảng cách về ESP32. |

> [!WARNING]
> Tuyệt đối không cắm ngược cực VIN và GND, nếu không cảm biến sẽ bị cháy ngay lập tức. Các chân còn lại trên VL53L0X (như XSHUT, GPIO1) thì bỏ trống.

---

## 💻 Phần 3: Cài Đặt Máy Chủ (Laptop/PC)

Máy tính của bạn sẽ đóng vai trò là "Bộ Não" (Server), chịu trách nhiệm nhận ảnh qua Wi-Fi và dùng AI (YOLOv8 + EasyOCR) để đọc biển số.

### Bước 1: Setup Môi Trường Server
1. Tải và cài đặt Python (phiên bản 3.10 trở lên). Nhớ tích vào ô `Add Python to PATH` khi cài đặt.
2. Mở thư mục dự án `c:\ARDUINO_ESP_PROJECT\DSD\`.
3. Mở **PowerShell**, gõ lệnh sau để tạo môi trường ảo và cài thư viện:
   ```powershell
   python -m venv .venv
   .\.venv\Scripts\activate
   pip install -r server\requirements.txt
   ```
4. Copy file model nhận diện AI là **`best.pt`** (tải từ Google Colab) dán vào bên trong thư mục `server\`.

### Bước 2: Chạy Server
- Nhấn đúp chuột vào file **`run_server.bat`** (ở ngoài cùng).
- Một cửa sổ đen hiện lên báo hiệu Server đã chạy ở địa chỉ `http://0.0.0.0:8000`.
- Mở trình duyệt Web (Chrome), truy cập vào đường dẫn: **http://localhost:8000** để xem Giao Diện Quản Lý Bãi Đỗ Xe.

---

## 📱 Phần 4: Chạy Thử Demo Bằng Điện Thoại (Không cần có ESP32)

Nếu bạn chưa mua linh kiện ESP32-CAM nhưng muốn Test xem mô hình AI nhận diện biển số hoạt động có tốt không, dự án này có tích hợp sẵn chức năng giả lập Camera bằng điện thoại di động!

1. **Khởi động Server** (Như Bước 2 của Phần 3).
2. Đảm bảo Laptop và Điện thoại của bạn đang **bắt chung 1 mạng Wi-Fi**.
3. Mở **CMD/PowerShell** trên Laptop, gõ lệnh `ipconfig` để lấy địa chỉ **IPv4** của Laptop (Ví dụ: `192.168.1.15`).
4. Mở Trình duyệt Web trên Điện thoại (Chrome/Safari), truy cập vào đường dẫn sau:
   👉 `http://ĐỊA_CHỈ_IP_CỦA_LAPTOP:8000/static/test.html`
5. Giao diện Test hiện ra. Bạn chỉ cần bấm "Chụp/Chọn ảnh biển số" rồi ấn gửi. Ảnh sẽ bay qua Wi-Fi về Laptop để AI phân tích và tự động hiển thị kết quả lên màn hình Dashboard ở Laptop ngay lập tức!

---

## ⚙️ Phần 5: Nạp Code Cho ESP32-CAM

### 1. Chuẩn bị Arduino IDE
1. Cài phần mềm **Arduino IDE** mới nhất.
2. Mở Arduino IDE, vào `File` -> `Preferences`. Chỗ **Additional Boards Manager URLs**, dán link này vào:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Vào `Tools` -> `Board` -> `Boards Manager`, tìm chữ **esp32** và ấn Install.
4. Tải thư viện cảm biến: Vào `Sketch` -> `Include Library` -> `Manage Libraries`, tìm **Adafruit VL53L0X** và cài đặt.

### 2. Cấu hình Board Mạch Cực Kỳ Quan Trọng
Gắn ESP32-CAM vào đế MB, cắm cáp USB vào máy tính. Trong thẻ `Tools` của Arduino IDE, bạn **BẮT BUỘC** phải chỉnh đúng các thông số sau:
- **Board:** `AI Thinker ESP32-CAM` (Hoặc `ESP32 Wrover Module`).
- **Partition Scheme:** Chọn `Huge APP (3MB No OTA/1MB SPIFFS)` (Nếu không chọn cái này, code camera sẽ báo lỗi dung lượng).
- **PSRAM:** `Enabled` (Bắt buộc để Camera có RAM xử lý ảnh).
- **Port:** Chọn cổng COM tương ứng của mạch.

### 3. Sửa Code và Nạp
Mở file `esp32_cam\esp32_cam.ino`. Bạn tìm và sửa 3 dòng đầu tiên:
```cpp
const char* ssid = "Tên_WiFi_Nhà_Bạn";
const char* password = "Mật_khẩu_WiFi";

// Thay dòng dưới bằng địa chỉ IPv4 của Laptop bạn (Mở CMD gõ ipconfig để xem)
String serverName = "http://192.168.1.15:8000/upload/entry"; 
```
> [!NOTE]
> Khi nạp code cho cái mạch ESP32-CAM thứ hai (đặt ở cổng ra), bạn nhớ đổi chữ `entry` ở cuối link thành `exit` (vd: `http://192.168.1.15:8000/upload/exit`).

Bấm nút **Upload (Mũi tên ngang)** trên Arduino IDE và chờ chạy đến 100%.

---

## 🚀 Phần 5: Cách Hoạt Động Của Hệ Thống

1. Bật nguồn cho 2 mạch ESP32-CAM, bật file `run_server.bat` trên Laptop.
2. **Xe Đi Vào:** Khi xe quét ngang qua cảm biến Laser ở Cổng Vào (cách dưới 50cm), mạch lập tức chụp ảnh biển số.
3. ESP32-CAM gửi tấm ảnh đó qua Wi-Fi đến Laptop.
4. YOLOv8 trên Laptop tìm thấy biển số, cắt khung hình, đưa cho EasyOCR đọc chữ và số.
5. Biển số được lưu vào Cơ sở dữ liệu và hiện lên Giao diện Web ở cột **"Đang Đỗ"**.
6. **Xe Đi Ra:** Quá trình tương tự diễn ra ở Cổng Ra. AI nhận diện, so khớp biển số, gạch tên khỏi danh sách Đang đỗ và tính thời gian xe đã gửi.

**CHÚC BẠN HOÀN THÀNH XUẤT SẮC ĐỒ ÁN NÀY! 🎉**
