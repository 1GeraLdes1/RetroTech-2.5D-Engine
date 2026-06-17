#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

class CustomPixelEngine
{
public:
    CustomPixelEngine(int screenWidth, int screenHeight, int windowScale, const wchar_t* title)
        : m_width(screenWidth),
          m_height(screenHeight),
          m_scale(windowScale),
          m_baseTitle(title)
    {
    }

    virtual ~CustomPixelEngine()
    {
        UnlockMouse();
    }

    int Run()
    {
        if (!CreateEngineWindow())
            return 1;

        SetupFramebuffer();

        if (!OnCreate())
            return 1;

        LARGE_INTEGER frequency;
        LARGE_INTEGER previousCounter;

        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&previousCounter);

        MSG message {};

        while (m_running)
        {
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT)
                    m_running = false;

                TranslateMessage(&message);
                DispatchMessageW(&message);
            }

            LARGE_INTEGER currentCounter;
            QueryPerformanceCounter(&currentCounter);

            const float elapsedTime =
                static_cast<float>(currentCounter.QuadPart - previousCounter.QuadPart) /
                static_cast<float>(frequency.QuadPart);

            previousCounter = currentCounter;

            UpdateFps(elapsedTime);
            UpdateMouseCaptureToggle();
            UpdateMouseLook();

            if (!OnUpdate(elapsedTime))
                m_running = false;

            Present();
        }

        UnlockMouse();
        OnDestroy();

        return 0;
    }

protected:
    virtual bool OnCreate()
    {
        return true;
    }

    virtual bool OnUpdate(float elapsedTime)
    {
        return true;
    }

    virtual void OnMouseMove(float mouseDeltaX)
    {
        (void)mouseDeltaX;
    }

    virtual void OnDestroy()
    {
    }

