#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// ================= THAY ĐỔI THÔNG TIN MẠNG =================
const char* ssid = "TÊN_WIFI_CỦA_BẠN";
const char* password = "MẬT_KHẨU_WIFI";

// ================= THAY ĐỔI ĐỊA CHỈ IP MÁY CHỦ =================
// IP của Laptop đang chạy Python Server
const String serverName = "http://192.168.1.100:8000/api/upload";

// ================= CẤU HÌNH CHÂN CHO ESP32-S3 (Freenove/Generic S3 CAM) =================
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

// ================= PIN CẢM BIẾN HỒNG NGOẠI =================
#define IR_SENSOR_PIN 2  // Chân cắm cảm biến hồng ngoại (Bạn có thể thay đổi tùy ý trên ESP32-S3)

void setup() {
  Serial.begin(115200);
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);

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
  
  // Quan trọng: ESP32-S3 N16R8 có 8MB PSRAM (Phải bật OPI PSRAM trong Tools)
  config.pixel_format = PIXFORMAT_JPEG;
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // Độ phân giải cao (1600x1200)
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    Serial.println("CẢNH BÁO: KHÔNG TÌM THẤY PSRAM! Ảnh sẽ bị mờ.");
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Khởi tạo camera thất bại: 0x%x\n", err);
    return;
  }
  Serial.println("Camera đã sẵn sàng!");
  
  // Tùy chỉnh chất lượng OV3660
  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // Đảo ngược ảnh nếu cần (1 hoặc 0)
    s->set_hmirror(s, 1);
  }
}

void sendPhoto() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Lỗi chụp ảnh!");
    return;
  }

  Serial.println("Đang gửi ảnh lên Server...");
  HTTPClient http;
  http.begin(serverName);
  
  // Tạo ranh giới cho form-data
  String boundary = "----ESP32S3Boundary";
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  // Tạo body yêu cầu
  String head = "--" + boundary + "\r\n";
  head += "Content-Disposition: form-data; name=\"file\"; filename=\"esp32s3_capture.jpg\"\r\n";
  head += "Content-Type: image/jpeg\r\n\r\n";
  String tail = "\r\n--" + boundary + "--\r\n";

  uint32_t totalLen = head.length() + fb->len + tail.length();

  // Mở stream để gửi dữ liệu
  http.addHeader("Content-Length", String(totalLen));
  
  // Bắt đầu gửi
  int httpResponseCode = http.POST((uint8_t *)head.c_str(), head.length()); // Ghi đè phương thức POST đơn giản
  
  // Vì HTTPClient không hỗ trợ trực tiếp stream body lớn một cách dễ dàng với multipart
  // Ta phải gửi toàn bộ buffer. Đoạn code dưới là cách tối ưu cho ESP32.
  WiFiClient *client = http.getStreamPtr();
  if(client) {
     client->write((const uint8_t *)head.c_str(), head.length());
     client->write(fb->buf, fb->len);
     client->write((const uint8_t *)tail.c_str(), tail.length());
  }

  httpResponseCode = http.GET(); // Nhận response
  
  if (httpResponseCode > 0) {
    Serial.printf("Server Response: %d\n", httpResponseCode);
    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.printf("Lỗi kết nối Server: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
  esp_camera_fb_return(fb);
}

void loop() {
  int ir_state = digitalRead(IR_SENSOR_PIN);
  
  // Khi có vật cản (xe đi vào), cảm biến hồng ngoại thường kéo xuống LOW (0)
  if (ir_state == LOW) {
    Serial.println("PHÁT HIỆN XE!");
    sendPhoto();
    
    // Đợi xe đi qua hẳn (tránh chụp liên tục)
    delay(5000); 
  }
}
