#include "kernel_shared.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <numbers>

extern "C" PPC_FUNC(__imp__sub_823A70F0);
extern "C" PPC_FUNC(__imp__sub_823A9700);
extern "C" PPC_FUNC(__imp__sub_823A9778);
extern "C" PPC_FUNC(__imp__sub_823A9D88);
extern "C" PPC_FUNC(__imp__sub_823AC118);
extern "C" PPC_FUNC(__imp__sub_824D0D58);
extern "C" PPC_FUNC(__imp__sub_824D0D78);
extern "C" PPC_FUNC(__imp__sub_82545EE0);
extern "C" PPC_FUNC(__imp__sub_822CDA88);
extern "C" PPC_FUNC(__imp__sub_822D4B68);
extern "C" PPC_FUNC(__imp__sub_822E36C0);
extern "C" PPC_FUNC(__imp__sub_822E3E18);
extern "C" PPC_FUNC(__imp__sub_8231DA80);
extern "C" PPC_FUNC(__imp__sub_82320AC8);
extern "C" PPC_FUNC(__imp__sub_82396828);
extern "C" PPC_FUNC(__imp__sub_825B75B0);
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
constexpr uint32_t kLife = 96; // Fighter's current life in 16.16 fixed point.
constexpr uint32_t kOpponent = 152;
constexpr uint32_t kPartner = 172;
constexpr uint32_t kOnStage = 996;
constexpr uint32_t kSolo = 1012;
constexpr uint32_t kOnPoint = 7792;
constexpr uint32_t kPosture = 875;
constexpr uint8_t kStanding = 3;
constexpr uint8_t kFirstDownedPosture = 12;
constexpr float kWalkStep = 24.0f;
constexpr float kHalfWidth = 1600.0f;
constexpr float kHalfDepth = 800.0f;
constexpr float kAttackDepth = 320.0f;
constexpr float kBodyContactDistance = 420.0f;
constexpr float kEnemyStep = 14.0f;
constexpr float kEnemyStrikeRange = 700.0f;
constexpr float kEnemyApproachGap = 645.0f;
constexpr float kEnemyAlignDepth = 115.0f;
constexpr unsigned kEnemyStartDelay = 45;
constexpr unsigned kEnemyWindup = 18;
constexpr unsigned kEnemyRecovery = 54;
constexpr uint32_t kEnemyStrike = 0x4000u; // TU3's decoded fighter input for Xbox X.
constexpr uint32_t kAttackButtons = 0xF000u;
constexpr uint32_t kDirectionButtons = 0x0Fu;
constexpr uint32_t kWalkLeft = 0x04u, kWalkRight = 0x08u;

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

enum class EnemyPhase { Approach, Windup, Recover };

struct EnemyController
{
    EnemyPhase phase = EnemyPhase::Recover;
    unsigned framesLeft = kEnemyStartDelay;
    bool strikePending = false;
    bool walking = false;
};

std::array<EnemyController, 2> g_enemies;
bool g_spawnPartner = true;

struct FloorMotion
{
    uint32_t fighter = 0;
    float x = 0.0f, z = 0.0f;
    bool walking = false;
};

std::array<FloorMotion, 3> g_motion;

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
    if (fighter && GRead32(base, fighter + kOnPoint) == 0)
        return GRead32(base, fighter + kPartner);
    return fighter;
}

uint32_t PlayerFighter(uint8_t* base)
{
    return ActiveFighter(base, GRead32(base, kPracticeSettings + 100));
}

uint32_t OpponentSide(uint8_t* base)
{
    return GRead32(base, kPracticeSettings + 100) ^ 1;
}

uint32_t EnemyFighter(uint8_t* base)
{
    return ActiveFighter(base, OpponentSide(base));
}

uint32_t EnemyPartner(uint8_t* base)
{
    const uint32_t side = OpponentSide(base);
    if (side > 1) return 0;
    const uint32_t lead = GRead32(base, kFighterTable + side * 4);
    // TU3 still allocates a reserve actor for a Solo side.
    return lead && !GRead32(base, lead + kSolo) ? GRead32(base, lead + kPartner) : 0;
}

std::array<uint32_t, 2> Enemies(uint8_t* base)
{
    const uint32_t second = EnemyPartner(base);
    if (second) return {GRead32(base, kFighterTable + OpponentSide(base) * 4), second};
    return {EnemyFighter(base), 0};
}

bool IsEnemy(uint8_t* base, uint32_t actor)
{
    if (!actor) return false;
    const auto enemies = Enemies(base);
    return actor == enemies[0] || actor == enemies[1];
}

