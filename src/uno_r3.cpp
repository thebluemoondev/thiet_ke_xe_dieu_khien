#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdlib.h>
#include <string.h>

// Chỉnh chiều màn hình OLED: 0 = chiều chuẩn, 2 = xoay ngược 180 độ
#define XOAY_MAN_HINH 2

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Chân cảm biến siêu âm HC-SR04
#define TRIG_PIN 2
#define ECHO_PIN 3

// Chân điều khiển cầu H L298N
const int ENA = 5;
const int IN1 = 7;
const int IN2 = 8;
const int IN3 = 9;
const int IN4 = 10;
const int ENB = 6;

// Biến trạng thái nhận từ Leonardo qua UART
char huongDi = 'S';
int mucPWM = 200;
int cheDoManHinh = 0; // 0: Mặt Robot, 1: Dò Map Radar, 2: Info Xe
int bieuCam = 0;      // 0: Auto (Ngủ/Thức), 1: Vui, 2: Ngầu, 3: Ngạc nhiên

unsigned long lastUartTime = 0;
bool uartKetNoi = false;

// Biến cho cảm biến siêu âm & Radar
int khoangCach = 999;
unsigned long lastPing = 0;
const int RADAR_CX = 43;
const int RADAR_CY = 62;
const int RADAR_R  = 52;
int gocQuet = 35;
int chieuQuet = 4;

// Bộ đệm nhận chuỗi UART <F,200,0,0>
char rxBuffer[24];
uint8_t rxIndex = 0;
bool dangNhanGoi = false;

// ================= ĐIỀU KHIỂN ĐỘNG CƠ =================
void dungXe() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void chayTien(int pwm) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, pwm);   analogWrite(ENB, pwm);
}

void chayLui(int pwm) {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, pwm);   analogWrite(ENB, pwm);
}

void reTrai(int pwm) {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, pwm);   analogWrite(ENB, pwm);
}

void rePhai(int pwm) {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, pwm);   analogWrite(ENB, pwm);
}

void capNhatDongCo() {
  // Tự động dừng an toàn nếu mất tín hiệu UART quá 500ms
  if (millis() - lastUartTime > 500) {
    uartKetNoi = false;
    huongDi = 'S';
    dungXe();
    return;
  }

  switch (huongDi) {
    case 'F': chayTien(mucPWM); break;
    case 'B': chayLui(mucPWM);  break;
    case 'L': reTrai(mucPWM);   break;
    case 'R': rePhai(mucPWM);   break;
    default:  dungXe();          break;
  }
}

// ================= ĐỌC & GIẢI MÃ UART =================
void giaiMaGoiTin(char *data) {
  char *token = strtok(data, ",");
  if (token != NULL) huongDi = token[0];

  token = strtok(NULL, ",");
  if (token != NULL) mucPWM = constrain(atoi(token), 0, 255);

  token = strtok(NULL, ",");
  if (token != NULL) cheDoManHinh = atoi(token);

  token = strtok(NULL, ",");
  if (token != NULL) bieuCam = atoi(token);

  lastUartTime = millis();
  uartKetNoi = true;
}

void docUART() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '<') {
      dangNhanGoi = true;
      rxIndex = 0;
    } else if (c == '>' && dangNhanGoi) {
      rxBuffer[rxIndex] = '\0';
      giaiMaGoiTin(rxBuffer);
      dangNhanGoi = false;
    } else if (dangNhanGoi && rxIndex < sizeof(rxBuffer) - 1) {
      rxBuffer[rxIndex++] = c;
    }
  }
}

// ================= ĐO SIÊU ÂM =================
int docSieuAm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 18000);
  if (duration == 0) return 999;
  int cm = duration * 0.034 / 2;
  return (cm < 2) ? 2 : cm;
}

