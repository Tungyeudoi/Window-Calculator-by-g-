#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <string>
#include <vector>
#include "calculatorengine.h"

// Windows SDK libraries only. No Qt, Boost, CMake, or third-party runtime.
class CalculatorWindow {
    HWND window_ = nullptr;
    std::vector<HWND> buttons_;
    CalculatorEngine engine_;
    HFONT buttonFont_ = nullptr;
    bool dark_ = true;
    int scale_ = 100;
    enum { Theme = 100, MemoryStart = 200, GridStart = 300 };

    int px(int value) const { return MulDiv(value, scale_, 100); }
    COLORREF background() const { return dark_ ? RGB(32,32,32) : RGB(243,243,243); }
    COLORREF foreground() const { return dark_ ? RGB(248,248,248) : RGB(30,30,30); }
    static std::wstring wide(const std::string& s) { return std::wstring(s.begin(), s.end()); }
    static HFONT font(int height, int weight = FW_NORMAL) {
        return CreateFontW(-height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    }
    void addButton(int id, const wchar_t* text) {
        HWND b = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            0, 0, 1, 1, window_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
            GetModuleHandleW(nullptr), nullptr);
        buttons_.push_back(b);
    }
    void layout() {
        RECT client{}; GetClientRect(window_, &client);
        const int margin = px(10), gap = px(4);
        const int width = client.right - 2 * margin;
        MoveWindow(GetDlgItem(window_, Theme), client.right - px(120), px(10), px(110), px(32), TRUE);
        for (int i = 0; i < 5; ++i) {
            const int x1 = width * i / 5, x2 = width * (i + 1) / 5;
            MoveWindow(GetDlgItem(window_, MemoryStart + i), margin + x1, px(148), x2-x1-gap, px(34), TRUE);
        }
        const int top = px(190), height = client.bottom - top - margin;
        for (int row = 0; row < 6; ++row)
            for (int col = 0; col < 4; ++col) {
                const int x1 = width * col / 4, x2 = width * (col+1) / 4;
                const int y1 = height * row / 6, y2 = height * (row+1) / 6;
                MoveWindow(GetDlgItem(window_, GridStart + row * 4 + col), margin+x1, top+y1,
                    x2-x1-gap, y2-y1-gap, TRUE);
            }
        InvalidateRect(window_, nullptr, TRUE);
    }
    void refresh() {
        EnableWindow(GetDlgItem(window_, MemoryStart), engine_.hasMemory());
        EnableWindow(GetDlgItem(window_, MemoryStart+1), engine_.hasMemory());
        InvalidateRect(window_, nullptr, FALSE);
    }
    void action(int id) {
        using U = CalculatorEngine::Unary;
        using M = CalculatorEngine::Memory;
        if (id == Theme) {
            dark_ = !dark_;
            SetWindowTextW(GetDlgItem(window_, Theme), dark_ ? L"Light mode" : L"Dark mode");
            RedrawWindow(window_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
        } else if (id >= MemoryStart && id < MemoryStart+5) {
            const M ops[] = {M::Clear, M::Recall, M::Add, M::Subtract, M::Store};
            engine_.memory(ops[id-MemoryStart]);
        } else {
            const int index = id - GridStart;
            const char digits[] = "        789 456 123  0  ";
            if (index >= 0 && index < 24 && digits[index] >= '0' && digits[index] <= '9')
                engine_.digit(digits[index]);
            else switch (index) {
            case 0: engine_.unary(U::Percent); break;
            case 1: engine_.clearEntry(); break;
            case 2: engine_.clear(); break;
            case 3: engine_.backspace(); break;
            case 4: engine_.unary(U::Reciprocal); break;
            case 5: engine_.unary(U::Square); break;
            case 6: engine_.unary(U::Sqrt); break;
            case 7: engine_.binary('/'); break;
            case 11: engine_.binary('*'); break;
            case 15: engine_.binary('-'); break;
            case 19: engine_.binary('+'); break;
            case 20: engine_.toggleSign(); break;
            case 22: engine_.decimalPoint(); break;
            case 23: engine_.equals(); break;
            default: break;
            }
        }
        refresh();
    }
    void drawText(HDC dc, const std::wstring& text, RECT rect, int size, int weight, COLORREF color, UINT align) {
        HFONT f = font(size, weight);
        HGDIOBJ old = SelectObject(dc, f);
        SetTextColor(dc, color); SetBkMode(dc, TRANSPARENT);
        DrawTextW(dc, text.c_str(), -1, &rect, align | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
        SelectObject(dc, old); DeleteObject(f);
    }
    void paint() {
        PAINTSTRUCT ps{}; HDC dc = BeginPaint(window_, &ps);
        RECT rc{}; GetClientRect(window_, &rc);
        HBRUSH brush = CreateSolidBrush(background()); FillRect(dc, &rc, brush); DeleteObject(brush);
        RECT title{px(12), px(8), rc.right-px(125), px(46)};
        drawText(dc, engine_.hasMemory() ? L"Standard  [M]" : L"Standard", title, px(22), FW_SEMIBOLD, foreground(), DT_LEFT);
        RECT expression{px(12), px(50), rc.right-px(14), px(78)};
        drawText(dc, wide(engine_.expression()), expression, px(16), FW_NORMAL,
            dark_ ? RGB(190,190,190) : RGB(90,90,90), DT_RIGHT);
        RECT display{px(12), px(80), rc.right-px(14), px(142)};
        const auto text = wide(engine_.display());
        int size = px(44);
        for (; size > px(12); --size) {
            HFONT f = font(size, FW_SEMIBOLD); HGDIOBJ old = SelectObject(dc, f);
            SIZE extent{}; GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &extent);
            SelectObject(dc, old); DeleteObject(f);
            if (extent.cx <= display.right-display.left) break;
        }
        drawText(dc, text, display, size, FW_SEMIBOLD, foreground(), DT_RIGHT);
        EndPaint(window_, &ps);
    }
    void drawButton(const DRAWITEMSTRUCT& item) {
        const int index = static_cast<int>(item.CtlID) - GridStart;
        const bool number = (index >= 8 && index % 4 != 3) || index == 20 || index == 22;
        COLORREF fill = dark_ ? (number ? RGB(60,60,60) : RGB(48,48,48))
                              : (number ? RGB(255,255,255) : RGB(228,228,228));
        COLORREF text = foreground();
        if (index == 23) { fill = dark_ ? RGB(118,185,237) : RGB(0,103,192); text = dark_ ? RGB(15,30,40) : RGB(255,255,255); }
        if (item.itemState & ODS_SELECTED) fill = dark_ ? RGB(90,100,110) : RGB(185,205,222);
        if (item.itemState & ODS_DISABLED) text = dark_ ? RGB(120,120,120) : RGB(145,145,145);
        HBRUSH bg = CreateSolidBrush(background()); FillRect(item.hDC, &item.rcItem, bg); DeleteObject(bg);
        HBRUSH brush = CreateSolidBrush(fill);
        HGDIOBJ oldBrush = SelectObject(item.hDC, brush), oldPen = SelectObject(item.hDC, GetStockObject(NULL_PEN));
        RoundRect(item.hDC, item.rcItem.left, item.rcItem.top, item.rcItem.right, item.rcItem.bottom, px(8), px(8));
        SelectObject(item.hDC, oldBrush); SelectObject(item.hDC, oldPen); DeleteObject(brush);
        wchar_t label[64]{}; GetWindowTextW(item.hwndItem, label, 64);
        HGDIOBJ oldFont = SelectObject(item.hDC, buttonFont_);
        SetBkMode(item.hDC, TRANSPARENT); SetTextColor(item.hDC, text);
        RECT rect = item.rcItem;
        DrawTextW(item.hDC, label, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(item.hDC, oldFont);
        if (item.itemState & ODS_FOCUS) { InflateRect(&rect, -px(4), -px(4)); DrawFocusRect(item.hDC, &rect); }
    }
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM w, LPARAM l) {
        auto* self = reinterpret_cast<CalculatorWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            self = static_cast<CalculatorWindow*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
            self->window_ = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self) return DefWindowProcW(hwnd, message, w, l);
        switch (message) {
        case WM_CREATE: {
            self->buttonFont_ = font(self->px(18));
            self->addButton(Theme, L"Light mode");
            const wchar_t* memory[] = {L"MC", L"MR", L"M+", L"M-", L"MS"};
            for (int i=0; i<5; ++i) self->addButton(MemoryStart+i, memory[i]);
            const wchar_t* grid[] = {
                L"%", L"CE", L"C", L"\u232b",
                L"1/x", L"x\u00b2", L"\u221ax", L"\u00f7",
                L"7", L"8", L"9", L"\u00d7",
                L"4", L"5", L"6", L"\u2212",
                L"1", L"2", L"3", L"+",
                L"\u00b1", L"0", L".", L"="};
            for (int i=0; i<24; ++i) self->addButton(GridStart+i, grid[i]);
            self->refresh(); return 0;
        }
        case WM_SIZE: self->layout(); return 0;
        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(l);
            info->ptMinTrackSize = {self->px(350), self->px(540)}; return 0;
        }
        case WM_COMMAND:
            if (HIWORD(w) == BN_CLICKED) self->action(LOWORD(w));
            return 0;
        case WM_DRAWITEM: self->drawButton(*reinterpret_cast<DRAWITEMSTRUCT*>(l)); return TRUE;
        case WM_PAINT: self->paint(); return 0;
        case WM_ERASEBKGND: return 1;
        case WM_DESTROY: PostQuitMessage(0); return 0;
        default: return DefWindowProcW(hwnd, message, w, l);
        }
    }
    bool keyboard(const MSG& msg) {
        if (GetKeyState(VK_CONTROL) & 0x8000 || GetKeyState(VK_MENU) & 0x8000) return false;
        if (msg.message == WM_KEYDOWN) {
            switch (msg.wParam) {
            case VK_RETURN: engine_.equals(); break;
            case VK_BACK: engine_.backspace(); break;
            case VK_DELETE: engine_.clearEntry(); break;
            case VK_ESCAPE: engine_.clear(); break;
            case VK_F9: engine_.toggleSign(); break;
            case VK_TAB: {
                HWND next = GetNextDlgTabItem(window_, GetFocus(), (GetKeyState(VK_SHIFT)&0x8000) != 0);
                if (next) SetFocus(next);
                return true;
            }
            default: return false;
            }
        } else if (msg.message == WM_CHAR) {
            const wchar_t c = static_cast<wchar_t>(msg.wParam);
            if (c >= L'0' && c <= L'9') engine_.digit(static_cast<char>(c));
            else if (c == L'.' || c == L',') engine_.decimalPoint();
            else if (c == L'+' || c == L'-' || c == L'*' || c == L'/') engine_.binary(static_cast<char>(c));
            else if (c == L'=') engine_.equals();
            else if (c == L'%') engine_.unary(CalculatorEngine::Unary::Percent);
            else return false;
        } else return false;
        refresh(); return true;
    }
public:
    ~CalculatorWindow() { if (buttonFont_) DeleteObject(buttonFont_); }
    int run(HINSTANCE instance, int show) {
        SetProcessDPIAware();
        HDC screen = GetDC(nullptr);
        scale_ = MulDiv(GetDeviceCaps(screen, LOGPIXELSX), 100, 96); ReleaseDC(nullptr, screen);
        WNDCLASSW wc{};
        wc.lpfnWndProc = windowProc; wc.hInstance = instance;
        wc.lpszClassName = L"SimpleCalculatorWindow";
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        if (!RegisterClassW(&wc)) return 1;
        window_ = CreateWindowW(wc.lpszClassName, L"Simple Calculator - C++", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT, px(410), px(640), nullptr, nullptr, instance, this);
        if (!window_) return 1;
        ShowWindow(window_, show); UpdateWindow(window_);
        MSG msg{}; BOOL status;
        while ((status = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
            if (keyboard(msg)) continue;
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        return status == -1 ? 1 : static_cast<int>(msg.wParam);
    }
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    CalculatorWindow app;
    return app.run(instance, show);
}
