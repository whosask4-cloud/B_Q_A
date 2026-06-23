#include <ESP32Servo.h>

#define SERVO_PIN 5
#define BOOT_BUTTON_PIN 0

Servo gateServo;

// Danh sách các góc xoay theo yêu cầu: 0 -> 270 -> 0
int angles[] = {180, 90};
int currentAngleIndex = 0;

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

  // Khởi tạo PWM Timer cho ESP32-S3 (bắt buộc đối với thư viện ESP32Servo)
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  gateServo.setPeriodHertz(50); // Servo tiêu chuẩn 50Hz
  gateServo.attach(SERVO_PIN, 500,
                   2400); // Gắn Servo vào chân số 2 với xung 500-2400us

  // Đưa servo về góc 0 mặc định ban đầu
  gateServo.write(angles[currentAngleIndex]);

  Serial.println("=== TEST SERVO VOI NUT BOOT (ESP32-S3) ===");
  Serial.println("Bam nut BOOT de chuyen goc: 0 -> 270 -> 0...");
  Serial.print("Goc hien tai dang la: ");
  Serial.println(angles[currentAngleIndex]);
}

void loop() {
  // Nút BOOT khi bấm sẽ ở mức LOW
  if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
    // Chờ 50ms chống dội nút (debounce)
    delay(50);

    // Chắc chắn là nút vẫn đang được bấm
    if (digitalRead(BOOT_BUTTON_PIN) == LOW) {

      // Chuyển sang góc tiếp theo trong mảng
      currentAngleIndex++;
      if (currentAngleIndex > 1) {
        currentAngleIndex = 0; // Quay vòng lại 0 nếu đã qua 270
      }

      int targetAngle = angles[currentAngleIndex];
      gateServo.write(targetAngle); // Ra lệnh quay Servo

      Serial.print("=> Da chuyen Servo den goc: ");
      Serial.println(targetAngle);

      // Chờ cho đến khi người dùng nhả nút BOOT ra thì mới tiếp tục
      while (digitalRead(BOOT_BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  }
}
