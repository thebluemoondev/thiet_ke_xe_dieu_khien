# Điều khiển 1 motor DC bằng Uno + PS2 + L298N

## 1. Phạm vi và code

Tài liệu hiểu tên “L289N” là **L298N**, motor DC hai dây đã nối vào **OUT1 và OUT2** (kênh A). Kiểm tra tên in trên board trước khi đấu. Chưa biết điện áp/dòng motor, nguồn, model bộ thu và phiên bản Sensor Shield nên không mặc định nguồn motor là 12 V.

`src/main.cpp` đã chuyển từ đọc nút/in Serial sang điều khiển motor. Giữ chân PS2 cũ, dùng thêm ba chân điều khiển kênh A. `platformio.ini` vẫn dùng board Uno và thư viện PS2X hiện có.

## 2. Nối dây tín hiệu

Tắt nguồn khi đấu dây. Gắn Sensor Shield lên Uno đúng hàng chân. Trên shield, **S là tín hiệu, V là nguồn, G là GND**; đọc chữ in trên board, không suy ra theo màu hay hướng cắm.

### Bộ thu PS2 → Sensor Shield / Uno

| Chân bộ thu | Chân Uno (hàng S trên shield) |
| --- | --- |
| DAT / DATA / DO | D12 |
| CMD / DI | D11 |
| ATT / SEL / CS | D10 |
| CLK | D13 |
| GND | GND / hàng G |
| VCC | Nguồn đúng điện áp theo thông số bộ thu |

Chỉ nối VCC vào hàng V 5 V nếu bộ thu xác nhận hỗ trợ 5 V. Bộ thu chỉ dùng 3,3 V cần nguồn 3,3 V và chuyển mức tín hiệu phù hợp với Uno; không đưa thẳng tín hiệu 5 V vào bộ thu đó. Không cấp nguồn vào chân tín hiệu. Nếu bộ thu đã hoạt động với code cũ, giữ nguyên phần đấu PS2.

Không mặc định ổ PS2 chuyên dụng trên shield nối sẵn D10–D13: sơ đồ chân có thể khác giữa các phiên bản. Bảng trên áp dụng khi nối dây rời đúng chân. ACK và chân cấp nguồn rung (nếu có) không dùng trong chương trình này.

### L298N → Sensor Shield / Uno

| Chân L298N | Nối đến |
| --- | --- |
| ENA | D5, hàng S (PWM) |
| IN1 | D7, hàng S |
| IN2 | D8, hàng S |
| GND | GND Uno và âm nguồn chung |
| OUT1, OUT2 | Hai dây motor đang có |
| IN3, IN4 | Không dùng |
| OUT3, OUT4 | Không nối motor |

**Tháo jumper ENA trước khi nối D5.** Nối D5 vào chân tín hiệu ENA, không nối vào chân 5 V còn lại của cặp jumper. Jumper ENA khác với jumper **5V-EN** cấp nguồn logic.

## 3. Cấp nguồn với mạch hạ áp

Cách thử dưới đây dùng **USB cấp nguồn Uno**, hạ áp cấp **5 V cho logic L298N**. Cách này tránh phải đoán cách cấp nguồn ngoài của từng phiên bản shield.

```text
Nguồn DC (+) ──┬── chân VS / VMS / +12V của L298N (nguồn motor)
               └── IN+ mạch hạ áp
Nguồn DC (-) ──┬── IN- mạch hạ áp
               ├── GND L298N
               └── GND Uno / Sensor Shield / bộ thu PS2

Hạ áp OUT+ (đã đo đúng 5,0 V) ── chân 5V L298N
Hạ áp OUT-                  ── GND chung
USB                         ── Uno
Uno D5 / D7 / D8             ── ENA / IN1 / IN2
L298N OUT1 / OUT2            ── motor
```

1. Nguồn DC nhánh motor phải phù hợp motor và module; chữ `+12V` trên cọc không có nghĩa motor nào cũng cấp 12 V. Kiểm tra dòng khởi động/kẹt trục và khả năng tản nhiệt của driver, không chỉ dòng chạy không tải.
2. Chỉnh và đo đầu ra hạ áp **5,0 V khi chưa nối tải**, bảo đảm điện áp đầu vào nằm trong dải cho phép của hạ áp. Nguồn vào phải đủ cao để loại hạ áp đang dùng ổn áp 5 V.
3. **Tháo jumper 5V-EN của L298N** trước khi đưa 5 V ngoài vào cọc 5V. Cách này dành cho module có jumper chọn bộ ổn áp 5 V như tài liệu tham khảo; xác nhận ký hiệu trên module thực tế.
4. Không nối OUT+ hạ áp vào 5V Uno trong sơ đồ dùng USB này. Không nối cọc nguồn motor vào 5V Uno hoặc hàng V của shield. Không cấp motor từ Uno/shield.
5. Bộ thu lấy nguồn đúng thông số của nó; hàng V trên shield chỉ được dùng sau khi xác nhận điện áp và jumper nguồn của shield. Không tự đưa nguồn motor vào cọc nguồn phụ của shield.
6. Nối chung GND giữa mọi phần. Đi dây nguồn motor trực tiếp về nguồn/driver, tránh cho dòng motor đi qua đường mạch Uno.

