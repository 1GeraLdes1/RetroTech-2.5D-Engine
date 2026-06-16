#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>

#pragma comment(lib, "Shell32.lib")

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

class RetroTechEngine : public olc::PixelGameEngine
{
public:
    RetroTechEngine()
    {
        sAppName = "RetroTech v0.5 - Wall Optimized 2.5D Engine";
    }

private:
    static constexpr float PI = 3.1415926535f;
    static constexpr float FOV = 70.0f * PI / 180.0f;
    static constexpr float HALF_FOV_TAN = 0.7002075f;

    static constexpr float MOVE_SPEED = 3.8f;
    static constexpr float SPRINT_SPEED = 6.2f;
    static constexpr float ROT_SPEED = 2.35f;
    static constexpr float PLAYER_RADIUS = 0.18f;
    static constexpr float MOUSE_SENSITIVITY = 0.0035f;

    static constexpr int TEXTURE_SIZE = 64;
    static constexpr int TEXTURE_MASK = TEXTURE_SIZE - 1;

    struct Vec2
    {
        float x;
        float y;
    };

    struct Player
    {
        Vec2 position;
        float angle;
    };

    struct WindowSearchData
    {
        DWORD processId;
        HWND window;
    };

    struct RayInfo
    {
        float angleOffset;
        float sinOffset;
        float cosOffset;
    };

    Player player = { { 10.5f, 10.5f }, -0.35f };

    HWND windowHandle = nullptr;

    bool mouseLocked = false;
    bool cursorHidden = false;
    bool firstMouseFrame = true;
    bool previousF2State = false;
    bool previousLeftMouseState = false;

    std::vector<RayInfo> rays;
    std::vector<olc::Pixel> skyRows;
    std::vector<olc::Pixel> floorRows;
    std::vector<olc::Pixel> wallTextureVertical;
    std::vector<olc::Pixel> wallTextureHorizontal;

    std::vector<std::string> map =
    {
        "111111111111111111111111",
        "100000000000000000000001",
        "100000000000000000000001",
        "100011111000000000111001",
        "100010001000000000100001",
        "100010001000011110100001",
        "100010001000010010100001",
        "100011111000010010111001",
        "100000000000010010000001",
        "100000000000011110000001",
        "100000000000000000000001",
        "100000000111111000000001",
        "100000000100001000111101",
        "100000000100001000100001",
        "100000000100001000100001",
        "100000000111111000100001",
        "100000000000000000111101",
        "100000000000000000000001",
        "100000111100000111100001",
        "100000100100000100100001",
        "100000111100000111100001",
        "100000000000000000000001",
        "100000000000000000000001",
        "111111111111111111111111",
    };

private:
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        WindowSearchData* data = reinterpret_cast<WindowSearchData*>(lParam);

        DWORD windowProcessId = 0;
        GetWindowThreadProcessId(hwnd, &windowProcessId);

        if (windowProcessId != data->processId)
            return TRUE;

        if (!IsWindowVisible(hwnd))
            return TRUE;

        if (GetWindow(hwnd, GW_OWNER) != nullptr)
            return TRUE;

        RECT rect;
        GetWindowRect(hwnd, &rect);

        if ((rect.right - rect.left) <= 0 || (rect.bottom - rect.top) <= 0)
            return TRUE;

