# Simple Calculator — C++ thuần + Win32

Chỉ cần Windows và g++/MinGW. Không cần Qt, Qt Online Installer, Boost, CMake, Ninja hay tải thêm thư viện. Win32 API và các thư viện Windows cần dùng đã có trong bộ MinGW. VS Code là trình soạn thảo, compiler g++ mới là công cụ biên dịch.

## Chạy trong VS Code

1. Giải nén ZIP, chọn File → Open Folder → thư mục SimpleCalculator chứa main.cpp và build.bat.
2. Mở Terminal → New Terminal (PowerShell).
3. Kiểm tra `g++ --version`. Nếu không nhận lệnh và máy có CodeBlocks/MinGW ở đường dẫn dưới đây, chạy:

```powershell
$env:Path = "C:\Program Files\CodeBlocks\MinGW\bin;$env:Path"
```

4. Build và chạy:

```powershell
.\build.bat
.\Calculator.exe
```

Đã kèm task VS Code: nhấn Ctrl+Shift+B để build nếu g++ có trong PATH của VS Code. Việc đổi PATH trong terminal chỉ ảnh hưởng terminal đó; nếu task chưa nhận g++, dùng `build.bat` trong terminal hoặc thêm thư mục MinGW/bin vào PATH của Windows và mở lại VS Code.

Hoặc dùng trực tiếp một lệnh:

```powershell
g++ -std=c++17 -O2 -municode -mwindows -static main.cpp calculatorengine.cpp -o Calculator.exe -lgdi32 -luser32
```

Đóng ứng dụng trước khi build lại để tránh lỗi `Permission denied` khi ghi đè file exe. Có thể cài extension C/C++ của Microsoft để gợi ý code, nhưng không bắt buộc để chạy các lệnh trên. Không dùng Run Code để build riêng main.cpp.

## Các file

- main.cpp: cửa sổ Windows, vẽ giao diện, nút bấm, bàn phím và bố cục co giãn.
- calculatorengine.h/.cpp: logic OOP độc lập giao diện.
- build.bat: biên dịch bằng g++.
- .vscode/tasks.json: phím tắt build.
- engine_tests.cpp: kiểm thử logic.

## Chức năng

Đủ số 0–9, dấu thập phân, cộng/trừ/nhân/chia, %, căn, bình phương, nghịch đảo, đổi dấu, C/CE/Backspace, MC/MR/M+/M-/MS, Light/Dark mode. Standard tính theo thứ tự thao tác, ví dụ 2 + 3 × 4 = 20. Nhấn = liên tiếp lặp phép tính cuối.

Bàn phím: số và + - * / %, Enter hoặc = để tính, Backspace để xóa ký tự, Delete cho CE, Esc cho C, F9 đổi dấu; Tab/Shift+Tab di chuyển focus, Space bấm nút có focus. Dấu phẩy được nhận như dấu chấm. Theme và bộ nhớ được giữ trong phiên chạy, không lưu sau khi đóng.

Chia 0 báo Cannot divide by zero; căn số âm báo Invalid input. Gõ số mới hoặc dùng C/CE để phục hồi. C không xóa bộ nhớ; MC mới xóa bộ nhớ.

## Độ chính xác

Bản đơn giản dùng long double của C++, nhập tối đa 15 chữ số, hiển thị 15 chữ số có nghĩa: 0.1 + 0.2 hiển thị 0.3. Đây là làm tròn khi hiển thị, không phải số thập phân chính xác tuyệt đối như engine decimal. Các chuỗi tính có triệt tiêu số gần nhau vẫn có thể lộ sai số nhị phân. Phạm vi kết quả khác 0 được giới hạn ở 1e-100 đến 1e100.

Giao diện lấy cảm hứng từ Windows Calculator; cửa sổ, nút và vẽ GDI dùng trực tiếp Win32 nên chỉ chạy Windows. Không có Scientific, lịch sử, chuyển đổi đơn vị hoặc hỗ trợ accessibility đầy đủ cho phần màn hình tự vẽ.

## Chạy kiểm thử

```powershell
g++ -std=c++17 -O2 -static engine_tests.cpp calculatorengine.cpp -o engine_tests.exe
.\engine_tests.exe
```

Đã biên dịch thành công bằng MinGW GCC 14.2.0 trên Windows và chạy đạt 44 kiểm tra logic. Đã kiểm tra khởi động tiến trình ứng dụng và vòng lặp thông điệp phản hồi; chưa kiểm tra trực quan giao diện. File Calculator.exe đã được build sẵn và kèm trong thư mục.
