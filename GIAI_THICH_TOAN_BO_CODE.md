# Giải thích toàn bộ mã nguồn SimpleCalculator

**Phiên bản được giải thích:** bản C++ + Win32 API, không dùng Qt hoặc Boost. Tài liệu đối chiếu với các file hiện có trong thư mục `SimpleCalculator`; không phải bản Qt được tạo trước đó.

**Cách đọc:** mở file này trong VS Code, nhấn **Ctrl+Shift+V** để xem Markdown Preview. Nên mở mã nguồn ở một cột và tài liệu ở cột bên cạnh. Tài liệu đi từ kiến trúc, engine đến giao diện; mỗi hàm đều được giải thích theo nhiệm vụ, dữ liệu và luồng xử lý.

## 1. Chương trình gồm những gì?

| File | Vai trò |
|---|---|
| `main.cpp` | Tạo cửa sổ Windows, tạo nút, vẽ giao diện, xử lý chuột/bàn phím và khởi chạy ứng dụng. |
| `calculatorengine.h` | Khai báo lớp `CalculatorEngine`, các hàm công khai và biến trạng thái. |
| `calculatorengine.cpp` | Cài đặt nhập số, phép toán, bộ nhớ, xóa và xử lý lỗi. |
| `engine_tests.cpp` | Chương trình console riêng để kiểm thử engine. |
| `build.bat` | Chạy lệnh biên dịch ứng dụng bằng MinGW g++. |
| `.vscode/tasks.json` | Cho phép VS Code gọi `build.bat` bằng Ctrl+Shift+B. |
| `README.md` | Hướng dẫn cài đặt tối thiểu, build, chạy và các giới hạn. |
| `Calculator.exe` | Chương trình đã được biên dịch, không phải mã nguồn để chỉnh sửa. |

Ứng dụng có hai lớp chính:

- `CalculatorEngine` biết **cách tính và trạng thái đang nhập**. Nó không biết cửa sổ hay nút bấm là gì.
- `CalculatorWindow` biết **cách nhận thao tác và hiển thị**. Nó gọi engine để tính thay vì tự thực hiện các công thức.

Ví dụ, khi bấm nút `7`, giao diện gọi `engine_.digit('7')`. Engine cập nhật dữ liệu. Sau đó giao diện đọc `engine_.display()` và vẽ kết quả. Cách tách này giúp kiểm thử toán học mà không cần mở cửa sổ.

## 2. Một số cú pháp C++ cần hiểu trước

| Cú pháp trong dự án | Ý nghĩa |
|---|---|
| `class CalculatorEngine` | Khai báo kiểu đối tượng gom dữ liệu và các hàm liên quan. |
| `public:` | Các hàm mà phần khác của chương trình có thể gọi. |
| `private:` | Dữ liệu/hàm nội bộ, không cho giao diện sửa tùy ý. |
| `value_`, `window_` | Dấu `_` ở cuối chỉ là quy ước đặt tên biến thành viên. |
| `using Decimal = long double;` | Đặt tên khác cho một kiểu sẵn có; không tạo hệ số học mới. |
| `enum class` | Nhóm giá trị có tên và kiểu rõ ràng, ví dụ `Unary::Square`. |
| `const Decimal&` | Nhận tham chiếu chỉ đọc, không sửa đối số qua tham chiếu này. |
| `display() const` | Hàm không sửa trạng thái thông thường của đối tượng. |
| `static` trên hàm thành viên | Hàm không cần con trỏ `this` và không tự truy cập biến riêng của một đối tượng. |
| `CalculatorEngine::equals()` | Phần định nghĩa bên ngoài lớp của hàm `equals`. |
| `nullptr` | Không trỏ tới đối tượng/tài nguyên nào. |
| `auto` | Compiler suy ra kiểu từ biểu thức khởi tạo. |
| `condition ? a : b` | Chọn `a` nếu điều kiện đúng, ngược lại chọn `b`. |
| `[](...) { ... }` | Lambda: một hàm nhỏ viết tại chỗ. |
| `try` / `throw` / `catch` | Thực hiện thao tác, phát sinh ngoại lệ, rồi xử lý ngoại lệ. |
| `std::` | Thành phần thuộc thư viện chuẩn C++. |
| `L"..."`, `L'0'` | Chuỗi/ký tự rộng, dùng với các API Windows Unicode. |
| `&` trong `a & FLAG` | Phép AND bit để kiểm tra cờ, khác với `&` trong khai báo tham chiếu. |
| `|` giữa các cờ | Ghép nhiều cờ bit; không phải OR logic `||`. |

Các lớp không dùng kế thừa: OOP trong dự án nằm ở việc đóng gói, kiểm soát truy cập và phối hợp các đối tượng. `CalculatorWindow` **chứa** một `CalculatorEngine`, gọi là quan hệ composition.

## 3. File `calculatorengine.h`: giao diện của engine

### 3.1. Header và thư viện

`#pragma once` ngăn nội dung header bị khai báo lặp trong cùng một đơn vị biên dịch khi được include nhiều lần. `<exception>` cung cấp kiểu ngoại lệ cơ sở; `<string>` cung cấp `std::string`.

File `.h` mô tả lớp có những gì; phần lớn thân hàm nằm ở `.cpp`. Vì cả giao diện lẫn chương trình kiểm thử đều include header này, chúng có thể dùng engine mà không phải chép lại logic.

### 3.2. Kiểu số và các enum

```cpp
using Decimal = long double;
enum class Unary { Percent, Sqrt, Square, Reciprocal };
enum class Memory { Clear, Recall, Add, Subtract, Store };
```

Tên `Decimal` giúp thay kiểu số ở một vị trí, nhưng **kiểu thực tế vẫn là `long double`**, không phải decimal chính xác theo cơ số 10. Độ chính xác phụ thuộc compiler/nền tảng.

`Unary` chỉ phép toán một toán hạng: `%`, căn, bình phương, nghịch đảo. `Memory` chỉ thao tác với thanh ghi bộ nhớ. Enum rõ nghĩa hơn các số quy ước như `unary(1)`.

### 3.3. Các biến trạng thái

| Biến | Giá trị đầu | Ý nghĩa |
|---|---|---|
| `value_` | `0` | Giá trị số hiện tại: số đang nhập hoặc kết quả vừa tính. |
| `left_` | `0` | Toán hạng bên trái của phép tính đang chờ. |
| `repeatRight_` | `0` | Toán hạng bên phải được lưu để lặp khi nhấn `=`. |
| `memory_` | `0` | Số được lưu trong bộ nhớ M. |
| `entry_` | `"0"` | Chuỗi nhập nguyên dạng, như `"12."` hoặc `"0.50"`. |
| `expression_` | Rỗng | Dòng biểu thức, ví dụ `"12 +"` hoặc `"12 + 3 ="`. |
| `error_` | Rỗng | Thông báo lỗi hiện tại. Rỗng nghĩa là không có lỗi. |
| `pending_` | `0` | Toán tử đang chờ: `+`, `-`, `*`, `/`; ký tự giá trị 0 nghĩa là không có. |
| `repeat_` | `0` | Toán tử của phép tính cuối để thực hiện `=` lần nữa. |
| `editing_` | `false` | Người dùng có đang chỉnh chuỗi nhập hay không. |
| `operandReady_` | `false` | Toán hạng hiện tại đã được cung cấp sau toán tử đang chờ hay chưa. |
| `hasMemory_` | `false` | Đã lưu bộ nhớ hay chưa, kể cả khi số được lưu là 0. |

Không thể gộp `editing_` và `operandReady_` thành một cờ: sau khi lấy căn, người dùng không còn sửa một chuỗi nhập, nhưng giá trị căn vẫn là toán hạng hợp lệ. Khi đó `editing_ = false` và `operandReady_ = true`.

Không thể chỉ giữ `value_`: một số thực không phân biệt được cách nhập `12`, `12.` và `12.00`. Chuỗi `entry_` giữ những chi tiết này cho tới khi chuyển sang hiển thị kết quả.

### 3.4. Các hàm đọc trạng thái trong header

```cpp
const std::string& expression() const { return expression_; }
bool hasMemory() const { return hasMemory_; }
bool hasError() const { return !error_.empty(); }
```

`expression()` trả tham chiếu chỉ đọc để tránh sao chép chuỗi. Không nên giữ tham chiếu này để dùng sau khi engine đã bị hủy. Hai hàm còn lại trả thông tin để UI hiển thị `[M]`, bật/tắt MC/MR hoặc biết engine đang lỗi.

Các hàm public còn lại được giải thích đầy đủ ở mục 4. Các hàm private hỗ trợ chuyển đổi số, kiểm tra kết quả và quản lý trạng thái.

## 4. File `calculatorengine.cpp`: từng hàm tính toán

### 4.1. Các thư viện

| Include | Dùng để làm gì? |
|---|---|
| `"calculatorengine.h"` | Biết khai báo lớp đang được cài đặt. |
| `<cmath>` | `std::sqrt`, `std::abs`, `std::isfinite`. |
| `<iomanip>` | `std::setprecision`, điều khiển cách xuất số. |
| `<locale>` | Cố định quy tắc đọc/ghi số với dấu chấm. |
| `<sstream>` | Chuyển số ↔ chuỗi bằng stream trong bộ nhớ. |
| `<stdexcept>` | `overflow_error`, `domain_error`, `invalid_argument`. |
| `<algorithm>` | `std::count_if` để đếm chữ số. |