// ================= GIAO DIỆN 1: KHUÔN MẶT & BIỂU CẢM ĐỘNG =================
void veGiaoDienMat() {
  display.clearDisplay();
  unsigned long now = millis();

  int matHienTai = bieuCam;
  bool dangNgu = (bieuCam == 0 && huongDi == 'S');

  if (dangNgu) {
    // --- MẶT NGỦ (Nhịp thở phập phồng + chữ Zzz bay lên) ---
    int nhipTho = (int)(sin(now / 450.0) * 2.0);

    display.fillRoundRect(26, 28 + nhipTho, 30, 4, 2, SSD1306_WHITE);
    display.fillRoundRect(72, 28 + nhipTho, 30, 4, 2, SSD1306_WHITE);

    int banKinhMieng = (nhipTho > 0) ? 3 : 1;
    display.drawCircle(64, 48 + nhipTho, banKinhMieng, SSD1306_WHITE);

    int buocZ = (now % 1500) / 500;
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(98, 22 - buocZ * 3);
    display.print(F("z"));
    if (buocZ >= 1) {
      display.setCursor(107, 15 - buocZ * 3);
      display.print(F("z"));
    }
    if (buocZ >= 2) {
      display.setCursor(116, 8);
      display.print(F("Z"));
    }

  } else if (matHienTai == 0) {
    // --- MẶT THỨC (Chớp mắt tự nhiên + liếc theo hướng lái + miệng hào hứng) ---
    int dx = (huongDi == 'L') ? -6 : (huongDi == 'R') ? 6 : 0;
    int dy = (huongDi == 'F') ? -4 : (huongDi == 'B') ? 4 : 0;
    int rungNhe = ((now / 90) % 2 == 0) ? 0 : -1;

    bool dangChopMat = (now % 2800) < 120;

    if (dangChopMat) {
      display.fillRoundRect(26, 26, 30, 4, 2, SSD1306_WHITE);
      display.fillRoundRect(72, 26, 30, 4, 2, SSD1306_WHITE);
    } else {
      display.fillRoundRect(26, 15 + rungNhe, 30, 26, 6, SSD1306_WHITE);
      display.fillRoundRect(72, 15 + rungNhe, 30, 26, 6, SSD1306_WHITE);
      display.fillCircle(41 + dx, 28 + dy + rungNhe, 5, SSD1306_BLACK);
      display.fillCircle(87 + dx, 28 + dy + rungNhe, 5, SSD1306_BLACK);
    }

    int doMoMieng = 3 + ((now / 140) % 3);
    display.drawRoundRect(56, 46, 16, doMoMieng, 2, SSD1306_WHITE);

  } else if (matHienTai == 1) {
    // --- BIỂU CẢM 1: VUI VẺ (^ ^ nhún nhảy + má hồng + cười tươi) ---
    int nhunNhay = (int)(abs(sin(now / 180.0)) * -5.0);

    for (int i = 0; i < 3; i++) {
      display.drawLine(27, 32 + i + nhunNhay, 41, 19 + i + nhunNhay, SSD1306_WHITE);
      display.drawLine(41, 19 + i + nhunNhay, 55, 32 + i + nhunNhay, SSD1306_WHITE);
      display.drawLine(73, 32 + i + nhunNhay, 87, 19 + i + nhunNhay, SSD1306_WHITE);
      display.drawLine(87, 19 + i + nhunNhay, 101, 32 + i + nhunNhay, SSD1306_WHITE);
    }

    if ((now / 250) % 2 == 0) {
      display.drawLine(23, 37 + nhunNhay, 27, 34 + nhunNhay, SSD1306_WHITE);
      display.drawLine(28, 37 + nhunNhay, 32, 34 + nhunNhay, SSD1306_WHITE);
      display.drawLine(96, 37 + nhunNhay, 100, 34 + nhunNhay, SSD1306_WHITE);
      display.drawLine(101, 37 + nhunNhay, 105, 34 + nhunNhay, SSD1306_WHITE);
    }

    int caoMieng = 7 + (int)(abs(sin(now / 180.0)) * 6.0);
    display.fillRoundRect(50, 41 + nhunNhay / 2, 28, caoMieng, 5, SSD1306_WHITE);
    display.fillRect(50, 41 + nhunNhay / 2, 28, 3, SSD1306_BLACK);

  } else if (matHienTai == 2) {
    // --- BIỂU CẢM 2: NGẦU / CHIẾN ĐẤU (\ / nheo mắt + tia sáng quét) ---
    int nheoMat = (int)(sin(now / 200.0) * 2.0);

    display.fillTriangle(24, 18 + nheoMat, 56, 28 + nheoMat, 27, 36 + nheoMat, SSD1306_WHITE);
    display.fillTriangle(104, 18 + nheoMat, 72, 28 + nheoMat, 101, 36 + nheoMat, SSD1306_WHITE);

    int tiaSang = (now % 1200) / 10;
    if (tiaSang < 128) {
      display.drawLine(tiaSang, 16, tiaSang - 6, 38, SSD1306_BLACK);
      display.drawLine(tiaSang + 1, 16, tiaSang - 5, 38, SSD1306_BLACK);
    }

    display.drawLine(52, 51, 68, 51, SSD1306_WHITE);
    display.drawLine(68, 51, 76, 45 - abs(nheoMat), SSD1306_WHITE);

  } else {
    // --- BIỂU CẢM 3: NGẠC NHIÊN (O O mắt đảo & co giãn đồng tử) ---
    int rungMat = ((now / 70) % 2 == 0) ? -1 : 1;
    int coGianDongTu = 4 + ((now / 220) % 3);
    int nhayLongMay = ((now / 250) % 2) * 2;

    display.drawFastHLine(31 + rungMat, 9 - nhayLongMay, 20, SSD1306_WHITE);
    display.drawFastHLine(77 + rungMat, 9 - nhayLongMay, 20, SSD1306_WHITE);

    display.fillCircle(41 + rungMat, 27, 13, SSD1306_WHITE);
    display.fillCircle(41 + rungMat, 27, coGianDongTu, SSD1306_BLACK);
    display.fillCircle(87 + rungMat, 27, 13, SSD1306_WHITE);
    display.fillCircle(87 + rungMat, 27, coGianDongTu, SSD1306_BLACK);

    int banKinhO = 4 + ((now / 220) % 2);
    display.drawCircle(64 + rungMat, 49, banKinhO, SSD1306_WHITE);
  }

  // Thanh trạng thái mini góc dưới cùng
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 56);
  display.print(F("PWM:"));
  display.print(mucPWM);

  if (khoangCach <= 25) {
    display.setCursor(74, 56);
    display.print(F("!CAN:"));
    display.print(khoangCach);
  }

  display.display();
}