void ResetControllers()
{
    g_enemies = {};
    g_enemies[1].framesLeft += 24;
    g_spawnPartner = true;
    g_motion = {};
}

bool Defeated(uint8_t* base, uint32_t fighter)
{
    return GRead32(base, fighter + kLife) == 0;
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

float FloorDepth(float x, float z)
{
    return -x * g_camera.rightZ + z * g_camera.rightX;
}

uint32_t WalkDirection(float along)
{
    return along < 0.0f ? kWalkLeft : kWalkRight;
}

bool NativeWalk(uint8_t* base, uint32_t fighter)
{
    const uint32_t moveset = GRead32(base, fighter + 136);
    if (!moveset) return false;
    constexpr uint32_t kMoveSize = 112;
    // TU3 resolves common stance 0x8001 through the character's move table.
    // Its forward and backward walk entries follow that stance in the table.
    const uint32_t neutral = GRead32(base, moveset + 416) + kMoveSize * GRead16(base, moveset + 138);
    const uint32_t move = GRead32(base, fighter + 280);
    return move > neutral && move <= neutral + 10 * kMoveSize;
}

bool CanWalk(uint8_t* base, uint32_t fighter)
{
    const uint8_t posture = GRead8(base, fighter + kPosture);
    return posture == kStanding || (posture < kFirstDownedPosture && NativeWalk(base, fighter));
}

float AlongBetween(uint8_t* base, uint32_t from, uint32_t to)
{
    return (ReadFloat(base, to) - ReadFloat(base, from)) * g_camera.rightX +
           (ReadFloat(base, to + 8) - ReadFloat(base, from + 8)) * g_camera.rightZ;
}

void FaceForStrike(uint8_t* base, uint32_t fighter, uint32_t target)
{
    const float x = ReadFloat(base, target) - ReadFloat(base, fighter);
    const float z = ReadFloat(base, target + 8) - ReadFloat(base, fighter + 8);
    if (std::abs(x) + std::abs(z) < 1.0f) return;
    const float radians = std::atan2(x, z);
    const auto angle = static_cast<uint16_t>(std::lround(radians * (32768.0f / std::numbers::pi_v<float>)));
    GWrite16(base, fighter + 14, angle);
    GWrite16(base, fighter + 190, angle);
    WriteFloat(base, fighter + 32, radians);
    WriteFloat(base, fighter + 184, radians);
}

bool EncounterPair(uint8_t* base, uint32_t first, uint32_t second)
{
    if (!InPractice(base) || !g_camera.ready || !first || !second) return false;
    const uint32_t player = PlayerFighter(base);
    return (first == player && IsEnemy(base, second)) || (second == player && IsEnemy(base, first));
}

bool RejectHit(uint8_t* base, uint32_t attacker, uint32_t defender)
{
    if (InPractice(base) && g_camera.ready && attacker != defender &&
        IsEnemy(base, attacker) && IsEnemy(base, defender)) return true;
    if (!EncounterPair(base, attacker, defender)) return false;
    const float depth = FloorDepth(ReadFloat(base, attacker) - ReadFloat(base, defender),
                                   ReadFloat(base, attacker + 8) - ReadFloat(base, defender + 8));
    return Defeated(base, attacker) || Defeated(base, defender) || std::abs(depth) > kAttackDepth;
}

uint32_t AttackTarget(uint8_t* base, uint32_t player, int horizontal, int depth)
{
    uint32_t target = 0;
    float bestScore = 0.0f;
    const float inputLength = std::hypot(float(horizontal), float(depth));
    for (const uint32_t enemy : Enemies(base))
    {
        if (!enemy || Defeated(base, enemy)) continue;
        const float x = ReadFloat(base, enemy) - ReadFloat(base, player);
        const float z = ReadFloat(base, enemy + 8) - ReadFloat(base, player + 8);
        const float across = FloorDepth(x, z);
        const float distance = std::hypot(x, z);
        float score = distance + std::max(0.0f, std::abs(across) - kAttackDepth) * 4.0f;
        if (inputLength > 0.0f && distance > 1.0f)
        {
            const float along = x * g_camera.rightX + z * g_camera.rightZ;
            const float alignment = (along * horizontal + across * depth) / (distance * inputLength);
            score += (1.0f - alignment) * 650.0f;
        }
        if (!target || score < bestScore)
        {
            target = enemy;
            bestScore = score;
        }
    }
    return target;
}

void SetFloorPosition(PPCContext& ctx, uint8_t* base, uint32_t fighter, float x, float z)
{
    // Keep the logical position and the pose origin used by the hit volumes in sync.
    PPCContext query = ctx;
    query.r3.u32 = fighter;
    __imp__sub_82396828(query, base);
    const uint32_t pose = query.r3.u32;
    WriteFloat(base, fighter, x);
    WriteFloat(base, fighter + 8, z);
    WriteFloat(base, fighter + 20, x);
    WriteFloat(base, fighter + 28, z);
    if (pose)
    {
        WriteFloat(base, pose + 48, x);
        WriteFloat(base, pose + 56, z);
    }
}

void MoveOnFloor(PPCContext& ctx, uint8_t* base, uint32_t fighter, float alongStep, float depthStep)
{
    const float x = ReadFloat(base, fighter);
    const float z = ReadFloat(base, fighter + 8);
    const float offsetX = x - g_camera.centerX, offsetZ = z - g_camera.centerZ;
    const float rightX = g_camera.rightX, rightZ = g_camera.rightZ;
    const float along = std::clamp(offsetX * rightX + offsetZ * rightZ + alongStep,
                                  -kHalfWidth, kHalfWidth);
    const float across = std::clamp(FloorDepth(offsetX, offsetZ) + depthStep, -kHalfDepth, kHalfDepth);
    const float nextX = g_camera.centerX + along * rightX - across * rightZ;
    const float nextZ = g_camera.centerZ + along * rightZ + across * rightX;
    if (std::abs(nextX - x) > 0.001f || std::abs(nextZ - z) > 0.001f)
        SetFloorPosition(ctx, base, fighter, nextX, nextZ);
}

void ReconcileWalk(PPCContext& ctx, uint8_t* base, uint32_t fighter, FloorMotion& motion)
{
    const bool walking = NativeWalk(base, fighter);
    const bool walkEnded = motion.walking && GRead8(base, fighter + kPosture) == kStanding;
    if (motion.fighter == fighter && (walking || walkEnded))
        SetFloorPosition(ctx, base, fighter, motion.x, motion.z);
    motion.walking = walking;
}

void MovePlayer(PPCContext& ctx, uint8_t* base, uint32_t fighter)
{
    int horizontal, depth;
    TekkenForceReadMoveAxes(horizontal, depth);
    const float step = horizontal && depth ? kWalkStep * 0.70710678f : kWalkStep;
    MoveOnFloor(ctx, base, fighter, horizontal * step, depth * step);
}

void UpdateEnemy(PPCContext& ctx, uint8_t* base, uint32_t enemy, uint32_t player, EnemyController& controller)
{
    controller.walking = false;
    if (Defeated(base, enemy) || Defeated(base, player))
    {
        controller.strikePending = false;
        return;
    }

    if (!CanWalk(base, enemy) || GRead8(base, player + kPosture) >= kFirstDownedPosture)
    {
        controller.strikePending = false;
        if (controller.phase == EnemyPhase::Windup) controller = { EnemyPhase::Recover, kEnemyRecovery, false };
        return;
    }

    if (controller.framesLeft && --controller.framesLeft) return;
    if (controller.phase == EnemyPhase::Windup)
    {
        // Commit to this strike. The player can step clear while it starts up.
        controller = { EnemyPhase::Recover, kEnemyRecovery, true };
        return;
    }

    controller.phase = EnemyPhase::Approach;
    const float x = ReadFloat(base, player) - ReadFloat(base, enemy);
    const float z = ReadFloat(base, player + 8) - ReadFloat(base, enemy + 8);
    const float along = x * g_camera.rightX + z * g_camera.rightZ;
    const float depth = FloorDepth(x, z);
    if (std::abs(along) <= kEnemyStrikeRange && std::abs(depth) <= kEnemyAlignDepth)
    {
        controller = { EnemyPhase::Windup, kEnemyWindup, false };
        return;
    }

    const float forward = along - std::clamp(along, -kEnemyApproachGap, kEnemyApproachGap);
    const float distance = std::hypot(forward, depth);
    const float scale = std::min(1.0f, kEnemyStep / distance);
    controller.walking = true;
    MoveOnFloor(ctx, base, enemy, forward * scale, depth * scale);
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

    // Use the Freestyle input path and its native nonlethal ground recovery.
    // Practice can otherwise load action settings from the previous session.
    GWrite32(base, settings, 0);      // Freestyle
    GWrite32(base, settings + 4, 0);  // Action Settings 1: Stand
    GWrite32(base, settings + 8, 0);  // Action Settings 2: Off
    GWrite32(base, settings + 12, 5); // Action on the Floor: Stand Up

    // TU3's own Battle Position selector (sub_823AD198) stores 1 for Default
    // Position and sets the position-reset bit on the Practice controller.
    constexpr uint32_t kBattlePosition = 24;
    constexpr uint32_t kPracticeFlags = 0x82DD6034u;
    constexpr uint32_t kPositionReset = 0x00200000u;
    GWrite32(base, settings + kBattlePosition, 1);
    GWrite32(base, kPracticeFlags, GRead32(base, kPracticeFlags) | kPositionReset);
    g_camera = {};
    ResetControllers();
}

// Disable the two Practice auto-refills. The position reset has its own refill.
PPC_FUNC(sub_823A9700)
{
    if (!InPractice(base)) __imp__sub_823A9700(ctx, base);
}

PPC_FUNC(sub_823A9778)
{
    if (!InPractice(base)) __imp__sub_823A9778(ctx, base);
}

PPC_FUNC(sub_824D0D58)
{
    if (ctx.lr == 0x8231D1D0u && InPractice(base))
    {
        const uint32_t player = PlayerFighter(base);
        if (player && ctx.r3.u32 == player + kLife - 4)
        {
            ResetControllers();
        }
    }
    __imp__sub_824D0D58(ctx, base);
}

// Solo-versus-Tag can also restore recoverable health independently of Practice.
PPC_FUNC(sub_824D0D78)
{
    const uint32_t fighter = ctx.r3.u32 - (kLife - 4);
    if (InPractice(base) && (fighter == PlayerFighter(base) || IsEnemy(base, fighter))) return;
    __imp__sub_824D0D78(ctx, base);
}

// The duel's body separation also pushes fighters at attack range. In the
// Force encounter let it resolve actual close contact without moving the
// player simply because the enemy approached.
PPC_FUNC(sub_822D4B68)
{
    const uint32_t first = ctx.r3.u32;
    const uint32_t second = ctx.r4.u32;
    if (EncounterPair(base, first, second))
    {
        const float x = ReadFloat(base, first) - ReadFloat(base, second);
        const float z = ReadFloat(base, first + 8) - ReadFloat(base, second + 8);
        if (x * x + z * z > kBodyContactDistance * kBodyContactDistance) return;
    }
    __imp__sub_822D4B68(ctx, base);
}

// Both guest collision paths queue hit reactions and damage for this pair.
PPC_FUNC(sub_822E36C0)
{
    if (!RejectHit(base, ctx.r3.u32, ctx.r4.u32)) __imp__sub_822E36C0(ctx, base);
}

PPC_FUNC(sub_822E3E18)
{
    if (!RejectHit(base, ctx.r3.u32, ctx.r4.u32)) __imp__sub_822E3E18(ctx, base);
}

// Resolve custom movement before the guest prepares this frame's hit volumes.
PPC_FUNC(sub_82320AC8)
{
    if (InPractice(base) && g_camera.ready)
    {
        const uint32_t player = PlayerFighter(base);
        const auto enemies = Enemies(base);
        if (player && enemies[0])
        {
            ReconcileWalk(ctx, base, player, g_motion[0]);
            if (!Defeated(base, player) && CanWalk(base, player)) MovePlayer(ctx, base, player);
            if (g_spawnPartner && enemies[1])
            {
                const uint32_t partner = enemies[1];
                // Reuse the tag partner's loaded model and moveset. Activate it
                // after the native reset has put the roster at its start positions.
                const uint32_t playerContact = GRead32(base, player + 156);
                GWrite32(base, partner + kOnPoint, 1);
                PPCContext spawn = ctx;
                spawn.r3.u32 = partner;
                spawn.r4.u32 = player;
                spawn.r5.u32 = 7; // Practice position reset.
                spawn.r6.u32 = 0; // Use the standing move.
                __imp__sub_8231DA80(spawn, base);
                GWrite32(base, player + 156, playerContact);
                GWrite32(base, partner + kOnStage, 1);
                constexpr float along = 920.0f, depth = -620.0f;
                SetFloorPosition(ctx, base, partner,
                    g_camera.centerX + along * g_camera.rightX - depth * g_camera.rightZ,
                    g_camera.centerZ + along * g_camera.rightZ + depth * g_camera.rightX);
                FaceForStrike(base, partner, player);
                g_spawnPartner = false;
            }
            for (unsigned i = 0; i < enemies.size(); ++i)
            {
                const uint32_t enemy = enemies[i];
                if (!enemy) continue;
                ReconcileWalk(ctx, base, enemy, g_motion[i + 1]);
                MoveOnFloor(ctx, base, enemy, 0.0f, 0.0f);
                UpdateEnemy(ctx, base, enemy, player, g_enemies[i]);
            }
        }
    }
    __imp__sub_82320AC8(ctx, base);
}

PPC_FUNC(sub_822CDA88)
{
    __imp__sub_822CDA88(ctx, base);
    if (!InPractice(base) || !g_camera.ready) return;
    const auto enemies = Enemies(base);
    const uint32_t actors[] = { PlayerFighter(base), enemies[0], enemies[1] };
    for (unsigned i = 0; i < g_motion.size(); ++i)
    {
        const uint32_t fighter = actors[i];
        if (!fighter) continue;
        g_motion[i].fighter = fighter;
        g_motion[i].x = ReadFloat(base, fighter);
        g_motion[i].z = ReadFloat(base, fighter + 8);
    }
}

// Allow the native KO flags and KO reaction for both fighters. Leave Practice's
// round flow in place so the player can still reset the encounter.
PPC_FUNC(sub_825B75B0)
{
    const uint32_t lr = uint32_t(ctx.lr);
    if ((lr == 0x822E8020u || lr == 0x822E2DACu) && InPractice(base) &&
        (IsEnemy(base, ctx.r31.u32) || ctx.r31.u32 == PlayerFighter(base)))
    {
        ctx.r3.u32 = 0;
        return;
    }
    __imp__sub_825B75B0(ctx, base);
}

// The Practice controller selects its existing "Off" floor action until the
// position reset restores the defeated fighter's life.
PPC_FUNC(sub_823A9D88)
{
    if (ctx.lr == 0x82304958u && InPractice(base))
    {
        const uint32_t enemy = ctx.r31.u32;
        if (IsEnemy(base, enemy) && Defeated(base, enemy))
        {
            ctx.r3.u64 = 13;
            return;
        }
    }
    __imp__sub_823A9D88(ctx, base);
}

// Practice supplies held and newly pressed buttons for the CPU fighter here.
// Keep its standing and get-up logic, and issue one jab after windup.
PPC_FUNC(sub_823AC118)
{
    const uint32_t fighter = ctx.r4.u32, held = ctx.r5.u32, pressed = ctx.r6.u32;
    __imp__sub_823AC118(ctx, base);
    if (!InPractice(base) || !g_camera.ready || !ctx.r3.u32 || !fighter || !held || !pressed) return;
    const auto enemies = Enemies(base);
    if ((fighter != enemies[0] && fighter != enemies[1]) || Defeated(base, fighter) || !CanWalk(base, fighter)) return;
    auto& enemy = g_enemies[fighter == enemies[0] ? 0 : 1];
    if (enemy.strikePending)
    {
        enemy.strikePending = false;
        const uint32_t player = PlayerFighter(base);
        if (!player || Defeated(base, player)) return;
        FaceForStrike(base, fighter, player);
        GWrite32(base, held, GRead32(base, held) | kEnemyStrike);
        GWrite32(base, pressed, GRead32(base, pressed) | kEnemyStrike);
    }
    else if (enemy.walking)
    {
        // Practice CPU directions are relative to the fighter. This is forward.
        GWrite32(base, held, (GRead32(base, held) & ~kDirectionButtons) | kWalkRight);
    }
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
    if (Defeated(base, fighter))
    {
        ctx.r3.u32 = 0;
        return;
    }
    ctx.r3.u32 &= ~kDirectionButtons;
    if (lr != 0x82303A1Cu || !CanWalk(base, fighter)) return;
    int horizontal, depth;
    TekkenForceReadMoveAxes(horizontal, depth);
    if (ctx.r3.u32 & kAttackButtons)
    {
        const uint32_t enemy = AttackTarget(base, fighter, horizontal, depth);
        if (enemy)
        {
            // Native attack movement also reads the fighter's opponent link.
            if (EnemyPartner(base)) GWrite32(base, fighter + kOpponent, enemy);
            FaceForStrike(base, fighter, enemy);
        }
        return;
    }
    if (horizontal) ctx.r3.u32 |= WalkDirection(float(horizontal));
    else if (depth)
    {
        const uint32_t enemy = AttackTarget(base, fighter, 0, 0);
        ctx.r3.u32 |= WalkDirection(enemy ? AlongBetween(base, fighter, enemy) : 1.0f);
    }
}