### 4.2. `format(const Decimal& value)` — định dạng kết quả

```cpp
if (value == 0) return "0";
std::ostringstream out;
out.imbue(std::locale::classic());
out << std::setprecision(15) << std::defaultfloat << value;
return out.str();
```

Hàm trả chuỗi dùng trên màn hình và trong biểu thức. Trường hợp số 0 được trả trực tiếp, giúp kết quả âm 0 cũng hiển thị `0`. Khi đang nhập, chuỗi `-0` vẫn có thể xuất hiện vì `display()` dùng `entry_`.

`ostringstream` là nơi ghi dữ liệu giống `cout`, nhưng ghi vào chuỗi. `locale::classic()` dùng quy ước số ổn định, không phụ thuộc thiết lập vùng của Windows.

`setprecision(15)` kết hợp `defaultfloat` nghĩa là tối đa **15 chữ số có nghĩa**, không phải luôn 15 số sau dấu chấm. Số rất lớn/nhỏ có thể chuyển sang ký hiệu khoa học như `1e+20`.

Việc định dạng không sửa `value_`. Ví dụ `1 / 3` được giữ nội bộ với độ chính xác của `long double`, dù màn hình chỉ hiển thị `0.333333333333333`.

### 4.3. `checked(Decimal value)` — kiểm tra phạm vi

Hàm nhận một kết quả số, kiểm tra rồi trả lại chính số đó:

1. `std::isfinite(value)` phải đúng. NaN hoặc vô cực gây ngoại lệ `Overflow`.
2. Nếu số khác 0, độ lớn phải nằm trong `[1e-100, 1e100]`. Ngoài khoảng này gây `Result out of range`.
3. Nếu hợp lệ, trả `value`.

Hậu tố `L` trong `1e100L` cho biết literal thuộc kiểu `long double`. `std::abs` lấy độ lớn nên giới hạn áp dụng cả số âm và dương. Giới hạn này là lựa chọn của ứng dụng, không phải giới hạn tối đa của mọi `long double`.

### 4.4. `calculate(a, op, b)` — thực hiện một phép toán hai ngôi

`switch (op)` chọn cộng, trừ, nhân hoặc chia. Mỗi nhánh tính kết quả rồi gọi `checked()`.

Riêng phép chia kiểm tra `b == 0` trước. Nếu bằng 0, hàm ném `std::domain_error("Cannot divide by zero")`. Không dùng ngưỡng gần 0, vì những số nhỏ nhưng khác 0 vẫn có thể là mẫu số hợp lệ.

Toán tử không được hỗ trợ gây `std::invalid_argument("Unknown operation")`. Nhánh này bảo vệ hàm; luồng nút bấm bình thường chỉ gửi bốn toán tử hợp lệ.

Đây là hàm `static`: nó chỉ cần các đối số `a`, `op`, `b`, không cần tự đọc trạng thái đang nhập.

### 4.5. `display()` — chọn nội dung màn hình

Thứ tự ưu tiên:

1. Có lỗi → trả `error_`.
2. Đang nhập → trả nguyên `entry_`.
3. Đang xem kết quả → trả `format(value_)`.

Nhờ đó, khi nhập `12.30`, số 0 cuối không biến mất. Sau khi tính, định dạng kết quả có thể bỏ các số 0 không cần thiết.

### 4.6. `clear()` — nút C

Hàm đưa các giá trị tính toán về 0; đặt chuỗi nhập về `"0"`; xóa biểu thức và lỗi; hủy phép toán chờ và phép lặp; tắt các cờ nhập.

`memory_` và `hasMemory_` không bị sửa. Vì vậy **C xóa phép tính nhưng không xóa bộ nhớ M**.

### 4.7. `fail(const std::exception& error)` — chuyển ngoại lệ thành trạng thái lỗi

Hàm gọi `clear()` để loại bỏ phép tính đang dở, rồi chép `error.what()` vào `error_`. Lần đọc `display()` tiếp theo trả thông báo này.

`binary()`, `equals()`, `unary()` và phần cập nhật memory có `catch` gọi `fail()`. Nhờ vậy lỗi toán học không thoát ra ngoài và làm đóng cửa sổ. Bộ nhớ đã lưu vẫn được giữ vì `clear()` không xóa nó.

### 4.8. `beginEntry()` — chuẩn bị nhập số

Hàm được gọi trước khi nhập chữ số hoặc dấu thập phân:

1. Nếu đang lỗi thì `clear()` để bắt đầu lại.
2. Nếu trước đó không ở chế độ nhập, tạo chuỗi mới `"0"`, bật `editing_`.
3. Khi không có toán tử đang chờ, xóa dòng biểu thức cũ.
4. Đặt `operandReady_ = true` vì người dùng đang cung cấp toán hạng.
5. Đặt `repeat_ = 0` để số nhập mới không dùng lại phép `=` trước đó.

Điều này giải thích vì sao gõ `7` sau kết quả `18` cho `7`, không cho `187`.

### 4.9. `syncEntry()` — chuyển chuỗi nhập thành số

```cpp
std::istringstream input(entry_);
input.imbue(std::locale::classic());
input >> value_;
```

Hàm đọc chuỗi `entry_` vào `value_`. Chuỗi vẫn được giữ để tiếp tục chỉnh sửa. Đọc trực tiếp vào `long double` tránh bước trung gian qua `double`, nhưng vẫn là số học nhị phân hữu hạn.

Hàm không kiểm tra `input.fail()`. Trong bản hiện tại, các hàm nhập chỉ tạo chuỗi số ngắn, hợp lệ như `0`, `-0.5`, `12.`. Nếu sau này thêm dán chuỗi từ clipboard hoặc đọc biểu thức bên ngoài, phải bổ sung kiểm tra đầu vào và trạng thái parse.

### 4.10. `digit(char digit)` — nhập số 0–9

Hàm bỏ qua ký tự ngoài `'0'` đến `'9'`, rồi gọi `beginEntry()`.

Nếu chuỗi đang là `"0"` hoặc `"-0"`, một chữ số khác 0 thay thế ký tự 0 cuối. Ví dụ `"-0"` nhận `'5'` thành `"-5"`. Nhập thêm 0 vào chuỗi `"0"` không tạo nhiều số 0 ở đầu.

Với các chuỗi khác, `std::count_if` đếm ký tự số. Lambda:

```cpp
[](char c) { return c >= '0' && c <= '9'; }
```

trả đúng nếu ký tự là chữ số. Khi đã đủ 15 chữ số, ký tự mới bị bỏ qua. Dấu âm và dấu chấm không tính vào giới hạn; số 0 trước dấu chấm có tính.

Nếu chưa đủ giới hạn, nối ký tự bằng `entry_ += digit`. Sau đó `syncEntry()` cập nhật giá trị số.

### 4.11. `decimalPoint()` — nhập dấu chấm

Gọi `beginEntry()`, tìm dấu chấm trong `entry_`. Nếu `find('.') == std::string::npos`, nghĩa là chưa có dấu chấm, mới nối thêm `.`. Cuối cùng đồng bộ giá trị số.

Ví dụ bấm dấu chấm đầu tiên cho `0.`. Gõ `1.2.3` chỉ tạo `1.23`, vì dấu chấm thứ hai bị bỏ qua.

### 4.12. `toggleSign()` — nút ±

Khi đang lỗi, hàm không làm gì. Khi đang nhập, hàm thêm hoặc xóa dấu `-` ở đầu chuỗi rồi parse lại. Khi đang xem kết quả, chỉ thực hiện `value_ = -value_`.

Sau đó hàm đánh dấu toán hạng sẵn sàng, hủy phép lặp `=`, và xóa biểu thức cũ nếu không có phép chờ.

Chi tiết của bản hiện tại: bấm ± ngay sau C tác động lên kết quả 0, không tạo một tiền tố nhập số âm. Gõ `5` tiếp theo sẽ bắt đầu số mới dương. Để nhập `-5`, nhập `5` rồi bấm ±. Để nhập `-0.5`, có thể nhập `0`, bấm ±, rồi `.5`.

### 4.13. `backspace()` — xóa một ký tự

Nếu đang lỗi, gọi `clear()`. Nếu không đang nhập, bỏ qua: không sửa chuỗi kết quả đã tính.

Khi đang nhập, `pop_back()` bỏ ký tự cuối. Nếu chuỗi còn rỗng hoặc chỉ `"-"`, đặt lại `"0"`. Cuối cùng parse lại.

Ví dụ `12.30` → `12.3` → `12.` → `12`. Việc giữ chuỗi nhập riêng khiến xóa dấu chấm hoạt động tự nhiên.

### 4.14. `clearEntry()` — nút CE

Nếu đang lỗi, xóa toàn bộ trạng thái phép tính bằng `clear()`. Nếu không lỗi, chỉ đặt giá trị hiện tại và chuỗi nhập về 0, bật chế độ nhập, đánh dấu toán hạng sẵn sàng và hủy phép lặp.

