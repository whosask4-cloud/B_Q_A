#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"

// ================= THÔNG SỐ CÀI ĐẶT =================
const char* ssid = "YOUR_WIFI_SSID";           // Tên Wi-Fi
const char* password = "YOUR_WIFI_PASSWORD";   // Mật khẩu Wi-Fi

// ĐỊA CHỈ IP CỦA LAPTOP (ví dụ: 192.168.1.100)
// Sửa thành IP thật của máy bạn và giữ nguyên port 8000
String serverName = "http://192.168.1.100:8000/upload/entry"; 
// Nếu nạp cho mạch ở cổng Ra, đổi chữ "entry" thành "exit":
// String serverName = "http://192.168.1.100:8000/upload/exit";

// Các chân I2C cắm cảm biến VL53L0X
#define I2C_SDA 13
#define I2C_SCL 14
// ====================================================

// Cấu hình chân cho ESP32-CAM (AI-Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// Trạng thái phát hiện xe
bool vehicleDetected = false;
unsigned long lastCaptureTime = 0;
const unsigned long captureDelay = 5000; // Đợi 5s mới cho chụp xe tiếp theo

void setup() {
  Serial.begin(115200);
  Serial.println();

  // Khởi tạo I2C cho cảm biến
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!lox.begin()) {
    Serial.println("Khong tim thay VL53L0X, kiem tra lai day noi!");
    while(1);
  }
  Serial.println("VL53L0X OK!");

  // Cấu hình Camera
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // UXGA (1600x1200) để ảnh đủ rõ cho OCR
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; 
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Khởi động camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera OK!");

  // Kết nối Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Dang ket noi WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Da ket noi!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void sendImageToServer() {
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    http.begin(serverName);

    // Boundary ngẫu nhiên cho multipart/form-data
    String boundary = "----ESP32CamBoundary";
    String contentType = "multipart/form-data; boundary=" + boundary;
    http.addHeader("Content-Type", contentType);

    // Payload HTTP
    String head = "--" + boundary + "\r\n" +
                  "Content-Disposition: form-data; name=\"file\"; filename=\"image.jpg\"\r\n" +
                  "Content-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    http.addHeader("Content-Length", String(totalLen));

    // Gửi dữ liệu
    WiFiClient *client = http.getStreamPtr();
    
    // Nếu dùng phiên bản ESP32 Arduino Core cũ, cần code send trực tiếp bằng WiFiClient
    // Ở đây ta dùng http.sendRequest với stream buffer nếu thư viện hỗ trợ, 
    // Nhưng cách dễ nhất là kết nối thủ công bằng client:
    
    http.begin(serverName);
    int httpResponseCode = http.sendRequest("POST", fb->buf, fb->len); // Fallback đơn giản: gửi raw binary
    
    /* CHÚ Ý: HTTPClient mặc định không hỗ trợ stream dễ dàng cho multipart.
       Để đơn giản, trong project này Python Server nhận UploadFile. 
       Do vậy, ta viết lại hàm gửi POST HTTP thô (Raw HTTP POST) */
    
    esp_camera_fb_return(fb); // Giải phóng buffer
    fb = esp_camera_fb_get(); // Chụp lại cho chắc

    String serverDomain = serverName; 
    serverDomain.replace("http://", "");
    int portIndex = serverDomain.indexOf(":");
    int slashIndex = serverDomain.indexOf("/");
    String host = serverDomain.substring(0, portIndex);
    int port = serverDomain.substring(portIndex+1, slashIndex).toInt();
    String path = serverDomain.substring(slashIndex);

    WiFiClient tcpClient;
    if (tcpClient.connect(host.c_str(), port)) {
      tcpClient.println("POST " + path + " HTTP/1.1");
      tcpClient.println("Host: " + host);
      tcpClient.println("Content-Length: " + String(totalLen));
      tcpClient.println("Content-Type: multipart/form-data; boundary=" + boundary);
      tcpClient.println();
      
      tcpClient.print(head);
      
      // Gửi ảnh chia thành các gói nhỏ
      uint8_t *fbBuf = fb->buf;
      size_t fbLen = fb->len;
      for (size_t n = 0; n < fbLen; n = n + 1024) {
        if (n + 1024 < fbLen) {
          tcpClient.write(fbBuf, 1024);
          fbBuf += 1024;
        } else if (fbLen % 1024 > 0) {
          size_t remainder = fbLen % 1024;
          tcpClient.write(fbBuf, remainder);
        }
      }  
      
      tcpClient.print(tail);
      
      long timeout = millis();
      while (!tcpClient.available() && millis() - timeout < 5000);
      
      // Đọc response
      while (tcpClient.available()) {
        String line = tcpClient.readStringUntil('\n');
        Serial.println(line);
      }
      tcpClient.stop();
      Serial.println("Gui anh thanh cong!");
    } else {
      Serial.println("Khong ket noi duoc toi Laptop (Server)");
    }
  }
  
  esp_camera_fb_return(fb);
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  if (measure.RangeStatus != 4) {  // 4 = out of phase (không thấy gì)
    int distance = measure.RangeMilliMeter; // Khoảng cách (mm)
    
    // Nếu khoảng cách nhỏ hơn 500mm (50cm) -> Có xe
    if (distance > 0 && distance < 500) {
      if (millis() - lastCaptureTime > captureDelay) {
        Serial.println("Phat hien xe! Dang chup anh...");
        sendImageToServer();
        lastCaptureTime = millis();
      }
    }
  }
  
  delay(100);
}