        data->window = hwnd;
        return FALSE;
    }

    HWND FindOwnWindow()
    {
        if (windowHandle && IsWindow(windowHandle))
            return windowHandle;

        WindowSearchData data;
        data.processId = GetCurrentProcessId();
        data.window = nullptr;

        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));

        windowHandle = data.window;
        return windowHandle;
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

    float ClampFloat(float value, float minValue, float maxValue) const
    {
        if (value < minValue)
            return minValue;

        if (value > maxValue)
            return maxValue;

        return value;
    }

    int ClampInt(int value, int minValue, int maxValue) const
    {
        if (value < minValue)
            return minValue;

        if (value > maxValue)
            return maxValue;

        return value;
    }

    int MapWidth() const
    {
        return static_cast<int>(map[0].size());
    }

    int MapHeight() const
    {
        return static_cast<int>(map.size());
    }

    bool IsWallCell(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= MapWidth() || y >= MapHeight())
            return true;

        return map[y][x] != '0';
    }

    bool IsWallAt(float x, float y) const
    {
        const int cellX = static_cast<int>(x);
        const int cellY = static_cast<int>(y);

        return IsWallCell(cellX, cellY);
    }

    bool CanMoveTo(float x, float y) const
    {
        if (IsWallAt(x - PLAYER_RADIUS, y - PLAYER_RADIUS)) return false;
        if (IsWallAt(x + PLAYER_RADIUS, y - PLAYER_RADIUS)) return false;
        if (IsWallAt(x - PLAYER_RADIUS, y + PLAYER_RADIUS)) return false;
        if (IsWallAt(x + PLAYER_RADIUS, y + PLAYER_RADIUS)) return false;

        return true;
    }

    void BuildRayTable()
    {
        rays.clear();
        rays.resize(ScreenWidth());

        const float screenWidth = static_cast<float>(ScreenWidth());

        for (int x = 0; x < ScreenWidth(); ++x)
        {
            const float cameraX = 2.0f * static_cast<float>(x) / screenWidth - 1.0f;
            const float angleOffset = std::atan(cameraX * HALF_FOV_TAN);

            rays[x].angleOffset = angleOffset;
            rays[x].sinOffset = std::sin(angleOffset);
            rays[x].cosOffset = std::cos(angleOffset);
        }
    }

    void BuildSkyFloorTables()
    {
        skyRows.clear();
        floorRows.clear();

        skyRows.resize(ScreenHeight() / 2);
        floorRows.resize(ScreenHeight() / 2);

        const int halfH = ScreenHeight() / 2;

        for (int y = 0; y < halfH; ++y)
        {
            float t = static_cast<float>(y) / static_cast<float>(halfH);

            uint8_t r = static_cast<uint8_t>(18.0f + 24.0f * t);
            uint8_t g = static_cast<uint8_t>(22.0f + 27.0f * t);
            uint8_t b = static_cast<uint8_t>(32.0f + 38.0f * t);

            skyRows[y] = olc::Pixel(r, g, b);
        }

        for (int y = 0; y < halfH; ++y)
        {
            float t = static_cast<float>(y) / static_cast<float>(halfH);

            uint8_t r = static_cast<uint8_t>(42.0f - 16.0f * t);
            uint8_t g = static_cast<uint8_t>(38.0f - 15.0f * t);
            uint8_t b = static_cast<uint8_t>(32.0f - 12.0f * t);

            floorRows[y] = olc::Pixel(r, g, b);
        }
    }

    olc::Pixel ShadePixel(olc::Pixel base, float factor) const
    {
        factor = ClampFloat(factor, 0.0f, 1.0f);

        return olc::Pixel
        (
            static_cast<uint8_t>(static_cast<float>(base.r) * factor),
            static_cast<uint8_t>(static_cast<float>(base.g) * factor),
            static_cast<uint8_t>(static_cast<float>(base.b) * factor)
        );
    }

    void BuildWallTextures()
    {
        wallTextureVertical.resize(TEXTURE_SIZE * TEXTURE_SIZE);
        wallTextureHorizontal.resize(TEXTURE_SIZE * TEXTURE_SIZE);

        for (int y = 0; y < TEXTURE_SIZE; ++y)
        {
            for (int x = 0; x < TEXTURE_SIZE; ++x)
            {
                bool mortar = false;

                if ((y % 16) < 2)
                    mortar = true;

                const int row = y / 16;

                if ((row % 2) == 0)
                {
                    if ((x % 32) < 2)
                        mortar = true;
                }
                else
                {
                    if (((x + 16) % 32) < 2)
                        mortar = true;
                }

                olc::Pixel verticalColor;
                olc::Pixel horizontalColor;

                if (mortar)
                {
                    verticalColor = olc::Pixel(80, 80, 82);
                    horizontalColor = olc::Pixel(62, 62, 64);
                }
                else
                {
                    verticalColor = olc::Pixel(150, 150, 150);
                    horizontalColor = olc::Pixel(112, 112, 112);
                }

                wallTextureVertical[y * TEXTURE_SIZE + x] = verticalColor;
                wallTextureHorizontal[y * TEXTURE_SIZE + x] = horizontalColor;
            }
        }
    }

    POINT GetClientCenterScreenPoint() const
    {
        POINT center = { 0, 0 };

        if (!windowHandle)
            return center;

        RECT clientRect;
        GetClientRect(windowHandle, &clientRect);

        center.x = (clientRect.right - clientRect.left) / 2;
        center.y = (clientRect.bottom - clientRect.top) / 2;

        ClientToScreen(windowHandle, &center);

        return center;
    }

    void ConfineCursorToClientArea()
    {
        if (!windowHandle)
            return;

        RECT clientRect;
        GetClientRect(windowHandle, &clientRect);

        POINT topLeft = { clientRect.left, clientRect.top };
        POINT bottomRight = { clientRect.right, clientRect.bottom };

        ClientToScreen(windowHandle, &topLeft);
        ClientToScreen(windowHandle, &bottomRight);

        RECT clipRect;
        clipRect.left = topLeft.x;
        clipRect.top = topLeft.y;
        clipRect.right = bottomRight.x;
        clipRect.bottom = bottomRight.y;

        ClipCursor(&clipRect);
    }

    void HideCursorHard()
    {
        if (cursorHidden)
            return;

        while (ShowCursor(FALSE) >= 0)
        {
        }

        cursorHidden = true;
    }

    void ShowCursorHard()
    {
        if (!cursorHidden)
            return;

        while (ShowCursor(TRUE) < 0)
        {
        }

        cursorHidden = false;
    }

    void EnableMouseLock()
    {
        FindOwnWindow();

        if (!windowHandle)
            return;

        SetForegroundWindow(windowHandle);
        SetFocus(windowHandle);
        SetCapture(windowHandle);

        mouseLocked = true;
        firstMouseFrame = true;

        ConfineCursorToClientArea();
        HideCursorHard();

        POINT center = GetClientCenterScreenPoint();
        SetCursorPos(center.x, center.y);
    }

    void DisableMouseLock()
    {
        mouseLocked = false;
        firstMouseFrame = true;

        ClipCursor(nullptr);
        ReleaseCapture();
        ShowCursorHard();
    }

    void UpdateMouseLockControls()
    {
        if (KeyPressedOnce(VK_F2, previousF2State))
        {
            if (mouseLocked)
                DisableMouseLock();
            else
                EnableMouseLock();
        }

        if (KeyPressedOnce(VK_LBUTTON, previousLeftMouseState))
        {
            if (!mouseLocked)
                EnableMouseLock();
        }
    }

    void UpdateMouseLook()
    {
        if (!mouseLocked)
            return;

        if (!windowHandle || !IsWindow(windowHandle))
            FindOwnWindow();

        if (!windowHandle)
            return;

        if (GetForegroundWindow() != windowHandle)
        {
            DisableMouseLock();
            return;
        }

        const POINT center = GetClientCenterScreenPoint();

        POINT mouse;
        GetCursorPos(&mouse);

        const int deltaX = mouse.x - center.x;

        if (!firstMouseFrame)
            player.angle += static_cast<float>(deltaX) * MOUSE_SENSITIVITY;

        firstMouseFrame = false;

        SetCursorPos(center.x, center.y);
    }

    void MovePlayer(float elapsedTime)
    {
        if (KeyDown(VK_LEFT))
            player.angle -= ROT_SPEED * elapsedTime;

        if (KeyDown(VK_RIGHT))
            player.angle += ROT_SPEED * elapsedTime;

        const float sinAngle = std::sin(player.angle);
        const float cosAngle = std::cos(player.angle);

        const Vec2 forward = { cosAngle, sinAngle };
        const Vec2 side = { -sinAngle, cosAngle };

        Vec2 movement = { 0.0f, 0.0f };

        if (KeyDown('W'))
        {
            movement.x += forward.x;
            movement.y += forward.y;
        }

        if (KeyDown('S'))
        {
            movement.x -= forward.x;
            movement.y -= forward.y;
        }

        if (KeyDown('D'))
        {
            movement.x += side.x;
            movement.y += side.y;
        }

        if (KeyDown('A'))
        {
            movement.x -= side.x;
            movement.y -= side.y;
        }

        const float lengthSquared = movement.x * movement.x + movement.y * movement.y;

        if (lengthSquared > 0.0f)
        {
            const float invLength = 1.0f / std::sqrt(lengthSquared);

            movement.x *= invLength;
            movement.y *= invLength;

            const float speed = KeyDown(VK_SHIFT) ? SPRINT_SPEED : MOVE_SPEED;

            const float newX = player.position.x + movement.x * speed * elapsedTime;
            const float newY = player.position.y + movement.y * speed * elapsedTime;

            if (CanMoveTo(newX, player.position.y))
                player.position.x = newX;

            if (CanMoveTo(player.position.x, newY))
                player.position.y = newY;
        }
    }

    olc::Pixel FogPixelFast(olc::Pixel base, float fog) const
    {
        return olc::Pixel
        (
            static_cast<uint8_t>(static_cast<float>(base.r) * fog),
            static_cast<uint8_t>(static_cast<float>(base.g) * fog),
            static_cast<uint8_t>(static_cast<float>(base.b) * fog)
        );
    }

    void DrawSkyAndFloor()
    {
        const int w = ScreenWidth();
        const int halfH = ScreenHeight() / 2;

        for (int y = 0; y < halfH; ++y)
            DrawLine(0, y, w, y, skyRows[y]);

        for (int y = 0; y < halfH; ++y)
            DrawLine(0, halfH + y, w, halfH + y, floorRows[y]);
    }

    void DrawWalls()
    {
        const int screenW = ScreenWidth();
        const int screenH = ScreenHeight();

        const float sinPlayer = std::sin(player.angle);
        const float cosPlayer = std::cos(player.angle);

        const int playerMapX = static_cast<int>(player.position.x);
        const int playerMapY = static_cast<int>(player.position.y);

        for (int x = 0; x < screenW; ++x)
        {
            const RayInfo& ray = rays[x];

            const float rayDirX = cosPlayer * ray.cosOffset - sinPlayer * ray.sinOffset;
            const float rayDirY = sinPlayer * ray.cosOffset + cosPlayer * ray.sinOffset;

            int mapX = playerMapX;
            int mapY = playerMapY;

            const float safeRayDirX = rayDirX == 0.0f ? 0.000001f : rayDirX;
            const float safeRayDirY = rayDirY == 0.0f ? 0.000001f : rayDirY;

            const float deltaDistX = std::abs(1.0f / safeRayDirX);
            const float deltaDistY = std::abs(1.0f / safeRayDirY);

            int stepX;
            int stepY;

            float sideDistX;
            float sideDistY;

            if (rayDirX < 0.0f)
            {
                stepX = -1;
                sideDistX = (player.position.x - static_cast<float>(mapX)) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (static_cast<float>(mapX) + 1.0f - player.position.x) * deltaDistX;
            }

            if (rayDirY < 0.0f)
            {
                stepY = -1;
                sideDistY = (player.position.y - static_cast<float>(mapY)) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (static_cast<float>(mapY) + 1.0f - player.position.y) * deltaDistY;
            }

            bool verticalSide;

            while (true)
            {
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    verticalSide = true;
                }
                else
                {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    verticalSide = false;
                }

                if (IsWallCell(mapX, mapY))
                    break;
            }

            float rawDistance;

            if (verticalSide)
            {
                rawDistance =
                    (static_cast<float>(mapX) - player.position.x + static_cast<float>(1 - stepX) * 0.5f)
                    / safeRayDirX;
            }
            else
            {
                rawDistance =
                    (static_cast<float>(mapY) - player.position.y + static_cast<float>(1 - stepY) * 0.5f)
                    / safeRayDirY;
            }

            float correctedDistance = rawDistance * ray.cosOffset;

            if (correctedDistance < 0.0001f)
                correctedDistance = 0.0001f;

            const float hitX = player.position.x + rayDirX * rawDistance;
            const float hitY = player.position.y + rayDirY * rawDistance;

            float wallX = verticalSide
                ? hitY - static_cast<int>(hitY)
                : hitX - static_cast<int>(hitX);

            if (wallX < 0.0f)
                wallX += 1.0f;

            const int texX = static_cast<int>(wallX * static_cast<float>(TEXTURE_SIZE)) & TEXTURE_MASK;

            const int lineHeight = static_cast<int>(static_cast<float>(screenH) / correctedDistance);
            const int wallTopUnclamped = -lineHeight / 2 + screenH / 2;

            int drawStart = wallTopUnclamped;
            int drawEnd = lineHeight / 2 + screenH / 2;

            drawStart = ClampInt(drawStart, 0, screenH - 1);
            drawEnd = ClampInt(drawEnd, 0, screenH - 1);

            if (drawStart > drawEnd)
                continue;

            const float texStep = static_cast<float>(TEXTURE_SIZE) / static_cast<float>(lineHeight);
            float texPosition = static_cast<float>(drawStart - wallTopUnclamped) * texStep;

            const float fog = ClampFloat(1.0f / (1.0f + correctedDistance * 0.065f), 0.22f, 1.0f);

            const std::vector<olc::Pixel>& texture = verticalSide
                ? wallTextureVertical
                : wallTextureHorizontal;

            for (int y = drawStart; y <= drawEnd; ++y)
            {
                const int texY = static_cast<int>(texPosition) & TEXTURE_MASK;
                texPosition += texStep;

                const olc::Pixel base = texture[texY * TEXTURE_SIZE + texX];
                Draw(x, y, FogPixelFast(base, fog));
            }
        }
    }

    void DrawCrosshair()
    {
        const int cx = ScreenWidth() / 2;
        const int cy = ScreenHeight() / 2;

        const olc::Pixel c = olc::Pixel(210, 210, 210);

        DrawLine(cx - 5, cy, cx - 2, cy, c);
        DrawLine(cx + 2, cy, cx + 5, cy, c);
        DrawLine(cx, cy - 5, cx, cy - 2, c);
        DrawLine(cx, cy + 2, cx, cy + 5, c);
    }

    void DrawMiniMap()
    {
        const int cell = 4;
        const int offsetX = 10;
        const int offsetY = 10;

        FillRect(offsetX - 4, offsetY - 4, MapWidth() * cell + 8, MapHeight() * cell + 8, olc::Pixel(5, 5, 5));

        for (int y = 0; y < MapHeight(); ++y)
        {
            for (int x = 0; x < MapWidth(); ++x)
            {
                const olc::Pixel color = IsWallCell(x, y)
                    ? olc::Pixel(155, 155, 155)
                    : olc::Pixel(25, 25, 25);

                FillRect(offsetX + x * cell, offsetY + y * cell, cell - 1, cell - 1, color);
            }
        }

        const int px = offsetX + static_cast<int>(player.position.x * static_cast<float>(cell));
        const int py = offsetY + static_cast<int>(player.position.y * static_cast<float>(cell));

        FillCircle(px, py, 2, olc::Pixel(255, 40, 40));

        const int lx = px + static_cast<int>(std::cos(player.angle) * 12.0f);
        const int ly = py + static_cast<int>(std::sin(player.angle) * 12.0f);

        DrawLine(px, py, lx, ly, olc::Pixel(255, 40, 40));
    }

    void DrawHelp()
    {
        FillRect(8, 8, 405, 64, olc::Pixel(0, 0, 0));
        DrawString(14, 14, "RetroTech v0.5 wall optimized", olc::Pixel(255, 255, 255));
        DrawString(14, 26, "Wall columns use precomputed ray sin/cos", olc::Pixel(230, 230, 80));
        DrawString(14, 38, "Texture Y uses step, fog once per column", olc::Pixel(230, 230, 80));
        DrawString(14, 50, "F2 mouse lock | TAB map | F1 help | ESC exit", olc::Pixel(230, 230, 80));
    }

    void DrawMouseStatus()
    {
        if (mouseLocked)
            return;

        FillRect(ScreenWidth() / 2 - 115, ScreenHeight() / 2 - 10, 230, 20, olc::Pixel(0, 0, 0));
        DrawString(ScreenWidth() / 2 - 105, ScreenHeight() / 2 - 4, "CLICK TO CAPTURE MOUSE", olc::Pixel(255, 255, 0));
    }

public:
    bool OnUserCreate() override
    {
        FindOwnWindow();

        BuildRayTable();
        BuildSkyFloorTables();
        BuildWallTextures();

        return true;
    }

    bool OnUserUpdate(float elapsedTime) override
    {
        if (KeyDown(VK_ESCAPE))
        {
            DisableMouseLock();
            return false;
        }

        UpdateMouseLockControls();
        UpdateMouseLook();
        MovePlayer(elapsedTime);

        DrawSkyAndFloor();
        DrawWalls();
        DrawCrosshair();

        if (KeyDown(VK_TAB))
            DrawMiniMap();

        if (KeyDown(VK_F1))
            DrawHelp();

        DrawMouseStatus();

        return true;
    }

    bool OnUserDestroy() override
    {
        DisableMouseLock();
        return true;
    }
};

int main()
{
    RetroTechEngine engine;

    if (engine.Construct(640, 360, 2, 2))
        engine.Start();

    return 0;
}
