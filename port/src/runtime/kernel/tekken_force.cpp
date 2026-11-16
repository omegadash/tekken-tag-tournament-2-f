#include "kernel_shared.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>

extern "C" PPC_FUNC(__imp__sub_823A70F0);
extern "C" PPC_FUNC(__imp__sub_82545EE0);
void TekkenForceReadMoveAxes(int& horizontal, int& depth);

namespace {
constexpr uint32_t kBattleState = 0x82EAC03Cu;
constexpr uint32_t kPracticeState = 21;
constexpr uint32_t kPracticeSettings = 0x82DD5F48u;
constexpr uint32_t kFighterTable = 0x82E88878u;
constexpr uint32_t kScene = 0x82E6B05Cu;
constexpr uint32_t kCameraManagerOffset = 724;
constexpr uint32_t kRenderCameraOffset = 384;
constexpr uint32_t kProjectionOffset = 0xA0;
constexpr float kWalkStep = 24.0f;
constexpr float kHalfWidth = 1600.0f;
constexpr float kHalfDepth = 800.0f;

struct FixedCamera
{
    bool ready = false;
    std::array<float, 3> eye{}, target{}, up{};
    float centerX = 0.0f;
    float centerZ = 0.0f;
    float rightX = 1.0f;
    float rightZ = 0.0f;
};

FixedCamera g_camera;

bool Enabled()
{
    static const bool enabled = [] {
        const char* value = std::getenv("TTT2_TEKKEN_FORCE");
        return value && std::strcmp(value, "1") == 0;
    }();
    return enabled;
}

bool InPractice(uint8_t* base)
{
    return Enabled() && GRead32(base, kBattleState) == kPracticeState;
}

uint32_t ActiveFighter(uint8_t* base, uint32_t side)
{
    if (side > 1) return 0;
    const uint32_t fighter = GRead32(base, kFighterTable + side * 4);
    if (fighter && GRead32(base, fighter + 7792) == 0)
        return GRead32(base, fighter + 172);
    return fighter;
}

uint32_t PlayerFighter(uint8_t* base)
{
    return ActiveFighter(base, GRead32(base, kPracticeSettings + 100));
}

float ReadFloat(uint8_t* base, uint32_t address)
{
    return std::bit_cast<float>(GRead32(base, address));
}

void WriteFloat(uint8_t* base, uint32_t address, float value)
{
    GWrite32(base, address, std::bit_cast<uint32_t>(value));
}

uint32_t RenderCamera(uint8_t* base)
{
    const uint32_t scene = GRead32(base, kScene);
    const uint32_t manager = scene ? GRead32(base, scene + kCameraManagerOffset) : 0;
    return manager ? GRead32(base, manager + kRenderCameraOffset) : 0;
}

void MoveFighter(uint8_t* base, uint32_t fighter)
{
    int horizontal, depth;
    TekkenForceReadMoveAxes(horizontal, depth);
    const float step = horizontal && depth ? kWalkStep * 0.70710678f : kWalkStep;
    const float x = ReadFloat(base, fighter);
    const float z = ReadFloat(base, fighter + 8);
    const float offsetX = x - g_camera.centerX, offsetZ = z - g_camera.centerZ;
    const float rightX = g_camera.rightX, rightZ = g_camera.rightZ;
    const float along = std::clamp(offsetX * rightX + offsetZ * rightZ + horizontal * step,
                                  -kHalfWidth, kHalfWidth);
    const float across = std::clamp(-offsetX * rightZ + offsetZ * rightX + depth * step,
                                   -kHalfDepth, kHalfDepth);
    const float nextX = g_camera.centerX + along * rightX - across * rightZ;
    const float nextZ = g_camera.centerZ + along * rightZ + across * rightX;
    if (std::abs(nextX - x) > 0.001f) WriteFloat(base, fighter, nextX);
    if (std::abs(nextZ - z) > 0.001f) WriteFloat(base, fighter + 8, nextZ);
}

void PrepareCamera(uint8_t* base, uint32_t camera, uint32_t player, uint32_t dummy)
{
    const float x0 = ReadFloat(base, player), x1 = ReadFloat(base, dummy);
    const float z0 = ReadFloat(base, player + 8), z1 = ReadFloat(base, dummy + 8);
    const float separation = std::hypot(x1 - x0, z1 - z0);
    const float projectionX = ReadFloat(base, camera + kProjectionOffset);
    // Default Position places the fighters 2 m apart. Wait for the stage and
    // the normal perspective projection before taking the encounter's origin.
    if (!(separation > 1950.0f && separation < 2050.0f && projectionX > 0.5f && projectionX < 5.0f)) return;
    g_camera.centerX = (x0 + x1) * 0.5f;
    g_camera.centerZ = (z0 + z1) * 0.5f;
    g_camera.rightX = (x1 - x0) / separation;
    g_camera.rightZ = (z1 - z0) / separation;
    const float floorY = (ReadFloat(base, player + 4) + ReadFloat(base, dummy + 4)) * 0.5f;
    const float fx = -g_camera.rightZ, fz = g_camera.rightX;
    constexpr float distance = 6200.0f, rise = 1300.0f;
    g_camera.target = { g_camera.centerX, floorY + 850.0f, g_camera.centerZ };
    g_camera.eye = { g_camera.centerX + fx * distance, g_camera.target[1] + rise,
                     g_camera.centerZ + fz * distance };
    g_camera.up = { 0.0f, 1.0f, 0.0f };
    g_camera.ready = true;
}

void ApplyCameraPose(uint8_t* base, uint32_t camera)
{
    for (unsigned i = 0; i < 3; ++i)
    {
        WriteFloat(base, camera + 0x10 + 4 * i, g_camera.eye[i]);
        WriteFloat(base, camera + 0x20 + 4 * i, g_camera.target[i]);
        WriteFloat(base, camera + 0x30 + 4 * i, g_camera.up[i]);
    }
}
}