Hàm giữ `left_` và `pending_`. Vì vậy `12 + 99`, CE, `3 =` cho `15`: chỉ `99` bị thay thế.

| Thao tác | Số đang nhập | Phép chờ | Bộ nhớ |
|---|---|---|---|
| Backspace | Xóa ký tự cuối nếu đang nhập | Giữ | Giữ |
| CE | Đưa về 0 | Giữ khi không ở trạng thái lỗi | Giữ |
| C | Đưa về 0 | Xóa | Giữ |
| MC | Không đổi | Không đổi | Xóa |

### 4.15. `binary(char op)` — nhận +, −, ×, ÷

Hàm bỏ qua nếu đang lỗi hoặc `op` không thuộc `"+-*/"`.

Trong `try`:

1. Nếu đã có `pending_` và có toán hạng mới (`operandReady_`), tính phép cũ trước.
2. Đưa `value_` hiện tại vào `left_`.
3. Lưu toán tử mới vào `pending_`, hủy phép lặp.
4. Đặt `editing_ = false`, `operandReady_ = false` để chờ số bên phải.
5. Dòng biểu thức trở thành, ví dụ, `"5 *"`.

Đây là cách Standard tính **theo thứ tự thao tác**, không phân tích ưu tiên nhân/chia trong cả biểu thức. Với `2 + 3 × 4`, lúc bấm `×`, phép `2 + 3` đã ra 5. Cuối cùng `5 × 4 = 20`.

Nếu bấm `2 + × 3`, sau `+` chưa có toán hạng mới, nên `×` chỉ thay phép chờ. Kết quả là `6`.

### 4.16. `equals()` — tính kết quả và lặp phép cuối

Có ba nhánh:

**Nhánh 1: có `pending_`.** Chọn toán hạng phải:

```cpp
const Decimal right = operandReady_ ? value_ : left_;
```

Nếu đã nhập số bên phải thì dùng số đó. Nếu chưa nhập thì dùng chính số bên trái: `5 + =` thành `5 + 5 = 10`.

Hàm tạo chuỗi biểu thức trước khi ghi đè kết quả, gọi `calculate()`, lưu `repeat_` và `repeatRight_`, rồi xóa `pending_`.

**Nhánh 2: không có phép chờ nhưng có `repeat_`.** Tính lại với `value_` là bên trái, còn toán tử và bên phải lấy từ lần trước. Ví dụ `2 × 3 =` ra 6; bấm `=` nữa ra `6 × 3 = 18`.

**Nhánh 3: không có cả phép chờ lẫn phép lặp.** Chỉ đặt dòng biểu thức thành `"7 ="`; giá trị vẫn là 7.

Cuối hàm, chuyển sang hiển thị kết quả bằng `editing_ = false`, đánh dấu giá trị sẵn sàng dùng tiếp. Ngoại lệ được chuyển vào `fail()`.

### 4.17. `unary(Unary op)` — phần trăm, căn, bình phương, nghịch đảo

Hàm lưu chuỗi giá trị cũ vào `before` trước khi tính để có thể tạo nhãn như `sqrt(9)`.

| Lựa chọn | Công thức/hành vi | Nhãn |
|---|---|---|
| `Percent`, đang chờ `+` hoặc `-` | `left_ * value_ / 100` | `10%` |
| `Percent`, các trường hợp khác | `value_ / 100` | `10%` |
| `Sqrt` | Từ chối số âm, sau đó `std::sqrt(value_)` | `sqrt(9)` |
| `Square` | `value_ * value_` | `sqr(3)` |
| `Reciprocal` | `calculate(1, '/', value_)` | `1/(4)` |

Mọi kết quả được kiểm tra bằng `checked()` trực tiếp hoặc thông qua `calculate()`.

Ví dụ phần trăm:

- `200 + 10 %`: giá trị bên phải trở thành 20; bấm `=` cho 220.
- `200 − 10 %`: bên phải trở thành 20; kết quả 180.
- `200 × 10 %`: bên phải trở thành 0.1; kết quả 20.
- `200 ÷ 10 %`: bên phải trở thành 0.1; kết quả 2000.
- `10 %` đứng riêng cho 0.1.

Nếu có phép chờ, dòng biểu thức gồm bên trái, toán tử và nhãn phép một ngôi; nếu không, chỉ có nhãn. Sau khi bấm `=`, biểu thức do `equals()` tạo dùng giá trị số đã biến đổi, không giữ toàn bộ lịch sử ký hiệu.

Cuối hàm, tắt chỉnh chuỗi nhập, bật toán hạng sẵn sàng và hủy phép lặp trước đó.

### 4.18. `memory(Memory op)` — bộ nhớ máy tính

Đây là **thanh ghi nhớ số của máy tính**, khác với việc cấp phát RAM/GDI của chương trình.

| Nút | Enum | Tác dụng |
|---|---|---|
| MC | `Clear` | Đưa `memory_` về 0 và `hasMemory_` về false. |
| MR | `Recall` | Lấy số nhớ vào `value_`, trở thành một toán hạng hiện tại. |
| M+ | `Add` | Cộng giá trị hiện tại vào `memory_`. |
| M− | `Subtract` | Trừ giá trị hiện tại khỏi `memory_`. |
| MS | `Store` | Ghi đè bộ nhớ bằng giá trị hiện tại. |

MC được xử lý ngay, kể cả khi engine đang lỗi. MR bỏ qua nếu chưa có bộ nhớ; nếu có bộ nhớ và đang lỗi, MR xóa trạng thái lỗi rồi khôi phục số nhớ.

Sau MR, `editing_ = false`, `operandReady_ = true`, `repeat_ = 0`. Một phép chờ hợp lệ được giữ, nên có thể dùng MR để cung cấp toán hạng phải.

M+/M−/MS bị bỏ qua khi đang lỗi. M+/M− kiểm tra kết quả trước khi gán, nên nếu cập nhật bộ nhớ vượt phạm vi, giá trị nhớ cũ vẫn được giữ. Sau cập nhật thành công, `hasMemory_ = true` và tắt chế độ nhập. Vì vậy gõ số sau MS bắt đầu một số mới.

## 5. Theo dõi trạng thái bằng ví dụ

### 5.1. Chuỗi thao tác `2 + 3 = =`

Ký hiệu `—` trong bảng nghĩa là không có toán tử; trong code đó là ký tự giá trị 0.

| Sau thao tác | `value_` | `left_` | `pending_` | `editing_` | `operandReady_` | `repeat_` | Màn hình |
|---|---:|---:|---|---|---|---|---|
| Khởi tạo | 0 | 0 | — | false | false | — | 0 |
| `2` | 2 | 0 | — | true | true | — | 2 |
| `+` | 2 | 2 | + | false | false | — | 2 |
| `3` | 3 | 2 | + | true | true | — | 3 |
| `=` | 5 | 2 | — | false | true | + | 5 |
| `=` tiếp | 8 | 2 | — | false | true | + | 8 |

Sau lần `=` đầu, `repeatRight_ = 3`. `left_` còn giá trị cũ nhưng không dùng cho nhánh lặp; nhánh đó dùng `value_` hiện tại.

### 5.2. Chuỗi `8 / 0 =`, rồi gõ `9`

`equals()` gọi `calculate(8, '/', 0)`. `calculate()` ném ngoại lệ. `catch` gọi `fail()`, xóa phép chờ và lưu thông báo. `display()` trả `Cannot divide by zero`.

Khi gõ `9`, `digit()` gọi `beginEntry()`. Hàm này thấy đang lỗi nên `clear()`, rồi mở chuỗi nhập mới. Kết quả trên màn hình trở thành 9, không tiếp tục phép chia cũ.

### 5.3. Vì sao cần kiểm thử cả trạng thái?

Kiểm thử riêng `2 + 3 = 5` chưa phát hiện được lỗi CE xóa nhầm toán tử, `=` lặp sai số bên phải hoặc gõ số mới nối vào kết quả cũ. Engine là một bộ xử lý có trạng thái; kết quả phụ thuộc cả giá trị lẫn trình tự thao tác.

## 6. File `main.cpp`: giao diện Win32

### 6.1. Giao diện nào do mình vẽ, giao diện nào do Windows tạo?

Windows cung cấp cửa sổ, thanh tiêu đề, nút đóng/thu nhỏ/phóng to, lớp nút `BUTTON`, focus và thông báo click. Code tự quyết định vị trí, màu, chữ, kích thước và cách vẽ phần nội dung.

