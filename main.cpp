#include "CustomPixelEngine.h"

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

class RetroTech : public CustomPixelEngine
{
public:
    RetroTech()
        : CustomPixelEngine(640, 360, 2, L"RetroTech v0.7 - CustomPixelEngine")
    {
    }

private:
    static constexpr float FOV_TAN_HALF = 0.7002075f;

    static constexpr float MOVE_SPEED = 3.8f;
    static constexpr float RUN_SPEED = 6.2f;
    static constexpr float TURN_SPEED = 2.35f;
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

    struct Ray
    {
        float sinOffset;
        float cosOffset;
    };

    Player player = { { 10.5f, 10.5f }, -0.35f };

    std::vector<Ray> rays;
    std::vector<uint32_t> skyRows;
    std::vector<uint32_t> floorRows;
    std::vector<uint32_t> wallTextureVertical;
    std::vector<uint32_t> wallTextureHorizontal;

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
        return IsWallCell(static_cast<int>(x), static_cast<int>(y));
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
        rays.resize(ScreenWidth());

        const float screenWidth = static_cast<float>(ScreenWidth());

        for (int x = 0; x < ScreenWidth(); ++x)
        {
            const float cameraX = 2.0f * static_cast<float>(x) / screenWidth - 1.0f;
            const float angleOffset = std::atan(cameraX * FOV_TAN_HALF);

            rays[x].sinOffset = std::sin(angleOffset);
            rays[x].cosOffset = std::cos(angleOffset);
        }
    }

    void BuildSkyFloorTables()
    {
        const int halfHeight = ScreenHeight() / 2;

        skyRows.resize(halfHeight);
        floorRows.resize(halfHeight);

        for (int y = 0; y < halfHeight; ++y)
        {
            const float t = static_cast<float>(y) / static_cast<float>(halfHeight);

            skyRows[y] = Color
            (
                static_cast<uint8_t>(18.0f + 24.0f * t),
                static_cast<uint8_t>(22.0f + 27.0f * t),
                static_cast<uint8_t>(32.0f + 38.0f * t)
            );

            floorRows[y] = Color
            (
                static_cast<uint8_t>(42.0f - 16.0f * t),
                static_cast<uint8_t>(38.0f - 15.0f * t),
                static_cast<uint8_t>(32.0f - 12.0f * t)
            );
        }
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

                if (mortar)
                {
                    wallTextureVertical[y * TEXTURE_SIZE + x] = Color(80, 80, 82);
                    wallTextureHorizontal[y * TEXTURE_SIZE + x] = Color(62, 62, 64);
                }
                else
                {
                    wallTextureVertical[y * TEXTURE_SIZE + x] = Color(150, 150, 150);
                    wallTextureHorizontal[y * TEXTURE_SIZE + x] = Color(112, 112, 112);
                }
            }
        }
    }

    void MovePlayer(float elapsedTime)
    {
        if (KeyDown(VK_LEFT))
            player.angle -= TURN_SPEED * elapsedTime;

        if (KeyDown(VK_RIGHT))
            player.angle += TURN_SPEED * elapsedTime;

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

            const float speed = KeyDown(VK_SHIFT) ? RUN_SPEED : MOVE_SPEED;

            const float newX = player.position.x + movement.x * speed * elapsedTime;
            const float newY = player.position.y + movement.y * speed * elapsedTime;

            if (CanMoveTo(newX, player.position.y))
                player.position.x = newX;

            if (CanMoveTo(player.position.x, newY))
                player.position.y = newY;
        }
    }

    void DrawSkyAndFloor()
    {
        const int halfHeight = ScreenHeight() / 2;

        for (int y = 0; y < halfHeight; ++y)
            DrawHorizontalLine(0, ScreenWidth() - 1, y, skyRows[y]);

        for (int y = 0; y < halfHeight; ++y)
            DrawHorizontalLine(0, ScreenWidth() - 1, halfHeight + y, floorRows[y]);
    }

    uint32_t FogPixel(uint32_t color, float fog) const
    {
        return Color
        (
            static_cast<uint8_t>(static_cast<float>(GetR(color)) * fog),
            static_cast<uint8_t>(static_cast<float>(GetG(color)) * fog),
            static_cast<uint8_t>(static_cast<float>(GetB(color)) * fog)
        );
    }

    void DrawWalls()
    {
        const float sinPlayer = std::sin(player.angle);
        const float cosPlayer = std::cos(player.angle);

        const int playerMapX = static_cast<int>(player.position.x);
        const int playerMapY = static_cast<int>(player.position.y);

        for (int x = 0; x < ScreenWidth(); ++x)
        {
            const Ray& ray = rays[x];

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

            const int lineHeight = static_cast<int>(static_cast<float>(ScreenHeight()) / correctedDistance);
            const int wallTop = -lineHeight / 2 + ScreenHeight() / 2;

            int drawStart = ClampInt(wallTop, 0, ScreenHeight() - 1);
            int drawEnd = ClampInt(lineHeight / 2 + ScreenHeight() / 2, 0, ScreenHeight() - 1);

            if (drawStart > drawEnd)
                continue;

            const float texStep = static_cast<float>(TEXTURE_SIZE) / static_cast<float>(lineHeight);
            float texPosition = static_cast<float>(drawStart - wallTop) * texStep;

            const float fog = ClampFloat(1.0f / (1.0f + correctedDistance * 0.065f), 0.22f, 1.0f);

            const std::vector<uint32_t>& texture = verticalSide
                ? wallTextureVertical
                : wallTextureHorizontal;

            for (int y = drawStart; y <= drawEnd; ++y)
            {
                const int texY = static_cast<int>(texPosition) & TEXTURE_MASK;
                texPosition += texStep;

                const uint32_t baseColor = texture[texY * TEXTURE_SIZE + texX];
                Pixels()[y * ScreenWidth() + x] = FogPixel(baseColor, fog);
            }
        }
    }

    void DrawCrosshair()
    {
        const int cx = ScreenWidth() / 2;
        const int cy = ScreenHeight() / 2;

        const uint32_t color = Color(210, 210, 210);

        DrawLine(cx - 5, cy, cx - 2, cy, color);
        DrawLine(cx + 2, cy, cx + 5, cy, color);
        DrawLine(cx, cy - 5, cx, cy - 2, color);
        DrawLine(cx, cy + 2, cx, cy + 5, color);
    }

    void DrawMinimap()
    {
        const int cell = 4;
        const int offsetX = 10;
        const int offsetY = 10;

        FillRect(offsetX - 4, offsetY - 4, MapWidth() * cell + 8, MapHeight() * cell + 8, Color(5, 5, 5));

        for (int y = 0; y < MapHeight(); ++y)
        {
            for (int x = 0; x < MapWidth(); ++x)
            {
                const uint32_t color = IsWallCell(x, y)
                    ? Color(155, 155, 155)
                    : Color(25, 25, 25);

                FillRect(offsetX + x * cell, offsetY + y * cell, cell - 1, cell - 1, color);
            }
        }

        const int px = offsetX + static_cast<int>(player.position.x * static_cast<float>(cell));
        const int py = offsetY + static_cast<int>(player.position.y * static_cast<float>(cell));

        FillCircle(px, py, 2, Color(255, 40, 40));

        const int lx = px + static_cast<int>(std::cos(player.angle) * 12.0f);
        const int ly = py + static_cast<int>(std::sin(player.angle) * 12.0f);

        DrawLine(px, py, lx, ly, Color(255, 40, 40));
    }

    void DrawMouseStatus()
    {
        if (IsMouseLocked())
            return;

        FillRect(ScreenWidth() / 2 - 80, ScreenHeight() / 2 - 5, 160, 10, Color(0, 0, 0));
    }

protected:
    bool OnCreate() override
    {
        BuildRayTable();
        BuildSkyFloorTables();
        BuildWallTextures();

        return true;
    }

    void OnMouseMove(float mouseDeltaX) override
    {
        player.angle += mouseDeltaX * MOUSE_SENSITIVITY;
    }

    bool OnUpdate(float elapsedTime) override
    {
        if (KeyDown(VK_ESCAPE))
        {
            UnlockMouse();
            return false;
        }

        MovePlayer(elapsedTime);

        DrawSkyAndFloor();
        DrawWalls();
        DrawCrosshair();

        if (KeyDown(VK_TAB))
            DrawMinimap();

        DrawMouseStatus();

        return true;
    }
};

int RunRetroTech()
{
    RetroTech game;
    return game.Run();
}

int main()
{
    return RunRetroTech();
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return RunRetroTech();
}