// Practice initializes its settings here and then copies them to the pause menu.
PPC_FUNC(sub_823A70F0)
{
    const uint32_t settings = ctx.r3.u32;
    __imp__sub_823A70F0(ctx, base);
    if (!Enabled()) return;

    // TU3's own Battle Position selector (sub_823AD198) stores 1 for Default
    // Position and sets the position-reset bit on the Practice controller.
    constexpr uint32_t kBattlePosition = 24;
    constexpr uint32_t kPracticeFlags = 0x82DD6034u;
    constexpr uint32_t kPositionReset = 0x00200000u;
    GWrite32(base, settings + kBattlePosition, 1);
    GWrite32(base, kPracticeFlags, GRead32(base, kPracticeFlags) | kPositionReset);
    g_camera = {};
}

// Set the pose before the guest publishes its view to the stage and renderer.
PPC_FUNC(sub_82545EE0)
{
    const uint32_t camera = ctx.r3.u32;
    if (InPractice(base) && camera && camera == RenderCamera(base))
    {
        if (!g_camera.ready)
        {
            const uint32_t side = GRead32(base, kPracticeSettings + 100);
            const uint32_t player = ActiveFighter(base, side);
            const uint32_t dummy = ActiveFighter(base, side ^ 1);
            if (player && dummy) PrepareCamera(base, camera, player, dummy);
        }
        if (g_camera.ready) ApplyCameraPose(base, camera);
    }
    __imp__sub_82545EE0(ctx, base);
}

void TekkenForceFighterInput(PPCContext& ctx, uint8_t* base)
{
    if (!InPractice(base) || !g_camera.ready) return;
    // These are the fighter input readers. UI navigation and Practice reset
    // read the pad separately and keep their native buttons.
    const uint32_t lr = uint32_t(ctx.lr);
    if (lr != 0x82303A1Cu && lr != 0x823038B4u && lr != 0x82303A70u) return;
    const uint32_t fighter = lr == 0x823038B4u ? ctx.r31.u32 : ctx.r26.u32;
    if (!fighter || fighter != PlayerFighter(base)) return;
    if (lr == 0x82303A1Cu) MoveFighter(base, fighter);
    ctx.r3.u32 &= ~0x0Fu;
}
