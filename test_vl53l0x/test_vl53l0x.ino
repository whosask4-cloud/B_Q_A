#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// Cấu hình chân I2C cho ESP32-S3 (SDA: 21, SCL: 47)
#define I2C_SDA 21
#define I2C_SCL 47

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

void setup() {
  Serial.begin(115200);

  // Đợi cho Serial khởi động xong
  while (!Serial) {
    delay(1);
  }
  
  Serial.println("=== BAT DAU TEST CAM BIEN GY-530 VL53L0X ===");

  // Khởi tạo giao tiếp I2C với chân SDA và SCL được cấu hình
  Wire.begin(I2C_SDA, I2C_SCL);
  
  // Khởi tạo cảm biến VL53L0X
  if (!lox.begin()) {
    Serial.println(F("LOI: Khong the khoi tao VL53L0X. Vui long kiem tra lai day noi (VCC, GND, SDA, SCL)!"));
    while(1);
  }
  
  Serial.println(F("VL53L0X da san sang!\n\n")); 
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;
    
  Serial.print("Dang do khoang cach... ");
  
  // Thực hiện đo khoảng cách
  lox.rangingTest(&measure, false); // Để `true` nếu muốn in log debug của thư viện

  if (measure.RangeStatus != 4) {  // RangeStatus = 4 là lỗi out of range (ngoài tầm đo)
    float distance_cm = measure.RangeMilliMeter / 10.0;
    
    Serial.print("Khoang cach do duoc (cm): "); 
    Serial.print(distance_cm);
    
    if (distance_cm <= 2.0) {
      Serial.println(" => PHAT HIEN VAT CAN TRONG TAM 2CM!");
    } else {
      Serial.println();
    }
  } else {
    Serial.println("Ngoai tam do (Out of range) hoac khong co vat can");
  }
    
  delay(200); // Chờ 200ms trước khi đo lại
}