Muốn chạy độc lập không cắm USB cần xác định điện áp nguồn, model hạ áp và sơ đồ nguồn shield trước khi chọn cách nuôi Uno qua jack/VIN hay nguồn 5 V ổn áp. Sơ đồ trên là cấu hình thử hoàn chỉnh với USB.

## 4. Cách dùng tay cầm

| Thao tác | Kết quả |
| --- | --- |
| Khởi động/kết nối lại | Motor tắt; thả L1, thả mũi tên lên/xuống và đưa cần trái về giữa để sẵn sàng |
| Giữ L1 + đẩy cần trái lên | Quay chiều thuận; đẩy càng xa tốc độ càng cao |
| Giữ L1 + kéo cần trái xuống | Quay chiều ngược; kéo càng xa tốc độ càng cao |
| Giữ L1 + mũi tên lên hoặc xuống | Quay thuận/ngược ở PWM 150 |
| Nhả L1 hoặc đưa cần về giữa, thả mũi tên | Ngắt motor |
| Nhấn X | Ngắt motor và khóa chạy; muốn chạy lại phải thả X, L1, mũi tên và đưa cần về giữa |
| Nhấn đồng thời lên và xuống | Ngắt motor |

Mũi tên ưu tiên hơn cần analog. PWM tối đa ban đầu là **200/255**; vùng chết cần là ±15 quanh 128. Có thể sửa `MAX_PWM` (không quá 255), `BUTTON_PWM` và `DEAD_ZONE` ở đầu code. PWM là mức điều khiển, không phải tốc độ vòng/phút được đo.

Khi đảo chiều, chương trình ngắt motor ít nhất 300 ms trước khi cấp chiều ngược và vẫn đọc PS2 trong lúc chờ. Khoảng nghỉ này không bảo đảm motor tải lớn đã đứng yên. “Ngắt motor” là ENA=0, motor có thể còn quay theo quán tính. Nếu chiều quay ngược ý muốn, tắt nguồn rồi đảo hai dây OUT1/OUT2.

Nếu thư viện báo khung đọc PS2 không hợp lệ, chương trình ngắt motor và thử kết nối lại mỗi giây. **Không bảo đảm phát hiện mọi trường hợp mất sóng**: một số bộ thu có thể tiếp tục trả khung hợp lệ chứa lệnh cũ. Phải thử tắt tay cầm thực tế khi motor không tải và có thể ngắt nguồn motor ngay; nếu motor vẫn chạy thì cấu hình bộ thu này chưa đáp ứng yêu cầu dừng khi mất sóng. Nút L1/X qua vô tuyến cũng không thay thế công tắc ngắt nguồn.

## 5. Nạp và kiểm tra

Trong terminal tại thư mục dự án:

```powershell
pio run
pio run --target upload
pio device monitor --baud 9600
```

1. Ban đầu ngắt nguồn motor, nối USB Uno và bật tay cầm. Kiểm tra Serial báo `PS2 OK` rồi `San sang` sau khi thả nút/cần.
2. Cố định motor, để trục không tải; cấp nguồn motor. Khi vừa cấp nguồn, motor phải đứng yên.
3. Giữ L1 rồi đẩy cần nhẹ, thử cả hai chiều. Thử mũi tên, nhả L1 và nút X.
4. Thử tắt tay cầm, mất kết nối và kết nối lại trong điều kiện có thể cắt nguồn motor ngay. Kiểm tra motor không tự chạy khi kết nối lại mà chưa thả điều khiển.
5. Nếu Uno reset khi motor chạy: kiểm tra nguồn đủ dòng, GND, dây nguồn và nhiễu motor. Nếu motor không quay: kiểm tra nguồn VS, 5 V logic, jumper ENA đã tháo, D5 ở đúng chân ENA và đang giữ L1. PWM thấp có thể chưa đủ lực khởi động.

Code chỉ điều khiển được sau khi `setup()` chạy; thêm điện trở kéo ENA xuống GND khoảng 10 kΩ nếu cần giữ driver tắt khi Uno đang reset/nạp hoặc chưa được cấp nguồn. Chưa kiểm tra trực tiếp trên phần cứng của bạn.

## 6. Tài liệu đối chiếu

- [Arduino: các chân PWM của Uno và analogWrite](https://support.arduino.cc/hc/en-us/articles/9350537961500-Use-PWM-output-with-Arduino).
- [Tài liệu module L298N: nguồn logic, jumper 5V-EN và ENA/PWM](https://www.gotronic.fr/pj2-35233-eng-1776.pdf). Phiên bản module thực tế có thể khác; đối chiếu nhãn trước khi đấu.
- Thư viện cục bộ `.pio/libdeps/uno/Arduino-PS2X/PS2X_lib.cpp`: dùng giá trị trả về của `read_gamepad(false, 0)` để nhận biết lỗi đọc.
