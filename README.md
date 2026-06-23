# 🚗 Hệ Thống Quản Lý Bãi Đỗ Xe Thông Minh Bằng AI (ESP32-S3 + YOLOv8)

Đây là tài liệu hướng dẫn cực kỳ chi tiết từ bước mua linh kiện, nối dây điện, cài đặt phần mềm cho đến khi đưa hệ thống vào chạy thực tế.

---

## 🛠 Phần 1: Chuẩn Bị Linh Kiện (Hardware)

Để mô hình bãi giữ xe có 2 cổng (1 cổng VÀO, 1 cổng RA) hoạt động độc lập, bạn cần mua đúng và đủ các linh kiện sau:

1. **Mạch xử lý hình ảnh (2 bộ):**
   - **Tên:** Mạch `ESP32-S3` (Kèm sẵn Camera OV3660).
   - **Cáp kết nối:** Cáp USB Type-C (hoặc MicroUSB tùy loại mạch) để cắm trực tiếp từ mạch S3 vào máy tính để nạp code và cấp nguồn. Mạch ESP32-S3 hiện đại đã tích hợp sẵn mạch nạp nên không cần mua thêm đế nạp rời.

2. **Cảm biến phát hiện xe (2 chiếc):**
   - **Tên:** Cảm biến khoảng cách Laser `VL53L0X`.
   - *Lý do chọn:* Đo khoảng cách bằng tia laser cực kỳ chính xác (tới từng mm), không bị nhiễu bởi ánh sáng mặt trời hay màu sắc của xe như các loại cảm biến hồng ngoại thông thường.

3. **Phụ kiện khác:**
   - **Dây nối (Jumper wires):** Mua 1 bó cáp cắm test board loại `Cái - Cái` (Female to Female) để cắm từ cảm biến vào ESP32.
   - **Nguồn cấp:** Cáp MicroUSB và 2 cục sạc điện thoại (5V-1A hoặc 5V-2A).

---

## 🔌 Phần 2: Sơ Đồ Cắm Dây (Wiring Diagram)

Mỗi mạch ESP32-S3 sẽ đi kèm với 1 cảm biến VL53L0X. Bạn dùng dây `Cái - Cái` cắm trực tiếp 4 chân của cảm biến vào các chân trần của ESP32-S3 theo đúng bảng sau:

| Chân Linh Kiện | Chân trên ESP32-S3 | Chức năng (Ý nghĩa) |
| :---: | :---: | :--- |
| **VL53L0X: VIN** | **5V** (hoặc 3.3V) | Cấp nguồn điện cho cảm biến hoạt động. |
| **VL53L0X: GND** | **GND** | Chân nối đất (âm). |
| **VL53L0X: SCL** | **GPIO 47** | Chân xung nhịp (Clock) để truyền dữ liệu I2C. |
| **VL53L0X: SDA** | **GPIO 21** | Chân dữ liệu (Data) để truyền khoảng cách về ESP32. |
| **Servo: Đỏ** | **Nguồn 5V Rời** | Nối vào cực Dương (5V) của **NGUỒN RỜI** (Ví dụ: hộp pin 4xAA hoặc module nguồn). |
| **Servo: Đen/Nâu**| **GND Nguồn Rời + ESP32** | Nối vào cực Âm của nguồn rời, **VÀ PHẢI NỐI CHUNG** với chân **GND** của ESP32. |
| **Servo: Vàng/Cam**| **GPIO 2** | Chân tín hiệu nhận lệnh mở/đóng Barie. |

> [!WARNING]
> **LƯU Ý CỰC KỲ QUAN TRỌNG:**
> 1. Tuyệt đối không cắm ngược cực VIN và GND của cảm biến VL53L0X, nếu không sẽ cháy ngay lập tức.
> 2. Khi dùng nguồn riêng cho Servo, **BẮT BUỘC PHẢI NỐI CHUNG MẠCH ĐẤT (GND)** giữa nguồn rời và mạch ESP32-S3. Nếu không nối chung GND, xung tín hiệu sẽ không có điện áp tham chiếu, dẫn đến Servo giật lag, chạy loạn xạ hoặc không chạy.

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

Nếu bạn chưa mua linh kiện ESP32-S3 nhưng muốn Test xem mô hình AI nhận diện biển số hoạt động có tốt không, dự án này có tích hợp sẵn chức năng giả lập Camera bằng điện thoại di động!

1. **Khởi động Server** (Như Bước 2 của Phần 3).
2. Đảm bảo Laptop và Điện thoại của bạn đang **bắt chung 1 mạng Wi-Fi**.
3. Mở **CMD/PowerShell** trên Laptop, gõ lệnh `ipconfig` để lấy địa chỉ **IPv4** của Laptop (Ví dụ: `192.168.1.15`).
4. Mở Trình duyệt Web trên Điện thoại (Chrome/Safari), truy cập vào đường dẫn sau:
   👉 `http://ĐỊA_CHỈ_IP_CỦA_LAPTOP:8000/static/test.html`
5. Giao diện Test hiện ra. Bạn chỉ cần bấm "Chụp/Chọn ảnh biển số" rồi ấn gửi. Ảnh sẽ bay qua Wi-Fi về Laptop để AI phân tích và tự động hiển thị kết quả lên màn hình Dashboard ở Laptop ngay lập tức!

---

