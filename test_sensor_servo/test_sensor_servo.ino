#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// Cấu hình chân
#define SERVO_PIN 2
#define I2C_SDA 21
#define I2C_SCL 47

Servo gateServo;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// Các góc đóng/mở theo yêu cầu
const int ANGLE_CLOSED = 180;
const int ANGLE_OPEN = 90;

bool isGateOpen = false; // Theo dõi trạng thái barie

void setup() {
  Serial.begin(115200);

  // Khởi tạo PWM Timer cho ESP32-S3
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  gateServo.setPeriodHertz(50);
  gateServo.attach(SERVO_PIN, 500, 2400);
  
  // Khởi động đưa barie về góc ĐÓNG (180 độ)
  gateServo.write(ANGLE_CLOSED);
  
  // Khởi tạo cảm biến I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!lox.begin()) {
    Serial.println(F("LOI: Khong the khoi tao VL53L0X. Vui long kiem tra I2C!"));
    while(1);
  }

  Serial.println("=== TEST KET HOP CAM BIEN VA SERVO ===");
  Serial.println("Trang thai mac dinh: DONG (180 do)");
  Serial.println("Neu khoang cach < 100mm se MO (90 do)");
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);

  bool object_detected = false;

  // Kiểm tra nếu cảm biến đọc thành công và khoảng cách < 100mm
  if (measure.RangeStatus != 4) { 
    if (measure.RangeMilliMeter < 100) {
      object_detected = true;
    }
  }

  // Logic điều khiển Barie
  if (object_detected && !isGateOpen) {
    Serial.print("=> Phat hien xe (Khoang cach: ");
    Serial.print(measure.RangeMilliMeter);
    Serial.println("mm) -> MO BARIE (90 do)!");
    
    gateServo.write(ANGLE_OPEN);
    isGateOpen = true;
    
    // Đợi đúng 3 giây cho xe qua
    Serial.println("...Dang cho 3 giay...");
    delay(3000);
    
    // Đóng barie lại
    Serial.println("=> Het thoi gian -> DONG BARIE (180 do)!");
    gateServo.write(ANGLE_CLOSED);
    isGateOpen = false;
    
    // Nghỉ 1 giây trước khi nhận diện xe tiếp theo
    delay(1000);
  }
  
  delay(100); // Đợi 100ms trước khi đo vòng tiếp theo
}
