#include <Arduino.h>
#include <PS2X_lib.h>

// Chân nối cục thu tay cầm PS2 trên Arduino Leonardo
#define PS2_DAT 12
#define PS2_CMD 11
#define PS2_SEL 10
#define PS2_CLK 13

PS2X ps2x;
int ps2Error = 1;

// Biến trạng thái gửi sang Uno R3
char huongDi = 'S';     // S: Dừng, F: Tiến, B: Lùi, L: Trái, R: Phải
int mucPWM = 200;       // Tốc độ mặc định (100 - 255)
int cheDoManHinh = 0;   // 0: Mặt Robot, 1: Dò Map Radar, 2: Info Xe
int bieuCam = 0;        // 0: Auto (Ngủ/Thức), 1: Vui, 2: Ngầu, 3: Ngạc nhiên

unsigned long lastSendTime = 0;

void ketNoiPS2() {
  ps2Error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, false, false);
}

void setup() {
  Serial.begin(9600);    // Debug qua USB máy tính
  Serial1.begin(9600);   // Giao tiếp UART phần cứng (Chân 0 RX1, Chân 1 TX1) sang Uno R3

  delay(500);
  ketNoiPS2();
}

void loop() {
  // Nếu chưa nhận tay cầm thì thử kết nối lại mỗi 1 giây
  if (ps2Error != 0) {
    ketNoiPS2();
    delay(1000);
    return;
  }

  ps2x.read_gamepad(false, 0);

  // 1. Đọc hướng di chuyển (4 nút D-Pad)
  if (ps2x.Button(PSB_PAD_UP)) {
    huongDi = 'F';
  } else if (ps2x.Button(PSB_PAD_DOWN)) {
    huongDi = 'B';
  } else if (ps2x.Button(PSB_PAD_LEFT)) {
    huongDi = 'L';
  } else if (ps2x.Button(PSB_PAD_RIGHT)) {
    huongDi = 'R';
  } else {
    huongDi = 'S';
  }

  // 2. Nút chỉnh PWM: R1 tăng tốc, L1 giảm tốc
  if (ps2x.ButtonPressed(PSB_R1)) {
    mucPWM += 25;
    if (mucPWM > 255) mucPWM = 255;
  }
  if (ps2x.ButtonPressed(PSB_L1)) {
    mucPWM -= 25;
    if (mucPWM < 100) mucPWM = 100;
  }

  // 3. Nút SELECT: Bật/tắt màn hình INFO XE (thông số & PWM hiện tại)
  if (ps2x.ButtonPressed(PSB_SELECT)) {
    if (cheDoManHinh == 2) {
      cheDoManHinh = 0; // Quay về màn hình mặt robot
    } else {
      cheDoManHinh = 2; // Mở trang Info xe
    }
  }

  // 4. Nút START: Bật/tắt chế độ Dò Map (Radar siêu âm)
  if (ps2x.ButtonPressed(PSB_START)) {
    if (cheDoManHinh == 1) {
      cheDoManHinh = 0; // Quay về màn hình mặt robot
    } else {
      cheDoManHinh = 1; // Mở trang Dò Map
    }
  }

  // 5. Nút TRÒN (CIRCLE): Chuyển đổi biểu cảm trên màn hình
  if (ps2x.ButtonPressed(PSB_CIRCLE)) {
    cheDoManHinh = 0; // Tự động chuyển về màn hình hiển thị mặt
    bieuCam = (bieuCam + 1) % 4;
  }

  // Gửi gói tin định dạng <HUONG,PWM,MODE,EXPR> sang Uno R3 mỗi 40ms
  if (millis() - lastSendTime >= 40) {
    lastSendTime = millis();
    Serial1.print('<');
    Serial1.print(huongDi);
    Serial1.print(',');
    Serial1.print(mucPWM);
    Serial1.print(',');
    Serial1.print(cheDoManHinh);
    Serial1.print(',');
    Serial1.print(bieuCam);
    Serial1.println('>');
  }

  delay(20);
}