// ================= GIAO DIỆN 2: DÒ MAP (RADAR SIÊU ÂM) =================
void veCungRadar(int banKinh) {
  for (int deg = 35; deg <= 145; deg += 3) {
    float rad = deg * PI / 180.0;
    int x = RADAR_CX + banKinh * cos(rad);
    int y = RADAR_CY - banKinh * sin(rad);
    display.drawPixel(x, y, SSD1306_WHITE);
  }
}

void veGiaoDienRadar() {
  display.clearDisplay();

  veCungRadar(18);
  veCungRadar(35);
  veCungRadar(RADAR_R);

  float radBienPhai = 35 * PI / 180.0;
  float radBienTrai = 145 * PI / 180.0;
  display.drawLine(RADAR_CX, RADAR_CY, RADAR_CX + RADAR_R * cos(radBienPhai), RADAR_CY - RADAR_R * sin(radBienPhai), SSD1306_WHITE);
  display.drawLine(RADAR_CX, RADAR_CY, RADAR_CX + RADAR_R * cos(radBienTrai), RADAR_CY - RADAR_R * sin(radBienTrai), SSD1306_WHITE);

  for (int r = 6; r < RADAR_R; r += 5) {
    display.drawPixel(RADAR_CX, RADAR_CY - r, SSD1306_WHITE);
  }

  float radQuet = gocQuet * PI / 180.0;
  display.drawLine(RADAR_CX, RADAR_CY, RADAR_CX + RADAR_R * cos(radQuet), RADAR_CY - RADAR_R * sin(radQuet), SSD1306_WHITE);

  if (khoangCach <= 100) {
    int rVatCan = map(khoangCach, 2, 100, 8, RADAR_R);
    for (int deg = 65; deg <= 115; deg += 2) {
      float rad = deg * PI / 180.0;
      int xObj = RADAR_CX + rVatCan * cos(rad);
      int yObj = RADAR_CY - rVatCan * sin(rad);
      display.drawPixel(xObj, yObj, SSD1306_WHITE);
      display.drawPixel(xObj, yObj - 1, SSD1306_WHITE);
    }
    int yMucTieu = RADAR_CY - rVatCan;
    display.fillCircle(RADAR_CX, yMucTieu, 3, SSD1306_WHITE);
    if ((millis() / 150) % 2 == 0) {
      display.drawCircle(RADAR_CX, yMucTieu, 6, SSD1306_WHITE);
    }
  }

  display.drawFastVLine(87, 0, 64, SSD1306_WHITE);
  display.fillRect(89, 0, 39, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(94, 2);
  display.print(F("SONAR"));

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(90, 16);
  if (khoangCach > 250) {
    display.setTextSize(2);
    display.print(F("---"));
  } else if (khoangCach >= 100) {
    display.setTextSize(1);
    display.setCursor(92, 20);
    display.print(khoangCach);
  } else {
    display.setTextSize(2);
    display.print(khoangCach);
  }

  display.setTextSize(1);
  display.setCursor(114, 33);
  display.print(F("cm"));

  display.setCursor(90, 44);
  if (khoangCach <= 20) {
    display.print(F("STOP!"));
  } else if (khoangCach <= 50) {
    display.print(F("GAN !"));
  } else {
    display.print(F("OK"));
  }

  display.drawRect(89, 55, 38, 8, SSD1306_WHITE);
  if (khoangCach <= 100) {
    int doRongThanh = map(khoangCach, 2, 100, 34, 2);
    display.fillRect(91, 57, doRongThanh, 4, SSD1306_WHITE);
  }

  display.display();
}

// ================= GIAO DIỆN 3: THÔNG SỐ XE & PWM (INFO) =================
void veGiaoDienInfo() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(22, 2);
  display.print(F("THONG SO XE (INFO)"));

  display.setTextColor(SSD1306_WHITE);

  int phanTram = map(mucPWM, 0, 255, 0, 100);
  display.setCursor(2, 16);
  display.print(F("PWM : "));
  display.print(mucPWM);
  display.print(F("/255 ("));
  display.print(phanTram);
  display.print(F("%)"));

  display.drawRect(2, 27, 124, 7, SSD1306_WHITE);
  int barW = map(mucPWM, 0, 255, 0, 120);
  display.fillRect(4, 29, barW, 3, SSD1306_WHITE);

  display.setCursor(2, 38);
  display.print(F("LAI : "));
  switch (huongDi) {
    case 'F': display.print(F("CHAY TIEN")); break;
    case 'B': display.print(F("CHAY LUI"));  break;
    case 'L': display.print(F("RE TRAI"));   break;
    case 'R': display.print(F("RE PHAI"));   break;
    default:  display.print(F("DANG DUNG")); break;
  }

  display.setCursor(2, 47);
  display.print(F("SIEU AM: "));
  if (khoangCach > 250) {
    display.print(F("> 250 cm"));
  } else {
    display.print(khoangCach);
    display.print(F(" cm"));
  }

  display.setCursor(2, 56);
  display.print(F("UART LEO: "));
  display.print(uartKetNoi ? F("KET NOI OK") : F("MAT TIN HIEU"));

  display.display();
}

// ================= SETUP & LOOP CHÍNH =================
void setup() {
  Serial.begin(9600); // Nhận dữ liệu UART từ Arduino Leonardo

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  dungXe();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true);
  }

  display.setRotation(XOAY_MAN_HINH);
}

void loop() {
  docUART();
  capNhatDongCo();

  gocQuet += chieuQuet;
  if (gocQuet >= 145 || gocQuet <= 35) {
    chieuQuet = -chieuQuet;
  }

  if (millis() - lastPing >= 65) {
    lastPing = millis();
    khoangCach = docSieuAm();
  }

  if (cheDoManHinh == 1) {
    veGiaoDienRadar();
  } else if (cheDoManHinh == 2) {
    veGiaoDienInfo();
  } else {
    veGiaoDienMat();
  }

  delay(15);
}