protected:
    int ScreenWidth() const
    {
        return m_width;
    }

    int ScreenHeight() const
    {
        return m_height;
    }

    int Fps() const
    {
        return m_displayFps;
    }

    HWND WindowHandle() const
    {
        return m_window;
    }

    bool IsMouseLocked() const
    {
        return m_mouseLocked;
    }

    bool KeyDown(int key) const
    {
        return (GetAsyncKeyState(key) & 0x8000) != 0;
    }

    bool KeyPressedOnce(int key, bool& previousState)
    {
        const bool current = KeyDown(key);
        const bool pressed = current && !previousState;
        previousState = current;
        return pressed;
    }

    uint32_t Color(uint8_t r, uint8_t g, uint8_t b) const
    {
        return
            (static_cast<uint32_t>(r) << 16) |
            (static_cast<uint32_t>(g) << 8) |
             static_cast<uint32_t>(b);
    }

    uint8_t GetR(uint32_t color) const
    {
        return static_cast<uint8_t>((color >> 16) & 255);
    }

    uint8_t GetG(uint32_t color) const
    {
        return static_cast<uint8_t>((color >> 8) & 255);
    }

    uint8_t GetB(uint32_t color) const
    {
        return static_cast<uint8_t>(color & 255);
    }

    uint32_t Shade(uint32_t color, float factor) const
    {
        if (factor < 0.0f)
            factor = 0.0f;

        if (factor > 1.0f)
            factor = 1.0f;

        return Color
        (
            static_cast<uint8_t>(static_cast<float>(GetR(color)) * factor),
            static_cast<uint8_t>(static_cast<float>(GetG(color)) * factor),
            static_cast<uint8_t>(static_cast<float>(GetB(color)) * factor)
        );
    }

    void Clear(uint32_t color)
    {
        std::fill(m_pixels.begin(), m_pixels.end(), color);
    }

    void PutPixel(int x, int y, uint32_t color)
    {
        if (x < 0 || y < 0 || x >= m_width || y >= m_height)
            return;

        m_pixels[y * m_width + x] = color;
    }

    void DrawLine(int x1, int y1, int x2, int y2, uint32_t color)
    {
        int dx = std::abs(x2 - x1);
        int sx = x1 < x2 ? 1 : -1;
        int dy = -std::abs(y2 - y1);
        int sy = y1 < y2 ? 1 : -1;
        int err = dx + dy;

        while (true)
        {
            PutPixel(x1, y1, color);

            if (x1 == x2 && y1 == y2)
                break;

            const int e2 = err * 2;

            if (e2 >= dy)
            {
                err += dy;
                x1 += sx;
            }

            if (e2 <= dx)
            {
                err += dx;
                y1 += sy;
            }
        }
    }

    void DrawVerticalLine(int x, int y1, int y2, uint32_t color)
    {
        if (x < 0 || x >= m_width)
            return;

        y1 = ClampInt(y1, 0, m_height - 1);
        y2 = ClampInt(y2, 0, m_height - 1);

        for (int y = y1; y <= y2; ++y)
            m_pixels[y * m_width + x] = color;
    }

    void DrawHorizontalLine(int x1, int x2, int y, uint32_t color)
    {
        if (y < 0 || y >= m_height)
            return;

        x1 = ClampInt(x1, 0, m_width - 1);
        x2 = ClampInt(x2, 0, m_width - 1);

        uint32_t* row = m_pixels.data() + y * m_width;

        for (int x = x1; x <= x2; ++x)
            row[x] = color;
    }

    void FillRect(int x, int y, int width, int height, uint32_t color)
    {
        const int x1 = ClampInt(x, 0, m_width);
        const int y1 = ClampInt(y, 0, m_height);
        const int x2 = ClampInt(x + width, 0, m_width);
        const int y2 = ClampInt(y + height, 0, m_height);

        for (int yy = y1; yy < y2; ++yy)
        {
            uint32_t* row = m_pixels.data() + yy * m_width;

            for (int xx = x1; xx < x2; ++xx)
                row[xx] = color;
        }
    }

    void FillCircle(int cx, int cy, int radius, uint32_t color)
    {
        for (int y = -radius; y <= radius; ++y)
        {
            for (int x = -radius; x <= radius; ++x)
            {
                if (x * x + y * y <= radius * radius)
                    PutPixel(cx + x, cy + y, color);
            }
        }
    }

    uint32_t* Pixels()
    {
        return m_pixels.data();
    }

    const uint32_t* Pixels() const
    {
        return m_pixels.data();
    }

    int ClampInt(int value, int minValue, int maxValue) const
    {
        if (value < minValue)
            return minValue;

        if (value > maxValue)
            return maxValue;

        return value;
    }

    float ClampFloat(float value, float minValue, float maxValue) const
    {
        if (value < minValue)
            return minValue;

        if (value > maxValue)
            return maxValue;

        return value;
    }

    void LockMouse()
    {
        if (!m_window)
            return;

        SetForegroundWindow(m_window);
        SetFocus(m_window);
        SetCapture(m_window);

        m_mouseLocked = true;
        m_firstMouseFrame = true;

        ClipMouseToClient();
        HideCursorHard();

        const POINT center = ClientCenterScreenPoint();
        SetCursorPos(center.x, center.y);
    }

    void UnlockMouse()
    {
        m_mouseLocked = false;
        m_firstMouseFrame = true;

        ClipCursor(nullptr);
        ReleaseCapture();
        ShowCursorHard();
    }

