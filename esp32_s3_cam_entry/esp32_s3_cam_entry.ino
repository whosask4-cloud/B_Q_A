#include "esp_camera.h"
#include <Adafruit_VL53L0X.h>
#include <ESP32Servo.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <Wire.h>

// ================= THAY ĐỔI THÔNG TIN MẠNG =================
const char *ssid = "H09";
const char *password = "hoilamgi";

// ================= THAY ĐỔI ĐỊA CHỈ IP MÁY CHỦ =================
// IP của Laptop đang chạy Python Server
const String serverName =
    "http://192.168.1.27:8000/upload/entry"; // <--- CỔNG VÀO

// ================= CẤU HÌNH CHÂN CHO ESP32-S3 (Freenove/Generic S3 CAM)
// =================
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5

#define Y9_GPIO_NUM 16
#define Y8_GPIO_NUM 17
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 12
#define Y5_GPIO_NUM 10
#define Y4_GPIO_NUM 8
#define Y3_GPIO_NUM 9
#define Y2_GPIO_NUM 11

#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

// ================= CẤU HÌNH CẢM BIẾN & SERVO =================
#define I2C_SDA 21
#define I2C_SCL 47
#define SERVO_PIN 2
#define BOOT_BUTTON_PIN 0 // Sử dụng nút BOOT để debug/chụp bằng tay

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
Servo gateServo;

// Góc xoay của Servo
const int ANGLE_CLOSED = 180; // Barie đóng
const int ANGLE_OPEN = 90;  // Barie mở

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Khởi tạo Servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  gateServo.setPeriodHertz(50);           // Standard 50hz servo
  gateServo.attach(SERVO_PIN, 500, 2400); // Attach servo to pin 2
  gateServo.write(ANGLE_CLOSED);          // Mặc định đóng

  // Khởi tạo I2C cho VL53L0X
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!lox.begin()) {
    Serial.println(F("CẢNH BÁO: Không tìm thấy cảm biến VL53L0X. Vui lòng kiểm "
                     "tra dây nối!"));
  } else {
    Serial.println(F("Khởi tạo VL53L0X thành công!"));
  }

  // Kết nối WiFi
  WiFi.begin(ssid, password);
  Serial.print("Đang kết nối WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi đã kết nối!");
  Serial.print("IP ESP32-S3: ");
  Serial.println(WiFi.localIP());

  // Khởi tạo Camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Khởi tạo camera thất bại: 0x%x\n", err);
    return;
  }
  Serial.println("Camera CỔNG VÀO đã sẵn sàng!");

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_brightness(s, 1);
    s->set_saturation(s, -2);
  }
  s->set_vflip(s, 1);
  s->set_hmirror(s, 0);
}

void openGate() {
  Serial.println("MỞ BARIE CHO XE VÀO!");
  gateServo.write(ANGLE_OPEN);

  // Mở trong 3 giây
  delay(3000);

  Serial.println("ĐÓNG BARIE LẠI!");
  gateServo.write(ANGLE_CLOSED);
}

void sendPhoto() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Lỗi chụp ảnh!");
    return;
  }

  Serial.println("Đang gửi ảnh CỔNG VÀO lên Server...");
  HTTPClient http;
  http.begin(serverName);

  String boundary = "----ESP32S3Boundary";
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"file\"; "
          "filename=\"esp32s3_capture_entry.jpg\"\r\n";
  head += "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t totalLen = head.length() + fb->len + tail.length();

  uint8_t *post_buffer = (uint8_t *)ps_malloc(totalLen);
  if (post_buffer == nullptr) {
    Serial.println("Lỗi: Không đủ bộ nhớ PSRAM!");
    http.end();
    esp_camera_fb_return(fb);
    return;
  }

  memcpy(post_buffer, head.c_str(), head.length());
  memcpy(post_buffer + head.length(), fb->buf, fb->len);
  memcpy(post_buffer + head.length() + fb->len, tail.c_str(), tail.length());

  int httpResponseCode = http.POST(post_buffer, totalLen);

  if (httpResponseCode > 0) {
    Serial.printf("Server Response: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println(response);

    // Kiểm tra xem phản hồi có chứa "success" hay không
    if (response.indexOf("\"status\":\"success\"") >= 0 ||
        response.indexOf("\"status\": \"success\"") >= 0) {
      Serial.println("=> Server nhận diện thành công!");
      openGate();
    } else {
      Serial.println("=> Không mở được barie (Không có xe, xe lỗi biển, hoặc "
                     "lỗi server).");
    }
  } else {
    Serial.printf("Lỗi kết nối Server: %s\n",
                  http.errorToString(httpResponseCode).c_str());
  }

  free(post_buffer);
  http.end();
  esp_camera_fb_return(fb);
}

void loop() {
  int boot_state = digitalRead(BOOT_BUTTON_PIN);
  bool object_detected = false;

  // Đọc dữ liệu từ cảm biến laser
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure,
                  false); // pass in 'true' to get debug data printout!

  // Kiểm tra nếu cảm biến hoạt động và có vật cản gần hơn 100mm
  if (measure.RangeStatus != 4) { // Nếu không bị out of range
    if (measure.RangeMilliMeter < 100) {
      object_detected = true;
    }
  }

  // Khi có vật cản (laser < 100mm) HOẶC bấm nút BOOT (LOW)
  if (object_detected || boot_state == LOW) {
    if (boot_state == LOW) {
      Serial.println("ĐÃ BẤM NÚT BOOT (DEBUG)! ĐỢI 1.5 GIÂY RỒI CHỤP ẢNH CỔNG VÀO...");
    } else {
      Serial.print("PHÁT HIỆN XE VÀO (Khoảng cách: ");
      Serial.print(measure.RangeMilliMeter);
      Serial.println("mm)! ĐỢI 1.5 GIÂY RỒI CHỤP ẢNH...");
    }

    delay(1500); // Đợi 1.5 giây
    sendPhoto();

    // Chờ người dùng thả nút ra (nếu bấm BOOT)
    while (digitalRead(BOOT_BUTTON_PIN) == LOW) {
      delay(10);
    }

    // Đợi 2 giây chống dội (tránh chụp liên tục)
    delay(2000);
  }
}