Các nút là **nút Windows thật có chế độ owner-draw**, không phải chỉ là hình chữ nhật vẽ lên màn hình. Windows tiếp tục xử lý tương tác; ứng dụng chịu trách nhiệm vẽ chúng. Xem [mô tả owner-drawn controls của Microsoft](https://learn.microsoft.com/en-us/windows/win32/controls/user-controls-intro).

Màn hình biểu thức/kết quả được vẽ trực tiếp bằng GDI, không phải một ô nhập văn bản `EDIT`. Vì thế bản hiện tại không có chọn/bôi đen kết quả bằng chuột hoặc Ctrl+C.

### 6.2. Các dòng tiền xử lý và include

```cpp
#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#define NOMINMAX
```

`UNICODE` điều khiển các tên API Windows dùng phiên bản Unicode; `_UNICODE` phục vụ các macro chuỗi tổng quát của runtime. Trong code này `_UNICODE` được định nghĩa cùng nhánh khi `UNICODE` chưa có; phần lớn hàm Windows được gọi trực tiếp bằng tên có hậu tố `W`, nên không phụ thuộc việc chọn phiên bản ngầm.

`NOMINMAX` ngăn `windows.h` định nghĩa macro `min`/`max`, tránh xung đột tên hàm C++.

| Include | Vai trò |
|---|---|
| `<windows.h>` | Khai báo Win32: cửa sổ, thông điệp, GDI, các handle. |
| `<algorithm>` | Trong file `main.cpp` hiện tại không dùng trực tiếp; có thể bỏ include này. Engine có dùng `<algorithm>` riêng. |
| `<string>` | Chuỗi thường và chuỗi rộng. |
| `<vector>` | Lưu danh sách handle các nút. |
| `"calculatorengine.h"` | Dùng engine tính toán. |

### 6.3. Các kiểu Windows thường gặp

| Kiểu | Cách hiểu trong dự án |
|---|---|
| `HWND` | Handle nhận diện cửa sổ hoặc control. Không phải đối tượng C++ để `delete`. |
| `HINSTANCE` | Handle mô-đun ứng dụng, dùng khi đăng ký/tạo cửa sổ. |
| `HFONT` | Handle font GDI. |
| `HBRUSH` | Handle cọ để tô nền. |
| `HDC` | Device context: ngữ cảnh mà GDI dùng để vẽ. |
| `HGDIOBJ` | Kiểu handle tổng quát cho đối tượng GDI. |
| `COLORREF` | Giá trị màu Windows, thường tạo bằng `RGB(r,g,b)`. |
| `RECT` | Hình chữ nhật với `left`, `top`, `right`, `bottom`. |
| `SIZE` | Kích thước `cx`, `cy`. |
| `MSG` | Thông điệp lấy từ hàng đợi Windows. |
| `WPARAM`, `LPARAM` | Dữ liệu đi kèm thông điệp; cách diễn giải tùy loại thông điệp. |
| `LRESULT` | Kiểu trả về của hàm xử lý thông điệp. |
| `UINT` | Số nguyên không dấu, dùng cho mã thông điệp/cờ. |
| `BOOL` | Kiểu nguyên của Windows; không đồng nhất với `bool` C++. |

Một handle chỉ là cách nhận diện tài nguyên. Cần dùng đúng API để quản lý tài nguyên tương ứng, không tự suy diễn đó là con trỏ đến cấu trúc có thể đọc/ghi tùy ý.

### 6.4. Các thành viên của `CalculatorWindow`

| Thành viên | Công dụng |
|---|---|
| `window_` | Handle cửa sổ chính, ban đầu null. |
| `buttons_` | Lưu các handle nút đã tạo. Hiện chỉ `push_back`, chưa được dùng để duyệt/xử lý tiếp. |
| `engine_` | Một đối tượng engine thuộc về cửa sổ này. |
| `buttonFont_` | Font dùng chung khi vẽ chữ các nút. |
| `dark_` | Chọn theme; mặc định dark. |
| `scale_` | Tỷ lệ phần trăm DPI, mặc định 100. |
| `Theme = 100` | ID nút đổi theme. |
| `MemoryStart = 200` | ID đầu của dãy năm nút bộ nhớ. |
| `GridStart = 300` | ID đầu của 24 nút trong lưới. |

Các thành viên xuất hiện trước `public:` nên mặc định là private. Giao diện bên ngoài chỉ cần `run()`. Destructor cũng public để đối tượng có thể được hủy khi rời `wWinMain`.

`buttons_` không sở hữu tài nguyên theo nghĩa tự hủy các cửa sổ con. Việc đóng cửa sổ cha sẽ hủy các nút con; hủy vector chỉ bỏ danh sách handle.

### 6.5. `px(int value)` — đổi đơn vị theo tỷ lệ màn hình

```cpp
return MulDiv(value, scale_, 100);
```

`MulDiv` nhân rồi chia theo số nguyên với quy tắc làm tròn của Windows. Nếu `scale_ = 150`, `px(10)` trả 15. Nhờ đó các lề và cỡ chữ không bị quá nhỏ khi Windows đặt tỷ lệ hiển thị cao hơn 100%.

Đây là đổi kích thước theo DPI ban đầu, khác với responsive khi kéo rộng cửa sổ. Responsive do `layout()` làm. Bản hiện tại dùng system DPI và không xử lý `WM_DPICHANGED` khi chuyển giữa màn hình khác DPI. Xem [các chế độ DPI awareness của Windows](https://learn.microsoft.com/en-us/windows/win32/hidpi/setting-the-default-dpi-awareness-for-a-process).

### 6.6. `background()` và `foreground()` — màu nền và chữ

Hai hàm dùng toán tử ba ngôi để chọn màu theo `dark_`.

- Nền dark: `RGB(32,32,32)`; nền light: `RGB(243,243,243)`.
- Chữ dark: gần trắng; chữ light: gần đen.

Đây là các hàm `const`, chỉ đọc theme. Chúng không thay toàn bộ màu giao diện: màu riêng của nút nằm trong `drawButton()`, màu biểu thức nằm trong `paint()`.

### 6.7. `wide(const std::string& s)` — đổi chuỗi ASCII sang chuỗi rộng

```cpp
return std::wstring(s.begin(), s.end());
```

Engine hiện trả chuỗi ASCII: chữ số, dấu phép tính và thông báo tiếng Anh. Hàm mở rộng từng ký tự sang `wchar_t`, đủ cho những chuỗi này để gọi API `W`.

Hàm **không phải bộ chuyển UTF-8 sang UTF-16**. Nếu đổi thông báo engine thành tiếng Việt UTF-8, cần chuyển mã thật bằng `MultiByteToWideChar` hoặc thay chiến lược chuỗi. Các nhãn ký hiệu trên nút dùng literal rộng ngay từ đầu nên không đi qua hàm này.

### 6.8. `font(int height, int weight)` — tạo font

Hàm gọi `CreateFontW` với tên `Segoe UI`. Đối số `weight` mặc định là `FW_NORMAL`, còn tiêu đề/kết quả dùng `FW_SEMIBOLD`.

Các nhóm đối số còn lại đặt góc chữ bằng 0, không nghiêng/gạch chân/gạch ngang, charset và các lựa chọn precision mặc định, chất lượng `CLEARTYPE_QUALITY`. Chiều cao âm yêu cầu chiều cao ký tự theo quy ước `CreateFont`.

Kết quả là `HFONT`. Mỗi font do hàm tạo cần được `DeleteObject` sau khi không còn được chọn vào DC. `buttonFont_` được giữ cho cả cửa sổ; font đo/vẽ kết quả được tạo và xóa trong từng lần vẽ.

### 6.9. `addButton(int id, const wchar_t* text)` — tạo nút

`CreateWindowW(L"BUTTON", ...)` tạo control thuộc lớp nút chuẩn của Windows. Các cờ:

| Cờ | Ý nghĩa |
|---|---|
| `WS_CHILD` | Nút là cửa sổ con của `window_`. |
| `WS_VISIBLE` | Nút có trạng thái hiển thị. |
| `WS_TABSTOP` | Nút có thể tham gia điều hướng Tab. |
| `BS_OWNERDRAW` | Cửa sổ cha chịu trách nhiệm vẽ nút. |

Tọa độ ban đầu `0,0` và kích thước `1,1` là tạm thời; `layout()` đặt vị trí thật sau đó.

Với cửa sổ con, tham số mang kiểu `HMENU` được dùng để truyền **ID control**, không phải menu. Vì vậy có phép ép:

```cpp
reinterpret_cast<HMENU>(static_cast<INT_PTR>(id))
```

`INT_PTR` có kích thước phù hợp với con trỏ. Khi nhận click, cửa sổ cha dùng ID này để biết nút nào được bấm. `GetModuleHandleW(nullptr)` lấy mô-đun chương trình hiện tại. Handle nút được thêm vào vector bằng `push_back`.

### 6.10. `layout()` — bố trí mọi nút

`GetClientRect` lấy kích thước vùng nội dung, không tính thanh tiêu đề và viền hệ thống. Code đặt lề 10 đơn vị và khoảng cách 4 đơn vị, sau khi đổi qua `px()`.

Nút theme được đặt góc trên bên phải. Hàng memory nằm ở độ cao `px(148)`, chia đều chiều rộng cho 5 nút. `GetDlgItem(window_, id)` tìm control con theo ID; không yêu cầu cửa sổ chính phải là dialog.

Lưới chính bắt đầu ở `px(190)`, sử dụng phần chiều cao còn lại. Với mỗi hàng và cột:

```cpp
const int x1 = width * col / 4;
const int x2 = width * (col + 1) / 4;
const int y1 = height * row / 6;
const int y2 = height * (row + 1) / 6;
```

Code tính hai biên rồi lấy hiệu, giúp phân bổ phần dư khi chiều rộng không chia hết cho 4. ID nút được tính bằng `GridStart + row * 4 + col`.

`MoveWindow` thay vị trí/kích thước control; đối số cuối `TRUE` yêu cầu cập nhật phần vẽ tương ứng. Cuối hàm, `InvalidateRect` đánh dấu nội dung cửa sổ cần vẽ lại.

Phần trên có chiều cao gần như cố định theo DPI; lưới phía dưới giãn theo cửa sổ. Cỡ chữ các nút không tăng theo toàn bộ kích thước cửa sổ. Đây là bố cục responsive bằng công thức tọa độ, không có grid engine của framework.

### 6.11. `refresh()` — đồng bộ UI với engine

Hàm bật/tắt MC và MR bằng `EnableWindow`, dựa trên `hasMemory()`. Tiếp theo, `InvalidateRect(window_, nullptr, FALSE)` đánh dấu vùng cần vẽ lại nhưng không yêu cầu bước xóa nền riêng.

Hàm không trực tiếp gọi `paint()`: Windows sẽ gửi yêu cầu vẽ phù hợp. Việc tách cập nhật dữ liệu và vẽ giúp nhiều thay đổi có thể được xử lý qua cơ chế repaint của hệ thống.

### 6.12. `action(int id)` — nối nút bấm với engine

`using U` và `using M` là alias ngắn cho hai enum, giúp lời gọi dễ đọc.

Hàm chia thành ba nhóm ID:

1. **Theme:** đảo `dark_`; đổi nhãn nút thành chế độ mà người dùng có thể chuyển tới; gọi `RedrawWindow` với `RDW_ALLCHILDREN` để cả nút con được vẽ lại.
2. **200–204:** dùng mảng `ops` để ánh xạ MC/MR/M+/M−/MS sang thao tác memory.
3. **300–323:** lấy chỉ số lưới `index = id - GridStart`, rồi gọi hàm nhập số hoặc phép tính tương ứng.

Bảng ánh xạ đầy đủ của lưới:

| Hàng | Cột 0 | Cột 1 | Cột 2 | Cột 3 |
|---|---|---|---|---|
| 0 | 300: `%` | 301: CE | 302: C | 303: Backspace |
| 1 | 304: `1/x` | 305: `x²` | 306: `√x` | 307: `÷` |
| 2 | 308: 7 | 309: 8 | 310: 9 | 311: `×` |
| 3 | 312: 4 | 313: 5 | 314: 6 | 315: `−` |
| 4 | 316: 1 | 317: 2 | 318: 3 | 319: `+` |
| 5 | 320: `±` | 321: 0 | 322: `.` | 323: `=` |

Chuỗi:

```cpp
const char digits[] = "        789 456 123  0  ";
```

có ký tự số tại đúng vị trí tương ứng của các nút số, còn vị trí khác là dấu cách. Chẳng hạn `digits[8]` là `'7'`, `digits[21]` là `'0'`.

Điều kiện `index >= 0 && index < 24` được kiểm tra trước khi truy cập mảng. Toán tử `&&` đánh giá ngắn mạch, nên ID ngoài khoảng không làm đọc mảng sai chỉ số. Các nút không phải số được xử lý qua `switch`.

Sau mọi thao tác, `refresh()` cập nhật màn hình và trạng thái memory.

### 6.13. `drawText(...)` — hàm vẽ chữ dùng chung

Các đối số lần lượt cho biết DC cần vẽ, nội dung, vùng chữ, cỡ chữ, độ đậm, màu và kiểu căn.

Quy trình:

1. Tạo font phù hợp.
2. `SelectObject` chọn font vào DC và giữ lại font cũ.
3. Đặt màu chữ, chế độ nền chữ trong suốt.
4. `DrawTextW` vẽ chuỗi.
5. Chọn lại font cũ rồi xóa font vừa tạo.

Các cờ của `DrawTextW`:

| Cờ | Tác dụng |
|---|---|
| `DT_SINGLELINE` | Chỉ vẽ một dòng. |
| `DT_VCENTER` | Căn giữa theo chiều dọc trong vùng. |
| `DT_NOPREFIX` | Không diễn giải dấu `&` thành ký hiệu phím tắt. |
| `DT_END_ELLIPSIS` | Nếu quá dài, có thể rút gọn bằng dấu ba chấm ở cuối. |
| `DT_LEFT` hoặc `DT_RIGHT` từ đối số | Chọn căn ngang. |

`RECT` được truyền theo giá trị nên nếu API tác động tới biến hình chữ nhật tại đây thì không sửa hình chữ nhật của bên gọi.

### 6.14. `paint()` — vẽ phần màn hình

Hàm chỉ được gọi khi xử lý `WM_PAINT`. `BeginPaint` cung cấp DC và thông tin vùng cần vẽ; `EndPaint` kết thúc chu kỳ vẽ.

Hàm tô nền, vẽ tiêu đề `Standard` hoặc `Standard [M]`, vẽ biểu thức màu xám ở trên, rồi vẽ kết quả cỡ lớn bên dưới. Nội dung được lấy trực tiếp từ engine mỗi lần vẽ.

Để vừa chiều rộng, hàm bắt đầu với cỡ chữ `px(44)`, tạo font thử, đo chiều rộng chuỗi bằng `GetTextExtentPoint32W`, rồi giảm từng đơn vị nếu còn quá rộng. Font thử luôn được khôi phục/xóa sau mỗi vòng. Sau khi chọn được cỡ, `drawText()` vẽ kết quả thật.

Vòng lặp dừng ở cỡ tối thiểu khoảng `px(12)`. Nếu vẫn quá dài, cơ chế ellipsis trong `drawText` có thể rút gọn. Biểu thức không có tooltip trong bản Win32 này.

`WS_CLIPCHILDREN` trên cửa sổ chính loại vùng nút con khỏi phần vẽ của cha. Vì vậy việc tô nền cửa sổ không nhằm tự vẽ đè tất cả các nút.

### 6.15. `drawButton(const DRAWITEMSTRUCT& item)` — tự vẽ nút

Windows gửi cấu trúc chứa ID, handle nút, DC, hình chữ nhật và trạng thái nút. Hàm không cần `BeginPaint` riêng vì DC đã có trong `item.hDC`.

**Chọn màu:** dựa trên chỉ số lưới để xác định nhóm số, theme và nút `=`. Nhóm số gồm cả ± và dấu thập phân. Màu nền nhóm số sáng hơn nhóm chức năng; `=` có màu nhấn xanh.

**Trạng thái:**

- `ODS_SELECTED`: nút đang nhấn, dùng màu phản hồi.
- `ODS_DISABLED`: nút bị vô hiệu hóa, dùng chữ mờ.
- `ODS_FOCUS`: nút có focus bàn phím, vẽ viền focus.

Không có logic theo dõi hover riêng trong bản hiện tại. Đổi màu khi đang nhấn không đồng nghĩa với hiệu ứng rê chuột.

**Vẽ nền:** tô nền của vùng nút bằng màu nền cửa sổ, rồi chọn cọ màu nút và `NULL_PEN` để gọi `RoundRect`. Kích thước bo góc truyền là `px(8)` theo hai chiều. Tô nền trước giúp các góc ngoài phần bo có màu đúng.

**Vẽ chữ:** đọc nhãn bằng `GetWindowTextW` vào mảng 64 ký tự rộng; dùng font chung `buttonFont_`, nền chữ trong suốt và căn giữa.

**Focus:** thu vùng vào 4 đơn vị bằng `InflateRect` với số âm, rồi `DrawFocusRect`.

Font/cọ/bút cũ được khôi phục trước khi xóa tài nguyên tự tạo. `NULL_PEN` là stock object của hệ thống, không được `DeleteObject` ở đây.

### 6.16. `windowProc(...)` — nơi nhận thông điệp Windows

Khai báo:

```cpp
static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM w, LPARAM l)
```

Windows cần một callback theo dạng hàm thông thường. Hàm thành viên `static` không có đối số `this` ẩn, nên dùng được. `CALLBACK` mô tả quy ước gọi phù hợp với API.

Để callback biết đối tượng C++ nào thuộc về cửa sổ, code dùng `GWLP_USERDATA`:

1. Trong `run()`, đối số cuối của `CreateWindowW` là `this`.
2. Khi nhận `WM_NCCREATE`, Windows cung cấp `CREATESTRUCTW` qua `l`.
3. Lấy `lpCreateParams`, chính là con trỏ đã truyền.
4. Gán handle cửa sổ vào `self->window_`.
5. Lưu `self` bằng `SetWindowLongPtrW`.
6. Các lần sau đọc lại bằng `GetWindowLongPtrW`.

`reinterpret_cast` được dùng ở ranh giới API để diễn giải tham số/giá trị kiểu con trỏ đã biết; `static_cast` đổi `void*` trong `lpCreateParams` về loại đối tượng. Những phép ép này dựa trên quy ước cụ thể của Win32, không phải cách ép tùy ý giữa mọi kiểu.

Nếu chưa có `self`, hàm gọi `DefWindowProcW` để hệ thống xử lý mặc định.

| Thông điệp | Cách xử lý trong code |
|---|---|
| `WM_NCCREATE` | Gắn đối tượng C++ vào cửa sổ trước khi xử lý tạo control. |
| `WM_CREATE` | Tạo font nút, nút theme, 5 nút memory và 24 nút lưới; cập nhật trạng thái ban đầu. |
| `WM_SIZE` | Gọi `layout()` sau thay đổi kích thước. |
| `WM_GETMINMAXINFO` | Đặt kích thước cửa sổ tối thiểu 350 × 540 sau khi scale. Đây là kích thước cửa sổ, không chỉ vùng client. |
| `WM_COMMAND` | Nếu mã thông báo là `BN_CLICKED`, lấy ID nút rồi gọi `action()`. |
| `WM_DRAWITEM` | Chuyển `DRAWITEMSTRUCT` cho `drawButton()` và trả `TRUE`. |
| `WM_PAINT` | Gọi `paint()`. |
| `WM_ERASEBKGND` | Trả 1 để cho biết ứng dụng đảm nhận nền; nền được tô trong `paint()`. |
| `WM_DESTROY` | Gọi `PostQuitMessage(0)` để kết thúc vòng lặp thông điệp. |
| Thông điệp khác | Gọi `DefWindowProcW`, giữ hành vi mặc định của Windows. |

Trong `WM_COMMAND`, `HIWORD(w)` lấy phần cao chứa mã thông báo, `LOWORD(w)` lấy phần thấp chứa ID. Vì thế chương trình phân biệt được một thông báo click và nút cụ thể.

Trong `WM_CREATE`, các escape như `\u232b`, `\u00b2`, `\u221a`, `\u00f7`, `\u00d7`, `\u2212`, `\u00b1` lần lượt tạo ký hiệu Backspace, ², √, ÷, ×, −, ± trong chuỗi rộng. Nhãn chỉ phục vụ hiển thị: phép chia vẫn được engine nhận bằng ký tự ASCII `'/'`.

### 6.17. `keyboard(const MSG& msg)` — xử lý phím ở cấp ứng dụng

Hàm được gọi ngay sau `GetMessage`, trước khi chuyển thông điệp cho cửa sổ/control. Nhờ vậy nhập số vẫn hoạt động khi focus đang ở một nút.

Nếu Ctrl hoặc Alt đang được giữ, hàm trả false để không diễn giải tổ hợp đó thành phép tính. `GetKeyState(...) & 0x8000` kiểm tra bit cho biết phím đang nhấn.

**Với `WM_KEYDOWN`:**

| Phím | Xử lý |
|---|---|
| Enter | `equals()` |
| Backspace | `backspace()` |
| Delete | `clearEntry()` |
| Escape | `clear()` |
| F9 | `toggleSign()` |
| Tab | Tìm control tiếp/trước bằng `GetNextDlgTabItem`, gọi `SetFocus`. |

Shift+Tab dùng tham số đi lùi. Các nút bị disabled không phải mục đích chọn bình thường của điều hướng này.

**Với `WM_CHAR`:** đọc ký tự người dùng thực sự nhập. Chữ số gọi `digit`, `.` hoặc `,` gọi `decimalPoint`, bốn toán tử gọi `binary`, `=` gọi `equals`, `%` gọi phép một ngôi tương ứng. Tách `WM_KEYDOWN` và `WM_CHAR` giúp không phải tự suy luận dấu `+` từ tổ hợp phím trên từng bố cục bàn phím.

Khi đã xử lý, hàm `refresh()` rồi trả true. Vòng lặp sẽ `continue`, tránh chuyển sự kiện đó thêm lần nữa. Enter vì thế có ý nghĩa tính kết quả toàn ứng dụng, không phải kích hoạt tùy ý nút đang focus.

Space không được hàm này nhận; control `BUTTON` xử lý theo hành vi của Windows để bấm nút có focus. Ctrl+C cũng không được cài đặt, dù bản Qt trước đó có hỗ trợ.

### 6.18. `~CalculatorWindow()` — hủy font chung

Destructor kiểm tra `buttonFont_` rồi `DeleteObject`. Đối tượng cửa sổ C++ nằm trên stack trong `wWinMain`, nên destructor chạy khi hàm kết thúc.

Các font tạm, cọ tạm đã được giải phóng tại nơi tạo. Các cửa sổ con được Windows hủy khi cửa sổ cha bị hủy. Không dùng `delete` với `HWND` hoặc `HFONT`.

### 6.19. `run(HINSTANCE instance, int show)` — chuẩn bị và vận hành ứng dụng

Hàm thực hiện theo thứ tự:

1. `SetProcessDPIAware()` khai báo ứng dụng nhận biết system DPI.
2. `GetDC(nullptr)` lấy DC màn hình, `GetDeviceCaps(..., LOGPIXELSX)` đọc DPI, tính `scale_`, rồi `ReleaseDC`.
3. Khởi tạo `WNDCLASSW wc{}` với các trường khác bằng 0.
4. Gán callback `windowProc`, mô-đun, tên lớp cửa sổ, con trỏ chuột mũi tên và icon mặc định.
5. `RegisterClassW` đăng ký lớp với Windows. Thất bại trả 1.
6. `CreateWindowW` tạo cửa sổ chính, kích thước ban đầu 410 × 640 sau scale, vị trí do Windows chọn bằng `CW_USEDEFAULT`.
7. `ShowWindow` hiển thị theo `show`; `UpdateWindow` yêu cầu xử lý vẽ nếu cần.
8. Chạy message loop tới khi đóng ứng dụng.

`WS_OVERLAPPEDWINDOW` cung cấp kiểu cửa sổ desktop quen thuộc với thanh tiêu đề, viền có thể kéo và các nút hệ thống. `WS_CLIPCHILDREN` giúp vùng vẽ của cha không bao phủ các control con.

Đăng ký lớp cửa sổ Win32 khác với khai báo `class` C++: tên `SimpleCalculatorWindow` là tên mà hệ điều hành dùng để biết cửa sổ cần callback nào.

### 6.20. Message loop — tại sao chương trình không kết thúc ngay?

```cpp
MSG msg{};
BOOL status;
while ((status = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
    if (keyboard(msg)) continue;
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
}
return status == -1 ? 1 : static_cast<int>(msg.wParam);
```

`GetMessageW` chờ thông điệp, không phải vòng lặp liên tục tự kiểm tra nút chuột. Giá trị dương nghĩa là có thông điệp để xử lý; 0 nghĩa là nhận `WM_QUIT`; -1 nghĩa là lỗi. Do đó code dùng so sánh `> 0` và kiểm tra -1 sau vòng lặp.

`TranslateMessage` có thể tạo thông điệp ký tự từ thông điệp phím. `DispatchMessageW` chuyển thông điệp tới window procedure của cửa sổ đích, có thể là cửa sổ chính hoặc nút con. Chi tiết vòng lặp theo [tài liệu Window Messages của Microsoft](https://learn.microsoft.com/en-us/windows/win32/learnwin32/window-messages).

Không phải mọi thông điệp đều phải đi qua hàng đợi: trong quá trình tạo cửa sổ hoặc gọi một số API, Windows cũng có thể gọi callback đồng bộ. Vì vậy `WM_NCCREATE` và `WM_CREATE` có thể được xử lý trước khi `CreateWindowW` trả về.

Khi người dùng đóng cửa sổ, xử lý mặc định dẫn tới hủy cửa sổ; `WM_DESTROY` gọi `PostQuitMessage`, message loop kết thúc, `run()` trả kết quả.

### 6.21. `wWinMain(...)` — điểm bắt đầu của chương trình GUI

```cpp
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    CalculatorWindow app;
    return app.run(instance, show);
}
```

MinGW được cấu hình để dùng entry point GUI Unicode này qua `-municode -mwindows`. Tham số thứ hai và chuỗi command line không dùng nên không đặt tên. `instance` nhận diện ứng dụng; `show` cho biết cách hiển thị cửa sổ.

Hàm tạo một đối tượng rồi giao việc cho `run()`. Chương trình kiểm thử có `main()` riêng vì là ứng dụng console, không có cửa sổ Win32.

## 7. Luồng hoàn chỉnh từ click tới kết quả

Ví dụ người dùng bấm nút `7`:

1. Windows nhận thao tác trên control `BUTTON` có ID 308.
2. Control gửi thông báo `BN_CLICKED` trong `WM_COMMAND` tới cửa sổ cha.
3. `windowProc()` lấy ID 308 và gọi `action(308)`.
4. `action()` tính `index = 8`, tra chuỗi `digits`, gọi `engine_.digit('7')`.
5. Engine cập nhật `entry_`, `value_` và cờ trạng thái.
6. `refresh()` đánh dấu màn hình cần cập nhật.
7. Khi xử lý `WM_PAINT`, `paint()` đọc `engine_.display()` rồi vẽ số 7.

Nếu người dùng gõ phím `7`, `keyboard()` gọi trực tiếp `engine_.digit('7')`, sau đó đi tiếp từ bước cập nhật UI. Hai cách nhập dùng chung logic tính toán.

## 8. File `build.bat`: giải thích từng lệnh

### 8.1. Phần chuẩn bị

| Lệnh | Ý nghĩa |
|---|---|
| `@echo off` | Không in từng lệnh batch trước khi chạy. |
| `setlocal` | Giới hạn các thay đổi môi trường của batch trong phạm vi chạy batch. |
| `cd /d "%~dp0"` | Chuyển tới thư mục chứa chính file batch; `/d` cho phép đổi cả ổ đĩa. Dấu ngoặc kép hỗ trợ đường dẫn có khoảng trắng. |
| `where g++ >nul 2>nul` | Tìm compiler trong PATH, ẩn cả stdout và stderr của phép kiểm tra. |
| `if errorlevel 1 (...)` | Nếu tìm compiler thất bại, in hướng dẫn rồi trả mã lỗi. Điều kiện này nghĩa là mã trả về từ 1 trở lên. |
| `exit /b 1` | Thoát batch với mã lỗi 1. |

`%~dp0` là cú pháp của batch/CMD, không phải PowerShell. Người dùng gọi `.\build.bat` từ PowerShell nhưng nội dung batch vẫn được xử lý theo cú pháp batch.

### 8.2. Lệnh biên dịch chính

```text
g++ -std=c++17 -O2 -Wall -Wextra -municode -mwindows -static main.cpp calculatorengine.cpp -o Calculator.exe -lgdi32 -luser32
```

| Thành phần | Ý nghĩa |
|---|---|
| `g++` | Compiler/linker driver cho C++. |
| `-std=c++17` | Chọn chuẩn C++17. |
| `-O2` | Bật mức tối ưu hóa phổ biến cho bản chạy. |
| `-Wall -Wextra` | Bật nhiều cảnh báo để phát hiện lỗi tiềm ẩn; không có nghĩa là bật mọi cảnh báo có thể có. |
| `-municode` | Dùng startup hỗ trợ entry point Unicode của MinGW. |
| `-mwindows` | Build Windows GUI subsystem, không mở cửa sổ console riêng khi chạy bình thường. |
| `-static` | Yêu cầu liên kết tĩnh các thư viện có bản tĩnh thích hợp, gồm runtime MinGW có thể liên kết tĩnh. Không loại bỏ phụ thuộc DLL hệ thống Windows. |
| `main.cpp calculatorengine.cpp` | Hai đơn vị mã nguồn của ứng dụng. Header được include, không cần đưa riêng vào lệnh. |
| `-o Calculator.exe` | Đặt tên file kết quả. |
| `-lgdi32` | Liên kết thư viện Windows GDI để vẽ, tạo font/cọ. |
| `-luser32` | Liên kết thư viện Windows UI: cửa sổ, thông điệp, nút. |

Sau lệnh, batch kiểm tra mã lỗi. Nếu build thất bại thì thoát; nếu thành công thì in `Build succeeded. Run .\Calculator.exe`. Batch chỉ build, không tự mở ứng dụng.

Không đưa `engine_tests.cpp` vào lệnh build GUI: file đó có entry point của chương trình kiểm thử riêng. Không chỉ build `main.cpp`: linker sẽ thiếu định nghĩa các hàm engine.

## 9. File `.vscode/tasks.json`

Đây là cấu hình VS Code gọi lệnh, không tham gia tính toán hoặc vẽ giao diện.

| Thuộc tính | Tác dụng |
|---|---|
| `version: "2.0.0"` | Phiên bản định dạng cấu hình task. |
| `tasks` | Danh sách task; dự án có một task build. |
| `label` | Tên task hiển thị trong VS Code. |
| `type: "shell"` | Chạy lệnh thông qua shell. |
| `command: ".\\build.bat"` | Sau giải mã JSON, chuỗi thực tế là `.\build.bat`; dấu `\` phải escape trong JSON. |
| `options.cwd` | Đặt thư mục làm việc là `${workspaceFolder}`, tức thư mục đã mở trong VS Code. |
| `group.kind: "build"` | Xếp vào nhóm task build. |
| `group.isDefault: true` | Đặt làm build task mặc định, dùng Ctrl+Shift+B. |
| `problemMatcher: ["$gcc"]` | Cho VS Code nhận diện định dạng lỗi/cảnh báo GCC trong output và đưa vào Problems. |

`${workspaceFolder}` là biến của VS Code, không phải tên biến C++. Cần mở đúng thư mục chứa `build.bat` và `main.cpp`.

Nếu bạn chỉ thêm MinGW vào `$env:Path` trong một terminal, task khác có thể chưa nhận PATH đó. Khi đó chạy `build.bat` ngay trong terminal đã cấu hình, hoặc đặt PATH ở Windows rồi mở lại VS Code. File này không có cấu hình debug `launch.json` và không cam kết F5 tự chạy.

## 10. File `engine_tests.cpp`: cách kiểm thử hoạt động

### 10.1. Các thư viện và alias

File include header engine, `<iostream>` để in kết quả, `<stdexcept>` để báo lỗi kiểm thử. `using Engine`, `using U`, `using M` rút ngắn tên lớp/enum.

`static int checks = 0` đếm số điều kiện đã kiểm tra trong file này. Ở phạm vi file, `static` làm biến có liên kết nội bộ, khác cách dùng `static` cho callback thành viên của lớp.

### 10.2. Hàm `expect(condition, message)`

Mỗi lần gọi, tăng bộ đếm. Nếu điều kiện sai, ném `runtime_error(message)`. Chương trình dừng ở kiểm tra sai đầu tiên và đưa thông báo ra stderr.

Hàm không dùng `assert`, vì `assert` có thể bị vô hiệu hóa bởi cấu hình build. Điều kiện trong `expect()` vẫn được kiểm tra ở bản tối ưu `-O2`.

### 10.3. Hàm `result(engine, expected)`

Gọi `expect()` để so sánh chuỗi màn hình thực tế với chuỗi mong đợi. Thông báo lỗi gồm cả `Expected ...` và `got ...`. Điều này kiểm tra cả phép toán lẫn cách hiển thị số/lỗi.

### 10.4. Hàm `enter(engine, text)`

Duyệt từng ký tự: gặp `.` thì gọi `decimalPoint()`, còn lại gọi `digit()`. Nó mô phỏng nhập bằng các phím, không gán thẳng `value_`, nhờ đó kiểm tra cùng luồng logic như UI.

Hàm trợ giúp không hỗ trợ gõ một chuỗi đầy đủ như `"2+3="` hay một số âm với `-`; bài kiểm thử gọi các hàm toán tử/đổi dấu riêng.

### 10.5. `main()` của chương trình kiểm thử

Tạo một engine, thực hiện các chuỗi thao tác trong `try`. Khi cần tình huống mới, `clear()` xóa phép tính. Kiểm thử memory còn dùng MC hoặc MS vì C không xóa bộ nhớ.

Các nhóm kiểm thử trong thứ tự mã nguồn:

| Nhóm | Hành vi được xác nhận |
|---|---|
| Thập phân cơ bản | `0.1 + 0.2` hiển thị `0.3`; biểu thức đúng `0.1 + 0.2 =`. |
| Standard tuần tự | `2 + 3 × 4 = 20`. |
| Đổi toán tử | `2 + × 3 = 6`. |
| Lặp bằng | Từ `2 × 3 = 6`, bấm bằng tiếp cho 18. |
| Nhập sau kết quả | Gõ 7 cho số mới 7; bằng tiếp không lặp phép cũ. |
| Bằng thiếu số phải | `5 + = 10`. |
| Chia 0 | Thông báo lỗi đúng và `hasError()` đúng. |
| Phục hồi lỗi | Nhập 9 thay lỗi, cờ lỗi hết. |
| Căn số âm | Đổi 9 thành -9 rồi căn cho `Invalid input`. |
| CE và nghịch đảo 0 | CE phục hồi về 0; nghịch đảo 0 báo lỗi. |
| Phép một ngôi | Căn 9 ra 3, bình phương 3 ra 9, nghịch đảo 4 ra 0.25. |
| Phần trăm | Bao phủ cộng, trừ, nhân, chia và phần trăm đứng riêng. |
| CE giữa phép tính | `12 + 99`, CE, `3 = 15`. |
| Backspace | Giữ và xóa dấu chấm đúng; xóa chuỗi âm về 0. |
| Đổi dấu | Kiểm tra hành vi ± sau C và nhập `-0.5`. |
| Dấu chấm trùng | `1.2.3` trở thành `1.23`. |
| Giới hạn nhập | Chuỗi 17 chữ số chỉ giữ 15 chữ số đầu. |
| Lưu/đọc/xóa nhớ | MS, C, MR giữ số; MC bỏ cờ memory. |
| Cộng/trừ nhớ | Lưu 10, cộng 5, trừ 2, MR ra 13. |
| MR trong phép chờ | 13 + MR = 26. |
| MR sau lỗi | Khôi phục 13 và bỏ trạng thái lỗi. |
| Số vô hạn tuần hoàn | `1 / 3` định dạng 15 chữ số; nhân lại 3 hiển thị 1. |
| Phạm vi | Bình phương 10 nhiều lần cuối cùng báo vượt phạm vi. |

Trong kiểm tra cuối, sau khi phát sinh lỗi, những lần `unary()` tiếp theo không làm gì vì có kiểm tra `hasError()` ở đầu hàm. Vì thế thông báo phạm vi được giữ tới cuối vòng lặp.

Nếu tất cả điều kiện đúng, in `Passed 44 checks` và trả 0. Nếu có ngoại lệ, in `FAILED: ...` và trả 1. Đây là **44 điều kiện kiểm tra**, không phải 44 bài test độc lập và không phải kiểm thử hình ảnh/UI.

### 10.6. Cách chạy test

Trong terminal tại thư mục dự án:

```powershell
g++ -std=c++17 -O2 -static engine_tests.cpp calculatorengine.cpp -o engine_tests.exe
.\engine_tests.exe
```

Không có `-mwindows` vì cần console để thấy kết quả. Không cần `main.cpp`, `gdi32` hoặc `user32` do engine không gọi Win32.

## 11. Quản lý tài nguyên: bộ nhớ tính toán khác tài nguyên Windows

| Tài nguyên | Tạo/lấy ở đâu? | Thu hồi thế nào? |
|---|---|---|
| Đối tượng `CalculatorWindow app` | Biến cục bộ trong `wWinMain`. | Tự hủy khi hàm kết thúc. |
| Đối tượng engine, string, vector | Thành viên C++ hoặc biến cục bộ. | Destructor của C++ tự quản lý. |
| Cửa sổ chính và các nút con | `CreateWindowW`. | Windows hủy theo vòng đời cửa sổ; con bị hủy cùng cha. |
| `buttonFont_` | `WM_CREATE` gọi `font()`. | Destructor `CalculatorWindow`. |
| Font đo/vẽ tạm | `paint()` / `drawText()`. | Khôi phục font cũ rồi `DeleteObject`. |
| Cọ nền/nút | `CreateSolidBrush`. | `DeleteObject` sau khi không còn được chọn vào DC. |
| DC màn hình | `GetDC(nullptr)`. | `ReleaseDC(nullptr, screen)`. |
| DC trong repaint | `BeginPaint`. | `EndPaint`, không dùng `ReleaseDC` cho cặp này. |
| DC của nút owner-draw | Windows cung cấp qua `DRAWITEMSTRUCT`. | Không tự hủy DC này. |
| Stock pen `NULL_PEN` | `GetStockObject`. | Không xóa stock object. |

Lưu ý mẫu `old = SelectObject(...); ...; SelectObject(..., old); DeleteObject(...)`: đây là cách tránh xóa một font/cọ khi nó vẫn đang được DC sử dụng. Handle cũ không tự động được phục hồi khi biến ra khỏi scope.

## 12. Số thực và các giới hạn cần hiểu đúng

Engine này không dùng bộ số thập phân đa độ chính xác. `long double` biểu diễn số với số lượng bit hữu hạn; nhiều phân số thập phân không có biểu diễn nhị phân hữu hạn.

Kết quả `0.1 + 0.2` hiển thị `0.3` nhờ định dạng 15 chữ số có nghĩa. Điều đó không chứng minh mọi phép tính thập phân đều chính xác tuyệt đối. Khi trừ hai số rất gần nhau hoặc cộng một số rất nhỏ vào số rất lớn, sai số vẫn có thể ảnh hưởng kết quả.

Chương trình giữ số nội bộ sau phép tính, không parse lại chuỗi hiển thị để làm tròn về 15 chữ số. Đây là lý do cách hiển thị và độ chính xác nội bộ cần được phân biệt.

Các giới hạn khác của bản hiện tại:

- Chỉ chạy Windows vì giao diện gọi Win32 trực tiếp.
- Chỉ có Standard tuần tự; không có parser với ngoặc/ưu tiên toán tử, Scientific, history hoặc chuyển đổi đơn vị.
- Theme và memory không được lưu ra file; mở lại trở về dark và bộ nhớ rỗng.
- Không hỗ trợ paste biểu thức hoặc copy kết quả bằng Ctrl+C.
- Có trạng thái pressed/disabled/focus; chưa có hover riêng hay animation.
- Nhận biết system DPI ban đầu, chưa hỗ trợ cập nhật DPI riêng cho từng màn hình.
- Phần display tự vẽ chưa có hỗ trợ accessibility đầy đủ cho trình đọc màn hình.
- Các API tạo font/nút/cọ chưa được kiểm tra thất bại ở mọi nơi; bản này tập trung vào ví dụ ứng dụng desktop dễ build.
- `syncEntry()` giả định chuỗi do chương trình tạo hợp lệ; phải kiểm tra parse nếu bổ sung nguồn nhập bên ngoài.
- Kết quả phép tính ngoài phạm vi ứng dụng được báo lỗi; không có cơ chế số lớn tùy ý.

## 13. Muốn chỉnh sửa thì tìm chỗ nào?

| Muốn đổi | Vị trí cần sửa | Điều cần giữ nhất quán |
|---|---|---|
| Tiêu đề trên thanh cửa sổ | `CreateWindowW` trong `run()`. | Giữ chuỗi rộng `L"..."`. |
| Chữ `Standard` | `paint()`. | Có hai nhánh có/không có `[M]`. |
| Màu nền/chữ chung | `background()`, `foreground()`. | Kiểm tra tương phản cả hai theme. |
| Màu nút số/phép toán/bằng | `drawButton()`. | Giữ trạng thái disabled/focus dễ nhận ra. |
| Độ bo góc | Các đối số cuối của `RoundRect`. | Kích thước đi qua `px()`. |
| Cỡ chữ nút | `font(self->px(18))` trong `WM_CREATE`. | Nút theme có nhãn dài hơn nút số. |
| Cỡ chữ kết quả | `paint()`, giá trị khởi đầu `px(44)`. | Giữ vòng đo chữ để tránh tràn. |
| Kích thước ban đầu | `px(410), px(640)` trong `run()`. | Không nhỏ hơn mức tối thiểu. |
| Kích thước tối thiểu | `WM_GETMINMAXINFO`. | Phải đủ chỗ cho vùng trên và 6 hàng nút. |
| Lề, khoảng cách, vị trí lưới | `layout()`. | Không để chiều cao lưới âm khi thu nhỏ. |
| Nhãn nút | Mảng `grid`/`memory` trong `WM_CREATE`. | Đổi nhãn không tự đổi phép tính. |
| Chức năng nút | `action()`. | Phải khớp ID và vị trí nhãn. |
| Phím tắt | `keyboard()`. | Phân biệt phím điều khiển và ký tự nhập. |
| Số chữ số được nhập | `digit()`, ngưỡng 15. | Xem lại khả năng chính xác của `long double` và test. |
| Số chữ số kết quả | `format()`, `setprecision(15)`. | Tăng số hiển thị có thể lộ sai số nhị phân. |
| Phạm vi kết quả | `checked()`. | Cần kiểm thử trường hợp vượt phạm vi mới. |
| Thông báo lỗi | Các chuỗi truyền cho exception trong engine. | Đổi tiếng Việt cần xử lý Unicode và sửa test mong đợi. |

Ví dụ thay nút `x²` bằng `x³`: cần thêm/chỉnh enum, nhánh tính trong `unary()`, nhãn trong `WM_CREATE`, ánh xạ trong `action()` và kiểm thử tương ứng. Chỉ đổi chữ trên nút sẽ làm giao diện ghi x³ nhưng vẫn tính bình phương.

Ví dụ thêm một hàng nút: ngoài mảng nhãn, cần sửa số hàng trong `layout()`, số lượng tạo nút, phạm vi kiểm tra `index`, bảng `digits` và các nhánh hành động. Bản hiện tại có nhiều chỗ sử dụng cố định 24 nút/6 hàng, chưa tự sinh mọi thứ từ một bảng cấu hình chung.

## 14. Cách đọc và tự trình bày lại dự án

Thứ tự đọc phù hợp cho người mới:

1. Xem `calculatorengine.h` để hiểu dữ liệu của máy tính.
2. Đọc `digit()`, `binary()`, `equals()` và theo dõi ví dụ `2 + 3 =`.
3. Đọc CE/C, các phép một ngôi và memory.
4. Đọc `wWinMain()` và `run()` để hiểu chương trình GUI tồn tại nhờ message loop.
5. Đọc `windowProc()` và `action()` để thấy thao tác được chuyển vào engine.
6. Đọc `layout()`, `paint()`, `drawButton()` để hiểu phần thiết kế giao diện.
7. Chạy kiểm thử trước và sau một thay đổi logic.

Khi giới thiệu dự án, có thể trình bày: “Ứng dụng dùng C++17. Lớp CalculatorEngine quản lý trạng thái nhập và phép tính, độc lập giao diện. Lớp CalculatorWindow dùng Win32 để tạo cửa sổ và nút owner-draw, dùng GDI để vẽ màu/chữ. Thao tác chuột và bàn phím gọi cùng engine; màn hình được vẽ lại dựa trên dữ liệu engine. Chương trình build bằng MinGW g++, không cần Qt hoặc Boost.”

## 15. Kiểm chứng và phạm vi của tài liệu

Mã nguồn bản Win32 đã được build thành công bằng GNU MinGW C++ 14.2.0 trong phiên làm việc này. Bộ kiểm thử engine báo **Passed 44 checks**. Đã kiểm tra tiến trình khởi động và phản hồi vòng lặp; chưa thực hiện kiểm tra trực quan đầy đủ giao diện.

Tài liệu này giải thích mã hiện có, không bổ sung tính năng và không thay đổi cách hoạt động của ứng dụng. Những hạn chế được nêu để phân biệt phần đã cài đặt với phần có thể mở rộng.