private:
    static CustomPixelEngine* FromWindow(HWND window)
    {
        return reinterpret_cast<CustomPixelEngine*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        CustomPixelEngine* engine = FromWindow(window);

        switch (message)
        {
            case WM_CREATE:
            {
                CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
                CustomPixelEngine* createdEngine = reinterpret_cast<CustomPixelEngine*>(createStruct->lpCreateParams);

                SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createdEngine));
                return 0;
            }

            case WM_CLOSE:
            case WM_DESTROY:
            {
                if (engine)
                    engine->m_running = false;

                PostQuitMessage(0);
                return 0;
            }

            case WM_KILLFOCUS:
            {
                if (engine)
                    engine->UnlockMouse();

                return 0;
            }
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    bool CreateEngineWindow()
    {
        m_instance = GetModuleHandleW(nullptr);

        WNDCLASSW windowClass {};
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = m_instance;
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        windowClass.lpszClassName = L"CustomPixelEngineWindowClass";

        RegisterClassW(&windowClass);

        RECT windowRect {};
        windowRect.left = 0;
        windowRect.top = 0;
        windowRect.right = m_width * m_scale;
        windowRect.bottom = m_height * m_scale;

        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        m_window = CreateWindowExW
        (
            0,
            windowClass.lpszClassName,
            m_baseTitle.c_str(),
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top,
            nullptr,
            nullptr,
            m_instance,
            this
        );

        return m_window != nullptr;
    }

    void SetupFramebuffer()
    {
        m_pixels.resize(m_width * m_height);

        m_bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        m_bitmapInfo.bmiHeader.biWidth = m_width;
        m_bitmapInfo.bmiHeader.biHeight = -m_height;
        m_bitmapInfo.bmiHeader.biPlanes = 1;
        m_bitmapInfo.bmiHeader.biBitCount = 32;
        m_bitmapInfo.bmiHeader.biCompression = BI_RGB;
    }

    void Present()
    {
        HDC deviceContext = GetDC(m_window);

        StretchDIBits
        (
            deviceContext,
            0,
            0,
            m_width * m_scale,
            m_height * m_scale,
            0,
            0,
            m_width,
            m_height,
            m_pixels.data(),
            &m_bitmapInfo,
            DIB_RGB_COLORS,
            SRCCOPY
        );

        ReleaseDC(m_window, deviceContext);
    }

    void UpdateFps(float elapsedTime)
    {
        m_fpsTimer += elapsedTime;
        ++m_frameCounter;

        if (m_fpsTimer >= 0.25f)
        {
            m_displayFps = static_cast<int>(static_cast<float>(m_frameCounter) / m_fpsTimer);

            const std::wstring title =
                m_baseTitle +
                L" | FPS: " +
                std::to_wstring(m_displayFps);

            SetWindowTextW(m_window, title.c_str());

            m_fpsTimer = 0.0f;
            m_frameCounter = 0;
        }
    }

    void UpdateMouseCaptureToggle()
    {
        if (KeyPressedOnce(VK_F2, m_previousF2State))
        {
            if (m_mouseLocked)
                UnlockMouse();
            else
                LockMouse();
        }

        if (KeyPressedOnce(VK_LBUTTON, m_previousLeftMouseState))
        {
            if (!m_mouseLocked)
                LockMouse();
        }
    }

    void UpdateMouseLook()
    {
        if (!m_mouseLocked)
            return;

        if (GetForegroundWindow() != m_window)
        {
            UnlockMouse();
            return;
        }

        const POINT center = ClientCenterScreenPoint();

        POINT mouse {};
        GetCursorPos(&mouse);

        const int deltaX = mouse.x - center.x;

        if (!m_firstMouseFrame)
            OnMouseMove(static_cast<float>(deltaX));

        m_firstMouseFrame = false;

        SetCursorPos(center.x, center.y);
    }

    POINT ClientCenterScreenPoint() const
    {
        RECT clientRect {};
        GetClientRect(m_window, &clientRect);

        POINT center {};
        center.x = (clientRect.right - clientRect.left) / 2;
        center.y = (clientRect.bottom - clientRect.top) / 2;

        ClientToScreen(m_window, &center);

        return center;
    }

    void ClipMouseToClient()
    {
        RECT clientRect {};
        GetClientRect(m_window, &clientRect);

        POINT topLeft { clientRect.left, clientRect.top };
        POINT bottomRight { clientRect.right, clientRect.bottom };

        ClientToScreen(m_window, &topLeft);
        ClientToScreen(m_window, &bottomRight);

        RECT clipRect {};
        clipRect.left = topLeft.x;
        clipRect.top = topLeft.y;
        clipRect.right = bottomRight.x;
        clipRect.bottom = bottomRight.y;

        ClipCursor(&clipRect);
    }

    void HideCursorHard()
    {
        if (m_cursorHidden)
            return;

        while (ShowCursor(FALSE) >= 0)
        {
        }

        m_cursorHidden = true;
    }

    void ShowCursorHard()
    {
        if (!m_cursorHidden)
            return;

        while (ShowCursor(TRUE) < 0)
        {
        }

        m_cursorHidden = false;
    }

private:
    int m_width = 0;
    int m_height = 0;
    int m_scale = 1;

    std::wstring m_baseTitle;

    HWND m_window = nullptr;
    HINSTANCE m_instance = nullptr;
    BITMAPINFO m_bitmapInfo {};
    std::vector<uint32_t> m_pixels;

    bool m_running = true;

    bool m_mouseLocked = false;
    bool m_cursorHidden = false;
    bool m_firstMouseFrame = true;
    bool m_previousF2State = false;
    bool m_previousLeftMouseState = false;

    float m_fpsTimer = 0.0f;
    int m_frameCounter = 0;
    int m_displayFps = 0;
};