## ⚙️ Phần 5: Nạp Code Cho ESP32-S3

### 1. Chuẩn bị Arduino IDE
1. Cài phần mềm **Arduino IDE** mới nhất.
2. Mở Arduino IDE, vào `File` -> `Preferences`. Chỗ **Additional Boards Manager URLs**, dán link này vào:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Vào `Tools` -> `Board` -> `Boards Manager`, tìm chữ **esp32** và ấn Install.
4. Tải thư viện cảm biến: Vào `Sketch` -> `Include Library` -> `Manage Libraries`, tìm **Adafruit VL53L0X** và cài đặt.

### 2. Cấu hình Board Mạch Cực Kỳ Quan Trọng
Cắm cáp USB nối mạch ESP32-S3 trực tiếp vào máy tính. Trong thẻ `Tools` của Arduino IDE, bạn **BẮT BUỘC** phải chỉnh đúng các thông số sau (dành riêng cho dòng S3 N16R8):
- **Board:** `ESP32S3 Dev Module` (Hoặc `Freenove ESP32-S3 WROOM`).
- **Flash Size:** `16MB (128Mb)`.
- **Partition Scheme:** Chọn `16MB (3MB APP/9.9MB FATFS)` hoặc `Huge APP`.
- **PSRAM:** Chọn `OPI PSRAM` (Cực kỳ quan trọng để cấp 8MB RAM cho Camera OV3660).
- **Port:** Chọn cổng COM tương ứng của mạch.

### 3. Cách Tìm IP Máy Tính Và Gắn Vào Code
Máy tính và mạch ESP32 phải bắt **cùng 1 mạng WiFi**. Để 2 thiết bị kết nối được với nhau, bạn phải lấy địa chỉ IP của máy tính và khai báo vào code ESP32.

**Hướng dẫn lấy IP:**
1. Bấm phím `Windows` trên bàn phím, gõ chữ `cmd` và ấn Enter để mở Command Prompt.
2. Trong màn hình đen CMD, gõ lệnh `ipconfig` và ấn Enter.
3. Tìm đến dòng **IPv4 Address** (thường nằm dưới mục *Wireless LAN adapter Wi-Fi*). Dãy số bên phải chính là IP của Laptop bạn (Ví dụ: `192.168.1.19`).

**Gắn IP vào Code Arduino:**
Mở file `esp32_s3_cam_entry\esp32_s3_cam_entry.ino`. Bạn tìm và sửa 3 dòng đầu tiên:
```cpp
const char* ssid = "Tên_WiFi_Nhà_Bạn";
const char* password = "Mật_khẩu_WiFi";

// Thay IP ở dòng dưới bằng địa chỉ IPv4 bạn vừa lấy được ở bước trên
const String serverName = "http://192.168.1.19:8000/upload/entry"; 
```
> [!NOTE]
> Khi nạp code cho cái mạch thứ hai (đặt ở Cổng Ra), bạn hãy mở file `esp32_s3_cam_exit.ino` lên và nạp nhé. (Link API upload ảnh bên trong file đó đã được đổi sẵn thành `exit`).

> [!WARNING]
> **LƯU Ý VỀ ĐỊA CHỈ IP MÁY CHỦ:**
> Nếu bạn khởi động lại mạng hoặc dùng **điện thoại phát WiFi**, địa chỉ IP của Laptop CÓ THỂ BỊ THAY ĐỔI. Nếu IP bị đổi, ESP32 sẽ không kết nối được. Hãy chạy lại lệnh `ipconfig` trên máy tính để lấy IP mới và cập nhật lại vào code Arduino. Khuyên dùng chức năng Set IP tĩnh trên Windows nếu làm việc cố định!

Bấm nút **Upload (Mũi tên ngang)** trên Arduino IDE và chờ chạy đến 100%.

---

## 🚀 Phần 5: Cách Hoạt Động Của Hệ Thống

1. Bật nguồn cho 2 mạch ESP32-S3, bật file `run_server.bat` trên Laptop.
2. **Xe Đi Vào:** Khi xe quét ngang qua cảm biến Laser ở Cổng Vào (cách dưới 10cm - 100mm), mạch lập tức mở Barie và chụp ảnh biển số.
3. ESP32-S3 gửi tấm ảnh đó qua Wi-Fi đến Laptop.
4. YOLOv8 trên Laptop tìm thấy biển số, cắt khung hình, đưa cho EasyOCR đọc chữ và số.
5. Biển số được lưu vào Cơ sở dữ liệu và hiện lên Giao diện Web ở cột **"Đang Đỗ"**.
6. **Xe Đi Ra:** Quá trình tương tự diễn ra ở Cổng Ra. AI nhận diện, so khớp biển số, gạch tên khỏi danh sách Đang đỗ và tính thời gian xe đã gửi.
7. **Khôi Phục Dữ Liệu:** Trong quá trình test, nếu bạn muốn xóa sạch toàn bộ danh sách xe và thư mục ảnh rác để làm lại từ đầu, chỉ cần bấm nút **"⚠️ Reset Dữ Liệu"** màu đỏ ngay góc trên cùng của Giao diện Web. Hệ thống sẽ tự dọn dẹp sạch sẽ trong 1 giây!

**CHÚC BẠN HOÀN THÀNH XUẤT SẮC ĐỒ ÁN NÀY! 🎉**
