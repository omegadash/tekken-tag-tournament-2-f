#include "kernel_shared.h"
#include "gpu_render.h"
#include "tekken_force_settings.h"
#include "tekken_force_controls.h"
#include "tekken_force_camera_diagnostics.h"
#include <algorithm>
#include <vector>
#include <atomic>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <numbers>
#include <chrono>

extern "C" PPC_FUNC(__imp__sub_823A70F0);
extern "C" PPC_FUNC(__imp__sub_823A9700);
extern "C" PPC_FUNC(__imp__sub_823A9778);
extern "C" PPC_FUNC(__imp__sub_823A9D88);
extern "C" PPC_FUNC(__imp__sub_823AC118);
extern "C" PPC_FUNC(__imp__sub_824D0D58);
extern "C" PPC_FUNC(__imp__sub_824D0D78);
extern "C" PPC_FUNC(__imp__sub_82545EE0);
extern "C" PPC_FUNC(__imp__sub_82541C90);
extern "C" PPC_FUNC(__imp__sub_82542D18);
extern "C" PPC_FUNC(__imp__sub_822CB9F0);
extern "C" PPC_FUNC(__imp__sub_82294410);
extern "C" PPC_FUNC(__imp__sub_822CC7C8);
extern "C" PPC_FUNC(__imp__sub_822CBD80);
extern "C" PPC_FUNC(__imp__sub_822CD798);
extern "C" PPC_FUNC(__imp__sub_822CDA88);
extern "C" PPC_FUNC(__imp__sub_822D4B68);
extern "C" PPC_FUNC(__imp__sub_822E1ED0);
extern "C" PPC_FUNC(__imp__sub_822E2060);
extern "C" PPC_FUNC(__imp__sub_822E2590);
extern "C" PPC_FUNC(__imp__sub_822E2B40);
extern "C" PPC_FUNC(__imp__sub_822E31C8);
extern "C" PPC_FUNC(__imp__sub_822E36C0);
extern "C" PPC_FUNC(__imp__sub_822E3E18);
extern "C" PPC_FUNC(__imp__sub_822E4350);
extern "C" PPC_FUNC(__imp__sub_822E4650);
extern "C" PPC_FUNC(__imp__sub_822E75F0);
extern "C" PPC_FUNC(__imp__sub_822E80D0);
extern "C" PPC_FUNC(__imp__sub_822F74C8);
extern "C" PPC_FUNC(__imp__sub_822F7F48);
extern "C" PPC_FUNC(__imp__sub_822F8C18);
extern "C" PPC_FUNC(__imp__sub_822FA6C8);
extern "C" PPC_FUNC(__imp__sub_822FA720);
extern "C" PPC_FUNC(__imp__sub_822FAF50);
extern "C" PPC_FUNC(__imp__sub_822FB4F8);
extern "C" PPC_FUNC(__imp__sub_822FB748);
extern "C" PPC_FUNC(__imp__sub_822FBF98);
extern "C" PPC_FUNC(__imp__sub_82303840);
extern "C" PPC_FUNC(__imp__sub_82303BB0);
extern "C" PPC_FUNC(__imp__sub_823165C8);
extern "C" PPC_FUNC(__imp__sub_82317BD0);
extern "C" PPC_FUNC(__imp__sub_82317EB0);
extern "C" PPC_FUNC(__imp__sub_82318FC0);
extern "C" PPC_FUNC(__imp__sub_8231DA80);
extern "C" PPC_FUNC(__imp__sub_82320AC8);
extern "C" PPC_FUNC(__imp__sub_82320C30);
extern "C" PPC_FUNC(__imp__sub_82320C88);
extern "C" PPC_FUNC(__imp__sub_82323CF8);
extern "C" PPC_FUNC(__imp__sub_82394F48);
extern "C" PPC_FUNC(__imp__sub_82396828);
extern "C" PPC_FUNC(__imp__sub_82396AE0);
extern "C" PPC_FUNC(__imp__sub_82397F60);
extern "C" PPC_FUNC(__imp__sub_82445A50);
extern "C" PPC_FUNC(__imp__sub_824D0970);
extern "C" PPC_FUNC(__imp__sub_825582D8);
extern "C" PPC_FUNC(__imp__sub_825B75B0);
void TekkenForceReadMoveAxes(int& horizontal, int& depth);
uint16_t TekkenForceReadButtons();
extern "C" PPC_FUNC(__imp__sub_82331B50);
extern "C" PPC_FUNC(__imp__sub_823D4690);
extern "C" PPC_FUNC(__imp__sub_824C75F8);
extern "C" PPC_FUNC(__imp__sub_82715898);
extern "C" PPC_FUNC(__imp__sub_827142C0);
extern "C" PPC_FUNC(__imp__sub_8249F158);
extern "C" PPC_FUNC(__imp__sub_823D4D38);
extern "C" PPC_FUNC(__imp__sub_822A8160);
extern "C" PPC_FUNC(__imp__sub_82314EA0);
void TekkenForceBlankScreen(bool on);
void TekkenForcePublishOverlay(uint8_t* base);

namespace {
constexpr uint32_t kBattleState = 0x82EAC03Cu;
constexpr uint32_t kPracticeState = 21;
constexpr uint32_t kPracticeSettings = 0x82DD5F48u;
constexpr uint32_t kFighterTable = 0x82E88878u;
constexpr uint32_t kPracticeExtra = 0x82EA71A0u;
constexpr uint32_t kFifthFighterSlot = kFighterTable + 4 * sizeof(uint32_t);
constexpr uint32_t kCombotFighter = 98, kCombotAlternate = 99, kPracticeDummyFighter = 118;
constexpr uint32_t kCombotIdle = 1438;     // Cb_sKAM00_
constexpr uint32_t kCombotApproach = 1439; // sWALK_00F
constexpr uint32_t kCombotWalkMove = 1444, kCombotWalkLoop = 1445;
constexpr uint32_t kCombotRun = 140, kCombotRunLast = 143; // sRUN__00F through sRUN__03F
constexpr uint32_t kCombotJab = 1503;      // Kz_lp00 from Combot's native move table
constexpr uint32_t kCombotFall = 41;       // Fall_sKAM
constexpr uint32_t kCombotFaceUp = 164, kCombotFaceDown = 176;
constexpr uint32_t kCombotRiseFaceUp = 188, kCombotRiseFaceDown = 235;
constexpr uint32_t kScene = 0x82E6B05Cu;
constexpr uint32_t kFighterModels = 0x82E6B540u;
constexpr uint32_t kCameraManagerOffset = 724;
constexpr uint32_t kRenderCameraOffset = 384;
constexpr uint32_t kProjectionOffset = 0xA0;
constexpr uint32_t kLife = 96; // Fighter's current life in 16.16 fixed point.
constexpr uint32_t kOpponent = 152;
constexpr uint32_t kContact = 156;
constexpr uint32_t kPartner = 172;
constexpr uint32_t kOnStage = 996;
constexpr uint32_t kSolo = 1012;
constexpr uint32_t kUsesCpuInput = 1032;
constexpr uint32_t kOnPoint = 7792;
constexpr uint32_t kTeam = 7936;
constexpr uint32_t kPosture = 875;
constexpr uint8_t kFirstDownedPosture = 12;
constexpr float kWalkStep = 24.0f;
constexpr float kBodyContactDistance = 420.0f;
constexpr float kEnemyStep = 14.0f;
constexpr float kEnemyRunStep = 42.0f;
constexpr float kEnemyStrikeRange = 700.0f;
constexpr float kEnemyApproachGap = 645.0f;
constexpr unsigned kEnemyStartDelay = 45;
constexpr unsigned kEnemyWindup = 18;
constexpr unsigned kEnemyRecovery = 54;
constexpr uint32_t kEnemyStrike = 0x4000u; // TU3's decoded fighter input for Xbox X.
constexpr uint32_t kAttackButtons = 0xF000u;
constexpr uint32_t kDirectionButtons = 0x0Fu;
constexpr uint32_t kTagButton = 0x0200u;
constexpr uint32_t kWalkLeft = 0x04u, kWalkRight = 0x08u;

struct FixedCamera
{
    bool ready = false;
    std::array<float, 3> eye{}, target{}, up{};
    float centerX = 0.0f;
    float centerZ = 0.0f;
    float rightX = 1.0f;
    float rightZ = 0.0f;
    float pullback = 1.5f;
    float followX = 0.0f, followZ = 0.0f;
    std::array<uint32_t, 8> lens{};
    std::array<uint32_t, 16> projection{};
    std::array<uint32_t, 16> stationaryView{}, stationaryProjection{}, stationaryVp{};
    bool stationaryReady = false;
};

FixedCamera g_camera;
uint64_t g_cameraTick = 0;

enum class EnemyPhase { Approach, Windup, Recover };

struct EnemyController
{
    EnemyPhase phase = EnemyPhase::Recover;
    unsigned framesLeft = kEnemyStartDelay;
    bool strikePending = false;
    bool walking = false;
    unsigned defeatedFrames = 0;
    bool entering = true;
    uint32_t entryLife = 0;
    uint32_t lastLife = 0;
    unsigned disengageFrames = 0;
};

constexpr unsigned kMaxEnemies = 4;
std::array<EnemyController, kMaxEnemies> g_enemies;
int g_attackOwner = -1;
unsigned g_nextAttacker = 0;
unsigned g_attackPause = kEnemyStartDelay;
unsigned g_turnFrames = 0;
uint32_t g_target = 0;
tekken_force::PlayerControls g_controls;
tekken_force::TargetAction g_targetAction = tekken_force::TargetAction::None;
thread_local int g_drawingEnemyGauge = -1;
thread_local bool g_gaugeMirrored = false;
thread_local bool g_firstGaugeSprite = false;
thread_local float g_gaugeOriginX = 0.0f, g_gaugeOriginY = 0.0f, g_gaugeWidth = 0.0f;
bool g_spawnPartner = true;
bool g_fullRoster = false;
bool g_runningNativePairs = false;
uint32_t g_reassignedReserve = 0;
uint32_t g_reserveInput = 0;
uint32_t g_reserveTeam = 0;

struct FloorMotion
{
    uint32_t fighter = 0;
    float x = 0.0f, z = 0.0f;
    bool walking = false;
    uint32_t life = 0;
};

std::array<FloorMotion, kMaxEnemies + 1> g_motion;
std::array<FloorMotion, kMaxEnemies> g_entryMotion;
std::array<FloorMotion, kMaxEnemies + 1> g_walkMotion;

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
    const uint32_t side = GRead32(base, kPracticeSettings + 100);
    if (g_fullRoster && side < 2) return GRead32(base, kFighterTable + side * 4);
    return ActiveFighter(base, side);
}

uint32_t OpponentSide(uint8_t* base)
{
    return GRead32(base, kPracticeSettings + 100) ^ 1;
}

uint32_t EnemyFighter(uint8_t* base)
{
    return ActiveFighter(base, OpponentSide(base));
}

bool LoadedCombot(uint8_t* base, uint32_t fighter)
{
    if (!fighter || !GRead32(base, fighter + 136)) return false;
    const uint32_t id = GRead32(base, fighter + 88);
    return id == kCombotFighter || id == kCombotAlternate;
}

uint32_t EnemyPartner(uint8_t* base)
{
    const uint32_t side = OpponentSide(base);
    if (side > 1) return 0;
    const uint32_t lead = GRead32(base, kFighterTable + side * 4);
    const uint32_t reserve = lead ? GRead32(base, lead + kPartner) : 0;
    // Practice marks duplicate CPU picks as Solo even when both actors were loaded separately.
    return LoadedCombot(base, reserve) ? reserve : 0;
}

uint32_t PlayerLead(uint8_t* base)
{
    const uint32_t side = GRead32(base, kPracticeSettings + 100);
    return side < 2 ? GRead32(base, kFighterTable + side * 4) : 0;
}

bool FullRosterRequested(uint8_t* base)
{
    const uint32_t player = PlayerLead(base);
    const uint32_t reserve = player ? GRead32(base, player + kPartner) : 0;
    return LoadedCombot(base, reserve) && EnemyPartner(base) && LoadedCombot(base, kPracticeExtra) &&
           GRead32(base, kPracticeExtra + 3068);
}

std::array<uint32_t, kMaxEnemies> Enemies(uint8_t* base)
{
    const uint32_t second = EnemyPartner(base);
    if (!second) return {EnemyFighter(base)};
    const uint32_t first = GRead32(base, kFighterTable + OpponentSide(base) * 4);
    if (!g_fullRoster) return {first, second};
    return {first, second, GRead32(base, PlayerLead(base) + kPartner), kPracticeExtra};
}

bool IsEnemy(uint8_t* base, uint32_t actor)
{
    if (!actor) return false;
    const auto enemies = Enemies(base);
    return std::find(enemies.begin(), enemies.end(), actor) != enemies.end();
}

void ResetControllers(uint8_t* base)
{
    if (g_reassignedReserve)
    {
        GWrite32(base, g_reassignedReserve + kTeam, g_reserveTeam);
        GWrite32(base, g_reassignedReserve + kUsesCpuInput, g_reserveInput);
        g_reassignedReserve = 0;
    }
    g_enemies = {};
    g_attackOwner = -1;
    g_nextAttacker = 0;
    g_attackPause = kEnemyStartDelay;
    g_turnFrames = 0;
    g_target = 0;
    g_controls = {};
    g_targetAction = tekken_force::TargetAction::None;
    const float previousPullback = g_camera.pullback;
    g_camera.pullback = 1.0f + tekken_force::LoadCameraPullbackPercent() / 100.0f;
    if (g_camera.ready)
        for (unsigned i = 0; i < 3; ++i)
            g_camera.eye[i] = g_camera.target[i] +
                (g_camera.eye[i] - g_camera.target[i]) * g_camera.pullback / previousPullback;
    g_spawnPartner = true;
    g_fullRoster = false;
    g_motion = {};
    g_entryMotion = {};
    g_walkMotion = {};
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

bool NativeWalk(uint8_t* base, uint32_t fighter)
{
    const uint32_t moveset = GRead32(base, fighter + 136);
    if (!moveset) return false;
    constexpr uint32_t kMoveSize = 112;
    // TU3 resolves common stance 0x8001 through the character's move table.
    // Its forward and backward walk entries follow that stance in the table.
    const uint32_t table = GRead32(base, moveset + 416);
    const uint32_t neutral = table + kMoveSize * GRead16(base, moveset + 138);
    const uint32_t move = GRead32(base, fighter + 280);
    if (LoadedCombot(base, fighter) && move >= table + kCombotRun * kMoveSize &&
        move <= table + kCombotRunLast * kMoveSize) return true;
    if (fighter == kPracticeExtra)
    {
        return move == table + kCombotApproach * kMoveSize ||
               move == table + kCombotWalkMove * kMoveSize || move == table + kCombotWalkLoop * kMoveSize;
    }
    return move > neutral && move <= neutral + 10 * kMoveSize;
}

bool ExtraInMove(uint8_t* base, uint32_t moveId)
{
    constexpr uint32_t kMoveSize = 112;
    const uint32_t table = GRead32(base, GRead32(base, kPracticeExtra + 136) + 416);
    return GRead32(base, kPracticeExtra + 280) == table + moveId * kMoveSize;
}

bool CanWalk(uint8_t* base, uint32_t fighter)
{
    const uint8_t posture = GRead8(base, fighter + kPosture);
    const uint32_t moveset = GRead32(base, fighter + 136);
    if (!moveset || posture >= kFirstDownedPosture) return false;
    const uint32_t neutral = GRead32(base, moveset + 416) + 112 * GRead16(base, moveset + 138);
    // An attack can have standing posture too. Arena walking must not cancel it.
    return GRead32(base, fighter + 280) == neutral || NativeWalk(base, fighter);
}

void FaceDirection(uint8_t* base, uint32_t fighter, float x, float z)
{
    if (std::abs(x) + std::abs(z) < 1.0f) return;
    const float radians = std::atan2(x, z);
    const auto angle = static_cast<uint16_t>(std::lround(radians * (32768.0f / std::numbers::pi_v<float>)));
    GWrite16(base, fighter + 14, angle);
    GWrite16(base, fighter + 190, angle);
    WriteFloat(base, fighter + 32, radians);
    WriteFloat(base, fighter + 184, radians);
}

void FaceForStrike(uint8_t* base, uint32_t fighter, uint32_t target)
{
    FaceDirection(base,fighter,ReadFloat(base,target)-ReadFloat(base,fighter),
        ReadFloat(base,target+8)-ReadFloat(base,fighter+8));
}

void SamplePlayerControls()
{
    int horizontal,depth;
    TekkenForceReadMoveAxes(horizontal,depth);
    const uint16_t buttons=TekkenForceReadButtons();
    const uint64_t now=uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    const auto action=g_controls.Sample(now,(buttons&0x0200u)!=0,depth,(buttons&0xF000u)!=0);
    if (action!=tekken_force::TargetAction::None) g_targetAction=action;
}

void FaceFreeMovement(uint8_t* base,uint32_t fighter)
{
    if (!CanWalk(base,fighter) || (TekkenForceReadButtons()&0xF000u)) return;
    int horizontal,depth;
    TekkenForceReadMoveAxes(horizontal,depth);
    if (horizontal || depth)
        FaceDirection(base,fighter,horizontal*g_camera.rightX-depth*g_camera.rightZ,
            horizontal*g_camera.rightZ+depth*g_camera.rightX);
}

bool EncounterPair(uint8_t* base, uint32_t first, uint32_t second)
{
    if (!InPractice(base) || !g_camera.ready || !first || !second) return false;
    const uint32_t player = PlayerFighter(base);
    return (first == player && IsEnemy(base, second)) || (second == player && IsEnemy(base, first));
}

bool RejectHit(uint8_t* base, uint32_t attacker, uint32_t defender)
{
    if (InPractice(base) && g_camera.ready)
    {
        if (g_fullRoster && attacker == defender && attacker == PlayerFighter(base)) return true;
        if (IsEnemy(base, attacker) && IsEnemy(base, defender) && (attacker != defender || g_fullRoster)) return true;
    }
    if (!EncounterPair(base, attacker, defender)) return false;
    if (g_runningNativePairs) return true;
    return Defeated(base, attacker) || Defeated(base, defender) ||
           !GRead32(base, attacker + kOnStage) || !GRead32(base, defender + kOnStage);
}

uint32_t AttackTarget(uint8_t* base, uint32_t player, int horizontal, int depth)
{
    if (g_target && !Defeated(base, g_target) && GRead32(base, g_target + kOnStage)) return g_target;
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
        float score = distance;
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

void UpdateTarget(uint8_t* base, uint32_t player)
{
    SamplePlayerControls();
    const uint32_t previous = g_target;
    const auto enemies = Enemies(base);
    const auto action=g_targetAction;
    g_targetAction=tekken_force::TargetAction::None;
    const auto valid = [&](uint32_t enemy) {
        return enemy && !Defeated(base, enemy) && GRead32(base, enemy + kOnStage);
    };
    if (action==tekken_force::TargetAction::Release || g_controls.freeMovement)
    {
        g_target=0;
        FaceFreeMovement(base,player);
    }
    else if (action==tekken_force::TargetAction::Cycle)
    {
        auto current = std::find(enemies.begin(), enemies.end(), g_target);
        const unsigned start = current == enemies.end() ? 0 : unsigned(current - enemies.begin()) + 1;
        for (unsigned step = 0; step < enemies.size(); ++step)
        {
            const uint32_t candidate = enemies[(start + step) % enemies.size()];
            if (valid(candidate)) { g_target = candidate; break; }
        }
    }
    if (action==tekken_force::TargetAction::Nearest) g_target=0;
    if (!g_controls.freeMovement && !valid(g_target))
    {
        g_target = 0;
        g_target = AttackTarget(base, player, 0, 0);
    }
    if (!g_controls.freeMovement && !g_target) g_controls.freeMovement=true;
    // Practice uses the extra actor's opponent link as a mode selector. Keep
    // its roster link stable and bind the actual target in the pose/hit paths.
    if (g_target && g_target != kPracticeExtra) GWrite32(base, player + kOpponent, g_target);
    if (g_target && CanWalk(base, player)) FaceForStrike(base, player, g_target);
    if ((g_target != previous || action!=tekken_force::TargetAction::None) && std::getenv("TTT2_TF_DIAG"))
        std::fprintf(stderr, "[tf-target] actor=%08X free=%u\n", g_target,unsigned(g_controls.freeMovement));
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

void ActivateEnemy(PPCContext& ctx, uint8_t* base, uint32_t enemy, uint32_t player, float along, float depth)
{
    const uint32_t contact = GRead32(base, player + kContact);
    GWrite32(base, enemy + kOnPoint, 1);
    GWrite32(base, enemy + kTeam, OpponentSide(base));
    GWrite32(base, enemy + kUsesCpuInput, 1);
    PPCContext spawn = ctx;
    spawn.r3.u32 = enemy;
    spawn.r4.u32 = player;
    spawn.r5.u32 = 7; // Practice position reset.
    spawn.r6.u32 = 0; // Use the standing move.
    __imp__sub_8231DA80(spawn, base);
    GWrite32(base, player + kContact, contact);
    GWrite32(base, enemy + kOnStage, 1);
    if (enemy == kPracticeExtra)
    {
        GWrite32(base, enemy + 92, 4); // Health record resolves its owner through slot 4.
        GWrite32(base, enemy + 108, GRead32(base, EnemyFighter(base) + 108));
        spawn.r3.u32 = enemy + kLife - 4;
        __imp__sub_824D0D58(spawn, base);
    }
    SetFloorPosition(ctx, base, enemy,
        g_camera.centerX + along * g_camera.rightX - depth * g_camera.rightZ,
        g_camera.centerZ + along * g_camera.rightZ + depth * g_camera.rightX);
    FaceForStrike(base, enemy, player);
}

void MoveOnFloor(PPCContext& ctx, uint8_t* base, uint32_t fighter, float alongStep, float depthStep)
{
    const float x = ReadFloat(base, fighter);
    const float z = ReadFloat(base, fighter + 8);
    const float offsetX = x - g_camera.centerX, offsetZ = z - g_camera.centerZ;
    const float rightX = g_camera.rightX, rightZ = g_camera.rightZ;
    // Use the stage floor rather than the prototype's 3.2 m by 1.6 m rectangle.
    // The native stage update remains responsible for its geometry and boundaries.
    const float along = offsetX * rightX + offsetZ * rightZ + alongStep;
    const float across = FloorDepth(offsetX, offsetZ) + depthStep;
    const float nextX = g_camera.centerX + along * rightX - across * rightZ;
    const float nextZ = g_camera.centerZ + along * rightZ + across * rightX;
    if (std::abs(nextX - x) > 0.001f || std::abs(nextZ - z) > 0.001f)
        SetFloorPosition(ctx, base, fighter, nextX, nextZ);
}

void ReconcileWalk(PPCContext& ctx, uint8_t* base, uint32_t fighter, FloorMotion& motion)
{
    const bool walking = NativeWalk(base, fighter);
    const bool walkEnded = motion.walking && CanWalk(base, fighter);
    if (motion.fighter == fighter && motion.life == GRead32(base,fighter+kLife) && (walking || walkEnded))
        SetFloorPosition(ctx, base, fighter, motion.x, motion.z);
    motion.walking = walking;
}

void MovePlayer(PPCContext& ctx, uint8_t* base, uint32_t fighter)
{
    int horizontal, depth;
    TekkenForceReadMoveAxes(horizontal, depth);
    if (!g_controls.freeMovement) depth=g_controls.depthMotion;
    const float step = horizontal && depth ? kWalkStep * 0.70710678f : kWalkStep;
    if (g_controls.freeMovement || !g_target)
    {
        MoveOnFloor(ctx,base,fighter,horizontal*step,depth*step);
        return;
    }
    const float tx=ReadFloat(base,g_target), tz=ReadFloat(base,g_target+8);
    const float dx=tx-ReadFloat(base,fighter), dz=tz-ReadFloat(base,fighter+8);
    const float radius=std::hypot(dx,dz);
    if (radius<1.0f || (!horizontal && !depth)) return;
    const float along=dx*g_camera.rightX+dz*g_camera.rightZ;
    const float side=std::abs(along)<50.0f ? (GRead32(base,fighter+1024)?-1.0f:1.0f) :
        (along>0.0f?1.0f:-1.0f);
    const float nextRadius=std::max(50.0f,radius-horizontal*side*step);
    const float angle=-depth*side*step/radius;
    const float px=-dx/radius*nextRadius, pz=-dz/radius*nextRadius;
    SetFloorPosition(ctx,base,fighter,tx+px*std::cos(angle)-pz*std::sin(angle),
        tz+px*std::sin(angle)+pz*std::cos(angle));
}

void SeparateEnemies(PPCContext& ctx, uint8_t* base, const std::array<uint32_t, kMaxEnemies>& enemies)
{
    constexpr float spacing = 1100.0f;
    for (unsigned i = 0; i < enemies.size(); ++i)
        for (unsigned j = i + 1; j < enemies.size(); ++j)
        {
            const uint32_t a = enemies[i], b = enemies[j];
            if (!a || !b || Defeated(base,a) || Defeated(base,b) ||
                !GRead32(base,a+kOnStage) || !GRead32(base,b+kOnStage) ||
                g_enemies[i].entering || g_enemies[j].entering) continue;
            float dx = ReadFloat(base,b)-ReadFloat(base,a), dz = ReadFloat(base,b+8)-ReadFloat(base,a+8);
            float distance = std::hypot(dx,dz);
            if (distance >= spacing) continue;
            if (distance < 1.0f) { dx = g_camera.rightX; dz = g_camera.rightZ; distance = 1.0f; }
            const bool moveA = int(i) != g_attackOwner && CanWalk(base,a);
            const bool moveB = int(j) != g_attackOwner && CanWalk(base,b);
            const unsigned movable = unsigned(moveA) + unsigned(moveB);
            if (!movable) continue;
            const float step = std::min(24.0f,(spacing-distance)/float(movable));
            dx *= step/distance; dz *= step/distance;
            if (moveA) SetFloorPosition(ctx,base,a,ReadFloat(base,a)-dx,ReadFloat(base,a+8)-dz);
            if (moveB) SetFloorPosition(ctx,base,b,ReadFloat(base,b)+dx,ReadFloat(base,b+8)+dz);
        }
}

void ScheduleAttack(uint8_t* base, uint32_t player, const std::array<uint32_t, kMaxEnemies>& enemies)
{
    if (g_attackOwner >= 0)
    {
        const uint32_t owner = enemies[g_attackOwner];
        const auto& turn = g_enemies[g_attackOwner];
        const bool interrupted = !owner || Defeated(base, owner) ||
            GRead8(base, owner + kPosture) >= kFirstDownedPosture;
        const bool finished = turn.phase == EnemyPhase::Recover && !turn.framesLeft &&
            !turn.strikePending && CanWalk(base, owner);
        const bool stalled = ++g_turnFrames > 300 && CanWalk(base, owner);
        if (!(interrupted || finished || stalled)) return;
        g_enemies[g_attackOwner].strikePending = false;
        g_enemies[g_attackOwner].disengageFrames = 90;
        g_nextAttacker = (unsigned(g_attackOwner) + 1) % kMaxEnemies;
        g_attackOwner = -1;
        g_attackPause = 24;
    }
    if (g_attackPause) { --g_attackPause; return; }
    if (Defeated(base, player) || GRead8(base, player + kPosture) >= kFirstDownedPosture) return;
    for (unsigned offset = 0; offset < kMaxEnemies; ++offset)
    {
        const unsigned i = (g_nextAttacker + offset) % kMaxEnemies;
        const uint32_t enemy = enemies[i];
        if (!enemy || Defeated(base, enemy) || !GRead32(base, enemy + kOnStage) ||
            g_enemies[i].entering || !CanWalk(base, enemy)) continue;
        if (std::hypot(ReadFloat(base, enemy) - ReadFloat(base, player),
                       ReadFloat(base, enemy + 8) - ReadFloat(base, player + 8)) > 4500.0f) continue;
        g_attackOwner = int(i);
        g_turnFrames = 0;
        g_enemies[i].phase = EnemyPhase::Approach;
        g_enemies[i].framesLeft = 0;
        g_enemies[i].disengageFrames = 0;
        break;
    }
}

void UpdateEnemy(PPCContext& ctx, uint8_t* base, uint32_t enemy, uint32_t player,
                 EnemyController& controller, unsigned index)
{
    controller.walking = false;
    if (!controller.entryLife) controller.entryLife = GRead32(base, enemy + kLife);
    if (GRead32(base, enemy + kLife) < controller.entryLife)
    {
        controller.entering = false;
    }
    if (GRead32(base,enemy+kLife) < controller.lastLife) controller.disengageFrames = 0;
    controller.lastLife = GRead32(base,enemy+kLife);
    const bool defeated = Defeated(base, enemy);
    if (g_fullRoster && defeated && GRead32(base, enemy + kOnStage))
    {
        constexpr unsigned kDownedFramesBeforeRemoval = 75;
        constexpr unsigned kMaxDefeatFrames = 180;
        const bool downed = GRead8(base, enemy + kPosture) >= kFirstDownedPosture;
        if (enemy == kPracticeExtra && ExtraInMove(base, kCombotIdle))
        {
            // The extra actor can return to idle after a reduced lethal reaction.
            PPCContext fall = ctx;
            fall.r3.u32 = enemy;
            fall.r4.u32 = kCombotFall;
            __imp__sub_823165C8(fall, base);
        }
        if (++controller.defeatedFrames >= kMaxDefeatFrames ||
            (downed && controller.defeatedFrames >= kDownedFramesBeforeRemoval))
            GWrite32(base, enemy + kOnStage, 0);
    }
    if (defeated || Defeated(base, player))
    {
        controller.strikePending = false;
        return;
    }

    // Native target-relative inputs and facing must always refer to the player.
    GWrite32(base, enemy + kOpponent, player);
    if (!CanWalk(base, enemy))
    {
        controller.strikePending = false;
        return;
    }
    FaceForStrike(base, enemy, player);
    const float x = ReadFloat(base, player) - ReadFloat(base, enemy);
    const float z = ReadFloat(base, player + 8) - ReadFloat(base, enemy + 8);
    const float along = x * g_camera.rightX + z * g_camera.rightZ;
    const float depth = FloorDepth(x, z);
    const float range = std::hypot(along, depth);
    if (controller.entering && range < 2200.0f) controller.entering = false;
    const bool ownsTurn = g_attackOwner == int(index) && !controller.entering;
    if (ownsTurn && GRead8(base, player + kPosture) >= kFirstDownedPosture)
    {
        controller.phase = EnemyPhase::Approach;
        controller.framesLeft = 0;
        controller.strikePending = false;
        return;
    }
    if (ownsTurn && controller.phase == EnemyPhase::Recover)
    {
        if (controller.framesLeft) --controller.framesLeft;
        return;
    }
    if (ownsTurn && controller.phase == EnemyPhase::Windup)
    {
        if (controller.framesLeft && --controller.framesLeft) return;
        controller.phase = EnemyPhase::Recover;
        controller.framesLeft = kEnemyRecovery;
        controller.strikePending = true;
        g_turnFrames = 0;
        return;
    }
    float forward, across;
    if (ownsTurn || controller.entering)
    {
        if (ownsTurn && range <= kEnemyStrikeRange)
        {
            controller.phase = EnemyPhase::Windup;
            controller.framesLeft = kEnemyWindup;
            return;
        }
        const float approach = std::max(0.0f,range-kEnemyApproachGap)/std::max(1.0f,range);
        forward = along * approach;
        across = depth * approach;
    }
    else
    {
        // Follow a retreating player, but hold when close so waiting enemies do
        // not continually back away from the player's attacks.
        if (range > 1550.0f) controller.disengageFrames = 0;
        if (controller.disengageFrames) --controller.disengageFrames;
        if (range < 900.0f && !controller.disengageFrames) return;
        // Stagger the screen columns as well as the floor quadrants, so the
        // waiting fighters do not form two overlapping silhouettes.
        constexpr float alongSlots[] = {-1900.0f,1900.0f,-800.0f,800.0f};
        constexpr float depthSlots[] = {-800.0f,800.0f,1900.0f,-1900.0f};
        const float waitAlong = alongSlots[index];
        const float waitDepth = depthSlots[index];
        forward = along + waitAlong;
        across = depth + waitDepth;
        // Orbit toward a free quadrant instead of crossing through the player.
        const float currentAngle = std::atan2(-along,-depth);
        const float goalAngle = std::atan2(waitAlong,waitDepth);
        const float turn = std::remainder(goalAngle-currentAngle,2.0f*std::numbers::pi_v<float>);
        if (controller.disengageFrames && range < 1550.0f)
        {
            forward = -along;
            across = -depth;
        }
        else if (range < 2600.0f && std::abs(turn) > .25f)
        {
            const float direction = turn > 0.0f ? 1.0f : -1.0f;
            const float radial = std::clamp(2060.0f-range,-180.0f,180.0f);
            forward = (-depth/range)*direction*650.0f + (-along/range)*radial;
            across = (along/range)*direction*650.0f + (-depth/range)*radial;
        }
    }
    const float distance = std::hypot(forward, across);
    if (distance < 35.0f) return;
    const float speed = controller.entering ? kEnemyRunStep : kEnemyStep;
    const float scale = std::min(1.0f, speed / distance);
    controller.walking = true;
    MoveOnFloor(ctx, base, enemy, forward * scale, across * scale);
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
    g_camera.followX = x0;
    g_camera.followZ = z0;
    const float floorY = (ReadFloat(base, player + 4) + ReadFloat(base, dummy + 4)) * 0.5f;
    const float fx = -g_camera.rightZ, fz = g_camera.rightX;
    const float distance = 6200.0f * g_camera.pullback, rise = 1300.0f * g_camera.pullback;
    g_camera.target = { x0, floorY + 850.0f, z0 };
    g_camera.eye = { x0 + fx * distance, g_camera.target[1] + rise, z0 + fz * distance };
    g_camera.up = { 0.0f, 1.0f, 0.0f };
    for (unsigned i = 0; i < g_camera.lens.size(); ++i)
        g_camera.lens[i] = GRead32(base, camera + 64 + i * 4);
    for (unsigned i = 0; i < g_camera.projection.size(); ++i)
        g_camera.projection[i] = GRead32(base, camera + kProjectionOffset + i * 4);
    g_camera.ready = true;
}

void UpdateCameraFollow(uint8_t* base)
{
    if (TekkenForceStationaryCamera()) return;
    const uint32_t player = PlayerFighter(base);
    if (player)
    {
        const float x = ReadFloat(base, player), z = ReadFloat(base, player + 8);
        // Follow arena travel without changing the stage-relative movement axes.
        const float dx = (x - g_camera.followX) * 0.20f, dz = (z - g_camera.followZ) * 0.20f;
        g_camera.followX += dx;
        g_camera.followZ += dz;
        g_camera.eye[0] += dx; g_camera.eye[2] += dz;
        g_camera.target[0] += dx; g_camera.target[2] += dz;
    }
}

void ApplyCameraPose(uint8_t* base, uint32_t camera)
{
    for (unsigned i = 0; i < g_camera.lens.size(); ++i)
        GWrite32(base, camera + 64 + i * 4, g_camera.lens[i]);
    for (unsigned i = 0; i < g_camera.projection.size(); ++i)
        GWrite32(base, camera + kProjectionOffset + i * 4, g_camera.projection[i]);
    for (unsigned i = 0; i < 3; ++i)
    {
        WriteFloat(base, camera + 0x10 + 4 * i, g_camera.eye[i]);
        WriteFloat(base, camera + 0x20 + 4 * i, g_camera.target[i]);
        WriteFloat(base, camera + 0x30 + 4 * i, g_camera.up[i]);
    }
}

bool ExtraActive(uint8_t* base)
{
    return g_fullRoster && InPractice(base) && GRead32(base, kPracticeExtra + kOnStage);
}

uint32_t CallOnFighter(const PPCContext& ctx, uint8_t* base, PPCFunc* function, uint32_t fighter, uint32_t opponent = 0)
{
    PPCContext call = ctx;
    call.r3.u32 = fighter;
    call.r4.u32 = opponent;
    function(call, base);
    return call.r3.u32;
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
    ResetControllers(base);
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
            ResetControllers(base);
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
        if (g_fullRoster && (Defeated(base, first) || Defeated(base, second))) return;
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

// The standard collision walker visits the point opponent and its partner.
// Walk all combat pairs once in the larger encounter and retain native effects.
PPC_FUNC(sub_822E4350)
{
    if (!g_fullRoster || !InPractice(base))
    {
        __imp__sub_822E4350(ctx, base);
        return;
    }
    const uint32_t player = PlayerFighter(base);
    for (const uint32_t enemy : Enemies(base))
    {
        if (!enemy || RejectHit(base, player, enemy)) continue;
        CallOnFighter(ctx, base, __imp__sub_822E36C0, player, enemy);
        CallOnFighter(ctx, base, __imp__sub_822E3E18, player, enemy);
        CallOnFighter(ctx, base, __imp__sub_822E36C0, enemy, player);
        CallOnFighter(ctx, base, __imp__sub_822E3E18, enemy, player);
    }
    g_runningNativePairs = true;
    __imp__sub_822E4350(ctx, base);
    g_runningNativePairs = false;
}

// These phases have four-slot loops even though the extra Practice actor has
// its own pose and moveset. Run the same per-fighter routines for that actor.
PPC_FUNC(sub_822FA6C8)
{
    __imp__sub_822FA6C8(ctx, base);
    if (ExtraActive(base)) CallOnFighter(ctx, base, __imp__sub_822F8C18, kPracticeExtra);
}

PPC_FUNC(sub_822F7F48)
{
    __imp__sub_822F7F48(ctx, base);
    if (!ExtraActive(base)) return;
    // Store the starting point of each swept attack volume before the move advances.
    const uint32_t actor = kPracticeExtra;
    const uint32_t move = GRead32(base, actor + 280);
    const uint32_t skeleton = CallOnFighter(ctx, base, __imp__sub_82397F60, actor);
    const auto count = std::min<unsigned>(GRead8(base, actor + 890), 4);
    for (unsigned i = 0; i < count; ++i)
    {
        if (GRead8(base, move + 97 + i * 2)) continue;
        const uint32_t joint = GRead8(base, move + 96 + i * 2);
        PPCContext sample = ctx;
        sample.r1.u32 -= 128;
        const uint32_t position = sample.r1.u32 + 80;
        for (uint32_t offset = 0; offset < 16; offset += 4) GWrite32(base, position + offset, 0);
        sample.r3.u32 = position;
        sample.r4.u32 = joint;
        sample.r5.u32 = actor + 3280;
        sample.r6.u32 = skeleton;
        sample.r7.u32 = actor;
        __imp__sub_822F74C8(sample, base);
        sample.r3.u32 = actor;
        sample.r4.u32 = joint;
        sample.r5.u32 = position;
        __imp__sub_822FA720(sample, base);
        for (uint32_t offset = 0; offset < 12; offset += 4)
            GWrite32(base, actor + 2080 + i * 24 + offset, GRead32(base, position + offset));
    }
    CallOnFighter(ctx, base, __imp__sub_822FBF98, actor);
}

PPC_FUNC(sub_822FB748)
{
    __imp__sub_822FB748(ctx, base);
    if (!ExtraActive(base)) return;
    CallOnFighter(ctx, base, __imp__sub_822FAF50, kPracticeExtra);
    CallOnFighter(ctx, base, __imp__sub_822FB4F8, kPracticeExtra);
}

PPC_FUNC(sub_822E1ED0)
{
    __imp__sub_822E1ED0(ctx, base);
    if (!ExtraActive(base)) return;
    const uint32_t actor = kPracticeExtra;
    GWrite32(base, actor + 1148, GRead32(base, actor + 1148) & 0x1FE7FFFFu);
    // TU3 has two 64-byte hit records. Clear the queued-hit state each tick.
    for (const uint32_t hit : {actor + 2652, actor + 2716})
    {
        GWrite32(base, hit, actor);
        for (uint32_t offset = 40; offset < 60; offset += 4) GWrite32(base, hit + offset, 0);
        GWrite32(base, hit + 60, GRead32(base, hit + 60) & 0x60005FFFu);
    }
}

PPC_FUNC(sub_822E2060)
{
    __imp__sub_822E2060(ctx, base);
    if (ExtraActive(base)) CallOnFighter(ctx, base, __imp__sub_82317BD0, kPracticeExtra, PlayerFighter(base));
}

PPC_FUNC(sub_822E2590)
{
    __imp__sub_822E2590(ctx, base);
    if (!ExtraActive(base)) return;
    PPCContext advance = ctx;
    advance.r3.u32 = kPracticeExtra;
    advance.r4.u32 = PlayerFighter(base);
    advance.r5.u32 = 0;
    __imp__sub_82318FC0(advance, base);
}

PPC_FUNC(sub_822CD798)
{
    __imp__sub_822CD798(ctx, base);
    if (!ExtraActive(base)) return;
    const uint32_t models = GRead32(base, kFighterModels);
    // Sample the fifth skeleton in both native model-update phases.
    for (const uint32_t phase : {1u, 2u})
    {
        PPCContext sample = ctx;
        sample.r3.u32 = models;
        sample.r4.u32 = 4;
        sample.r5.u32 = phase;
        __imp__sub_82445A50(sample, base);
    }
    CallOnFighter(ctx, base, __imp__sub_82294410, kPracticeExtra);
}

PPC_FUNC(sub_82323CF8)
{
    __imp__sub_82323CF8(ctx, base);
    if (ExtraActive(base)) CallOnFighter(ctx, base, __imp__sub_82320C88, kPracticeExtra);
}

PPC_FUNC(sub_82320C30)
{
    __imp__sub_82320C30(ctx, base);
    if (!ExtraActive(base)) return;
    GWrite16(base, kPracticeExtra + 192, GRead16(base, kPracticeExtra + 190));
    PPCContext transform = ctx;
    transform.r3.u32 = kPracticeExtra;
    transform.r4.u32 = PlayerFighter(base);
    transform.r5.u32 = 0;
    __imp__sub_822CBD80(transform, base);
}

PPC_FUNC(sub_822E80D0)
{
    if (ExtraActive(base))
    {
        const uint32_t actor = kPracticeExtra;
        GWrite32(base, actor + 1148, GRead32(base, actor + 1148) & ~0x80u);
        for (const uint32_t offset : {1216u, 1220u, 1224u}) GWrite32(base, actor + offset, 0);
        const uint32_t result = CallOnFighter(ctx, base, __imp__sub_822E75F0, actor);
        CallOnFighter(ctx, base, __imp__sub_824D0970, actor + kLife - 4);
        PPCContext event = ctx;
        event.r3.u32 = actor;
        event.r4.u32 = PlayerFighter(base);
        event.r5.u32 = GRead32(base, actor + 2780);
        event.r6.u32 = result;
        __imp__sub_825582D8(event, base);
    }
    __imp__sub_822E80D0(ctx, base);
}

PPC_FUNC(sub_822CC7C8)
{
    __imp__sub_822CC7C8(ctx, base);
    if (!ExtraActive(base)) return;
    CallOnFighter(ctx, base, __imp__sub_82303BB0, kPracticeExtra);
    CallOnFighter(ctx, base, __imp__sub_82396AE0, kPracticeExtra);
    if (!GRead32(base, kPracticeExtra + 2060)) CallOnFighter(ctx, base, __imp__sub_82303840, kPracticeExtra);
}

PPC_FUNC(sub_822E31C8)
{
    __imp__sub_822E31C8(ctx, base);
    if (!ExtraActive(base)) return;
    CallOnFighter(ctx, base, __imp__sub_822E4650, kPracticeExtra);
    CallOnFighter(ctx, base, __imp__sub_82394F48, kPracticeExtra);
    CallOnFighter(ctx, base, __imp__sub_82317EB0, kPracticeExtra);
    CallOnFighter(ctx, base, __imp__sub_822E2B40, kPracticeExtra);
}

// Resolve custom movement before the guest prepares this frame's hit volumes.
PPC_FUNC(sub_82320AC8)
{
    ++g_cameraTick;
    TekkenForcePublishOverlay(base);
    if (InPractice(base) && g_camera.ready)
    {
        TekkenForceBlankScreen(false);   // the select hid itself until the encounter is up
        if (g_spawnPartner) g_fullRoster = FullRosterRequested(base);
        const uint32_t player = PlayerFighter(base);
        const auto enemies = Enemies(base);
        if (player && enemies[0])
        {
            ReconcileWalk(ctx, base, player, g_motion[0]);
            if (g_spawnPartner && enemies[1])
            {
                const float offsetX = ReadFloat(base, player) - g_camera.centerX;
                const float offsetZ = ReadFloat(base, player + 8) - g_camera.centerZ;
                const float originAlong = offsetX * g_camera.rightX + offsetZ * g_camera.rightZ;
                const float originDepth = FloorDepth(offsetX, offsetZ);
                const uint32_t camera = RenderCamera(base);
                const float projection = std::max(0.5f, ReadFloat(base, camera + kProjectionOffset));
                const float offscreen = 6200.0f * g_camera.pullback / projection + 2600.0f;
                ActivateEnemy(ctx, base, enemies[0], player, originAlong - offscreen, originDepth - 1400.0f);
                ActivateEnemy(ctx, base, enemies[1], player, originAlong + offscreen, originDepth + 1400.0f);
                if (g_fullRoster)
                {
                    GWrite32(base, enemies[0] + kSolo, 0);
                    GWrite32(base, enemies[1] + kSolo, 0);
                    g_reassignedReserve = enemies[2];
                    g_reserveInput = GRead32(base, enemies[2] + kUsesCpuInput);
                    g_reserveTeam = GRead32(base, enemies[2] + kTeam);
                    ActivateEnemy(ctx, base, enemies[2], player, originAlong - offscreen - 1200.0f, originDepth + 2400.0f);
                    ActivateEnemy(ctx, base, enemies[3], player, originAlong + offscreen + 1200.0f, originDepth - 2400.0f);
                }
                g_spawnPartner = false;
            }
            UpdateTarget(base, player);
            if (!Defeated(base,player) && CanWalk(base,player)) MovePlayer(ctx,base,player);
            TekkenForceTraceCamera("follow-input",base,RenderCamera(base),player,g_cameraTick,
                g_camera.eye.data(),g_camera.target.data());
            UpdateCameraFollow(base);
            ScheduleAttack(base, player, enemies);
            for (unsigned i = 0; i < enemies.size(); ++i)
            {
                const uint32_t enemy = enemies[i];
                if (!enemy) continue;
                if (g_entryMotion[i].fighter == enemy && g_enemies[i].entering &&
                    GRead32(base, enemy + kLife) == g_enemies[i].entryLife)
                {
                    // Practice can also reposition the CPU between battle ticks.
                    SetFloorPosition(ctx, base, enemy, g_entryMotion[i].x, g_entryMotion[i].z);
                }
                ReconcileWalk(ctx, base, enemy, g_motion[i + 1]);
                MoveOnFloor(ctx, base, enemy, 0.0f, 0.0f);
                UpdateEnemy(ctx, base, enemy, player, g_enemies[i], i);
                g_entryMotion[i] = {};
                if (g_enemies[i].entering)
                    g_entryMotion[i] = {enemy, ReadFloat(base, enemy), ReadFloat(base, enemy + 8), true};
            }
            SeparateEnemies(ctx,base,enemies);
            const uint32_t actors[] = {player,enemies[0],enemies[1],enemies[2],enemies[3]};
            int horizontal, depth;
            TekkenForceReadMoveAxes(horizontal,depth);
            if (!g_controls.freeMovement) depth=g_controls.depthMotion;
            for (unsigned i=0;i<g_walkMotion.size();++i)
            {
                const uint32_t actor=actors[i];
                g_walkMotion[i]={};
                if (!actor || Defeated(base,actor) || !CanWalk(base,actor)) continue;
                const bool moving=i ? g_enemies[i-1].walking : horizontal || depth;
                if (moving || NativeWalk(base,actor))
                    g_walkMotion[i]={actor,ReadFloat(base,actor),ReadFloat(base,actor+8),true,
                        GRead32(base,actor+kLife)};
            }
        }
    }
    __imp__sub_82320AC8(ctx, base);
}

PPC_FUNC(sub_822CDA88)
{
    const bool registerExtra = InPractice(base) && FullRosterRequested(base);
    const uint32_t saved = registerExtra ? GRead32(base, kFifthFighterSlot) : 0;
    if (registerExtra) GWrite32(base, kFifthFighterSlot, kPracticeExtra);
    __imp__sub_822CDA88(ctx, base);
    if (registerExtra) GWrite32(base, kFifthFighterSlot, saved);
    if (!InPractice(base) || !g_camera.ready) return;
    const auto enemies = Enemies(base);
    const uint32_t actors[] = { PlayerFighter(base), enemies[0], enemies[1], enemies[2], enemies[3] };
    for (unsigned i = 0; i < g_motion.size(); ++i)
    {
        const uint32_t fighter = actors[i];
        if (!fighter) continue;
        if (g_walkMotion[i].fighter == fighter && g_walkMotion[i].walking &&
            GRead32(base,fighter+kLife) == g_walkMotion[i].life && CanWalk(base,fighter))
        {
            // Arena steering owns walk displacement; the native cycle owns
            // the pose. Keep native attack lunges and hit/guard knockback.
            SetFloorPosition(ctx,base,fighter,g_walkMotion[i].x,g_walkMotion[i].z);
        }
        if (i && g_entryMotion[i - 1].fighter == fighter && g_entryMotion[i - 1].walking &&
            GRead32(base, fighter + kLife) == g_enemies[i - 1].entryLife && !Defeated(base, fighter))
        {
            // Native run root motion assumes a duel opponent and can snap to it.
            // Keep the run pose, but let arena movement own the entrance distance.
            SetFloorPosition(ctx, base, fighter, g_entryMotion[i - 1].x, g_entryMotion[i - 1].z);
        }
        g_motion[i].fighter = fighter;
        g_motion[i].x = ReadFloat(base, fighter);
        g_motion[i].z = ReadFloat(base, fighter + 8);
        g_motion[i].life = GRead32(base,fighter+kLife);
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
    const auto slot = std::find(enemies.begin(), enemies.end(), fighter);
    if (slot == enemies.end() || Defeated(base, fighter)) return;
    if (fighter == kPracticeExtra && GRead32(base, fighter + 212) >= 32)
    {
        // Drive the extra actor's recovery with its native floor moves.
        if (ExtraInMove(base, kCombotFaceUp)) CallOnFighter(ctx, base, __imp__sub_823165C8, fighter, kCombotRiseFaceUp);
        if (ExtraInMove(base, kCombotFaceDown)) CallOnFighter(ctx, base, __imp__sub_823165C8, fighter, kCombotRiseFaceDown);
    }
    if (!CanWalk(base, fighter)) return;
    auto& enemy = g_enemies[slot - enemies.begin()];
    GWrite32(base, held, GRead32(base, held) & ~(kAttackButtons | kDirectionButtons));
    GWrite32(base, pressed, GRead32(base, pressed) & ~(kAttackButtons | kDirectionButtons));
    if (enemy.strikePending && g_attackOwner == slot - enemies.begin())
    {
        enemy.strikePending = false;
        const uint32_t player = PlayerFighter(base);
        if (!player || Defeated(base, player)) return;
        FaceForStrike(base, fighter, player);
        if (fighter == kPracticeExtra)
        {
            CallOnFighter(ctx, base, __imp__sub_823165C8, fighter, kCombotJab);
            return;
        }
        GWrite32(base, held, GRead32(base, held) | kEnemyStrike);
        GWrite32(base, pressed, GRead32(base, pressed) | kEnemyStrike);
    }
    else if (enemy.walking)
    {
        if (enemy.entering)
        {
            const uint32_t moves = GRead32(base, GRead32(base, fighter + 136) + 416);
            const uint32_t move = GRead32(base, fighter + 280);
            if (move < moves + kCombotRun * 112 || move > moves + kCombotRunLast * 112)
                CallOnFighter(ctx, base, __imp__sub_823165C8, fighter, kCombotRun);
            return;
        }
        const uint32_t moves = GRead32(base, GRead32(base, fighter + 136) + 416);
        const uint32_t move = GRead32(base, fighter + 280);
        if (move >= moves + kCombotRun * 112 && move <= moves + kCombotRunLast * 112)
            CallOnFighter(ctx, base, __imp__sub_823165C8, fighter, kCombotApproach);
        if (fighter == kPracticeExtra)
        {
            if (!NativeWalk(base, fighter)) CallOnFighter(ctx, base, __imp__sub_823165C8, fighter, kCombotApproach);
            return;
        }
        // Practice CPU directions are relative to the fighter. This is forward.
        GWrite32(base, held, (GRead32(base, held) & ~kDirectionButtons) | kWalkRight);
    }
    else if (NativeWalk(base, fighter))
    {
        CallOnFighter(ctx, base, __imp__sub_823165C8, fighter, kCombotIdle);
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

// Several native paths build the render view directly, bypassing sub_82545EE0.
// Apply the same pose to every publication, not just the main camera update.
PPC_FUNC(sub_82541C90)
{
    const bool arena=InPractice(base) && g_camera.ready && ctx.r3.u32 == RenderCamera(base)+16;
    const uint32_t projection=ctx.r4.u32,view=ctx.r5.u32,vp=ctx.r6.u32;
    if (arena)
        ApplyCameraPose(base, ctx.r3.u32 - 16);
    __imp__sub_82541C90(ctx, base);
    if (arena)
    {
        if(TekkenForceStationaryCamera())
        {
            for(unsigned i=0;i<16;++i)
            {
                if(!g_camera.stationaryReady)
                {
                    g_camera.stationaryView[i]=GRead32(base,view+i*4);
                    g_camera.stationaryProjection[i]=GRead32(base,projection+i*4);
                    g_camera.stationaryVp[i]=GRead32(base,vp+i*4);
                }
                GWrite32(base,view+i*4,g_camera.stationaryView[i]);
                GWrite32(base,projection+i*4,g_camera.stationaryProjection[i]);
                GWrite32(base,vp+i*4,g_camera.stationaryVp[i]);
            }
            g_camera.stationaryReady=true;
        }
        TekkenForceTraceCamera("publish",base,RenderCamera(base),PlayerFighter(base),g_cameraTick,
            g_camera.eye.data(),g_camera.target.data());
    }
}

PPC_FUNC(sub_82542D18)
{
    const uint32_t fighter = ctx.r3.u32;
    if (InPractice(base) && g_camera.ready &&
        (fighter == PlayerFighter(base) || IsEnemy(base, fighter)))
    {
        if (fighter==PlayerFighter(base) && g_controls.freeMovement) FaceFreeMovement(base,fighter);
        const float angle = ReadFloat(base, fighter + 184);
        const float horizontal = std::sin(angle) * g_camera.rightX + std::cos(angle) * g_camera.rightZ;
        const uint32_t side = std::abs(horizontal) < 0.05f ? GRead32(base, fighter + 1024) : horizontal < 0.0f;
        GWrite32(base, fighter + 1020, side);
        GWrite32(base, fighter + 1024, side);
        ctx.r3.u32 = side;
        return;
    }
    __imp__sub_82542D18(ctx, base);
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
    if (lr==0x82303A1Cu) SamplePlayerControls();
    if (Defeated(base, fighter))
    {
        ctx.r3.u32 = 0;
        return;
    }
    if (g_fullRoster) ctx.r3.u32 &= ~kTagButton;
    if (lr != 0x82303A1Cu || !CanWalk(base, fighter)) return;
    int horizontal, depth;
    TekkenForceReadMoveAxes(horizontal, depth);
    if (!g_controls.freeMovement && (ctx.r3.u32 & kAttackButtons))
    {
        const uint32_t enemy = AttackTarget(base, fighter, horizontal, depth);
        if (enemy)
        {
            if (enemy != kPracticeExtra) GWrite32(base, fighter + kOpponent, enemy);
        }
        return;
    }
}

PPC_FUNC(sub_82303BB0)
{
    const uint32_t fighter = ctx.r3.u32;
    __imp__sub_82303BB0(ctx, base);
    if (!InPractice(base) || !g_camera.ready || fighter != PlayerFighter(base) || !CanWalk(base, fighter)) return;
    // Native command history stays intact. A double-tap depth gesture uses
    // walk execution; a single held vertical input retains crouch/jump.
    if (GRead32(base, fighter + 2064) & kAttackButtons) return;
    int horizontal, depth;
    TekkenForceReadMoveAxes(horizontal, depth);
    const int movementDepth=g_controls.freeMovement ? depth : g_controls.depthMotion;
    if (!g_controls.freeMovement && depth && !movementDepth && !g_controls.nativeVerticalHold)
    {
        GWrite32(base,fighter+2048,horizontal ?
            (1u<<(((horizontal>0)==(GRead32(base,fighter+1024)==0))?6:4)) : 32u);
        GWrite32(base,fighter+2052,0);
        return;
    }
    if (!horizontal && !movementDepth) return;
    if (!g_controls.freeMovement && g_controls.nativeVerticalHold && !movementDepth) return;
    const bool facingRight = GRead32(base, fighter + 1024) == 0;
    const unsigned direction = !horizontal || (horizontal > 0) == facingRight ? 6 : 4;
    GWrite32(base, fighter + 2048, 1u << direction);
    GWrite32(base, fighter + 2052, 0);
}

// The host overlay only hides the automated select flow. Enemy gauges use native HUD sprites.
namespace {
std::atomic<bool> g_blank{ false };
}

void TekkenForceBlankScreen(bool on)
{
    static const bool keep = [] { const char* e = std::getenv("TTT2_TF_NOBLANK"); return e && *e == '1'; }();
    g_blank.store(on && !keep, std::memory_order_relaxed);
}

void TekkenForcePublishOverlay(uint8_t* base)
{
    if (!Enabled()) return;
    static bool published = false;
    std::vector<GpuOverlayRect> rects;
    if (g_blank.load(std::memory_order_relaxed))
        rects.push_back({ 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f });
    else if (InPractice(base) && g_camera.ready && !g_controls.freeMovement && g_target && !Defeated(base, g_target) &&
             GRead32(base, g_target + kOnStage) && !Defeated(base, PlayerFighter(base)))
    {
        const auto& eye = g_camera.eye;
        const auto& target = g_camera.target;
        std::array<float, 3> forward{ target[0] - eye[0], target[1] - eye[1], target[2] - eye[2] };
        const float length = std::sqrt(forward[0]*forward[0] + forward[1]*forward[1] + forward[2]*forward[2]);
        for (float& value : forward) value /= length;
        const float planar = std::hypot(forward[0], forward[2]);
        const std::array<float,3> right{ -forward[2]/planar, 0.0f, forward[0]/planar };
        const std::array<float,3> up{ -right[2]*forward[1],
            right[2]*forward[0] - right[0]*forward[2], right[0]*forward[1] };
        std::array<float,3> marker{ReadFloat(base,g_target),ReadFloat(base,g_target+4)+2100.0f,
            ReadFloat(base,g_target+8)};
        float top=ReadFloat(base,g_target+4)+500.0f;
        for (unsigned i=0;i<14;++i)
        {
            const uint32_t hurt=g_target+2176+i*24;
            const float height=ReadFloat(base,hurt+4)+ReadFloat(base,hurt+12);
            if (std::isfinite(height) && height>top)
            {
                top=height;
                marker={ReadFloat(base,hurt),height+140.0f,ReadFloat(base,hurt+8)};
            }
        }
        const std::array<float,3> delta{marker[0]-eye[0],marker[1]-eye[1],marker[2]-eye[2]};
        const auto dot = [&](const auto& axis) {
            return delta[0]*axis[0]+delta[1]*axis[1]+delta[2]*axis[2];
        };
        const float depth = dot(forward);
        if (depth > 100.0f)
        {
            const float x = 0.5f + 0.5f * std::bit_cast<float>(g_camera.projection[0]) * dot(right) / depth;
            const float y = 0.5f - 0.5f * std::bit_cast<float>(g_camera.projection[5]) * dot(up) / depth;
            if (x > 0.025f && x < 0.975f && y > 0.13f && y < 0.90f)
            {
                rects.push_back({x-.0045f,y-.017f,.009f,.018f,.03f,.03f,.03f});
                rects.push_back({x-.0027f,y-.015f,.0054f,.017f,1.0f,.82f,.06f});
                for (int row=0; row<10; ++row)
                {
                    const float width = .024f * (1.0f-float(row)/10.0f);
                    rects.push_back({x-width*.5f,y+row*.0018f,width,.002f,.03f,.03f,.03f});
                    if (row<8)
                        rects.push_back({x-width*.5f+.002f,y+row*.0018f,width-.004f,.0018f,1.0f,.82f,.06f});
                }
            }
        }
    }
    if (rects.empty() && !published) return;
    published = !rects.empty();
    GpuSetOverlayRects(rects.data(), unsigned(rects.size()));
}

PPC_FUNC(sub_82331B50)
{
    // Replace the fifth actor's character before the battle loader generates its model command.
    if (Enabled() && ctx.r4.u32 == 4 && ctx.r5.u32 == kPracticeDummyFighter)
        ctx.r5.u32 = kCombotFighter;
    __imp__sub_82331B50(ctx, base);
}

PPC_FUNC(sub_82715898)
{
    const uint32_t slot = ctx.r3.u32, character = ctx.r4.u32;
    __imp__sub_82715898(ctx, base);
    if (!Enabled() || slot < 1 || slot > 4 || character != 49) return;
    // The native equipment loader copies palette zero for every actor, including duplicates.
    // Choose one of TU3's two registered Combot palettes before it requests each actor's resources.
    const uint32_t wardrobe = CallOnFighter(ctx, base, __imp__sub_827142C0, slot);
    if (!wardrobe) return;
    const uint32_t first = GRead32(base, wardrobe), end = GRead32(base, wardrobe + 4);
    for (uint32_t item = first; item < end; item += 44)
    {
        PPCContext color = ctx;
        color.r3.u32 = 0x82F320E8u; // TU3's native item and default costume palette table.
        color.r4.u32 = character;
        color.r5.u32 = GRead32(base, item);
        color.r6.u32 = slot % 2 == 0 ? 1 : 0;
        __imp__sub_8249F158(color, base);
        if (color.r3.s32 >= 0) GWrite32(base, item + 4, color.r3.u32);
    }
}

PPC_FUNC(sub_823D4690)
{
    // This is the native draw entry for one of the four roster health gauges.
    if (InPractice(base) && GRead32(base, ctx.r3.u32 + 16) != GRead32(base, kPracticeSettings + 100)) return;
    const uint32_t gauge = ctx.r3.u32;
    __imp__sub_823D4690(ctx, base);
    if (!InPractice(base) || !g_camera.ready || !g_fullRoster) return;
    // Reuse the native textures, bevel, gradient and damage fill, scaled by the sprite submission below.
    std::array<uint8_t, 144> saved;
    std::memcpy(saved.data(), base + gauge, saved.size());
    const uint32_t fifth = GRead32(base, kFifthFighterSlot);
    GWrite32(base, kFifthFighterSlot, kPracticeExtra);
    const auto enemies = Enemies(base);
    for (unsigned i = 0; i < enemies.size(); ++i)
    {
        const uint32_t actor = enemies[i];
        if (!actor || !GRead32(base, actor + kOnStage)) continue;
        const uint32_t slot = actor == kPracticeExtra ? 4 : GRead32(base, actor + 76);
        GWrite32(base, gauge + 8, slot);
        const float health = float(GRead32(base, actor + kLife)) / 65536.0f;
        const float maxHealth = std::max(1.0f, float(GRead32(base, actor + 108)) / 65536.0f);
        WriteFloat(base, gauge + 24, std::clamp(health / maxHealth, 0.0f, 1.0f));
        WriteFloat(base, gauge + 32, 0.0f);
        WriteFloat(base, gauge + 48, 0.0f);
        WriteFloat(base, gauge + 56, maxHealth);
        for (const unsigned offset : {60u, 84u, 104u, 120u}) GWrite32(base, gauge + offset, 0);
        g_drawingEnemyGauge = int(i);
        g_gaugeMirrored = (slot & 1) != 0;
        g_firstGaugeSprite = true;
        PPCContext draw = ctx;
        draw.r3.u32 = gauge;
        __imp__sub_823D4690(draw, base);
    }
    g_drawingEnemyGauge = -1;
    GWrite32(base, kFifthFighterSlot, fifth);
    std::memcpy(base + gauge, saved.data(), saved.size());
}

PPC_FUNC(sub_822CBD80)
{
    if (InPractice(base) && g_camera.ready && ctx.r3.u32==PlayerFighter(base) && g_controls.freeMovement)
    {
        const uint32_t player=ctx.r3.u32, saved=GRead32(base,player+kOpponent);
        FaceFreeMovement(base,player);
        GWrite32(base,player+kOpponent,0);
        ctx.r4.u32=0;
        __imp__sub_822CBD80(ctx,base);
        GWrite32(base,player+kOpponent,saved);
        return;
    }
    if (InPractice(base) && g_camera.ready && ctx.r3.u32 == PlayerFighter(base) &&
        g_target && !Defeated(base, g_target))
    {
        const uint32_t player = ctx.r3.u32, saved = GRead32(base, player + kOpponent);
        GWrite32(base, player + kOpponent, g_target);
        if (CanWalk(base, player)) FaceForStrike(base, player, g_target);
        __imp__sub_822CBD80(ctx, base);
        GWrite32(base, player + kOpponent, saved);
        return;
    }
    if (InPractice(base) && g_camera.ready && IsEnemy(base, ctx.r3.u32))
    {
        const uint32_t player = PlayerFighter(base);
        GWrite32(base, ctx.r3.u32 + kOpponent, player);
        if (CanWalk(base, ctx.r3.u32)) FaceForStrike(base, ctx.r3.u32, player);
        ctx.r4.u32 = player;
    }
    __imp__sub_822CBD80(ctx, base);
}

uint16_t TekkenForceFilterPadButtons(uint8_t* base, uint16_t buttons, int16_t x, int16_t y)
{
    if (!InPractice(base) || !g_camera.ready) return buttons;
    buttons &= ~uint16_t(0x0200u);
    if (buttons & 0x0020u) return buttons; // Back + A remains the native reset chord.
    constexpr int deadzone=7849; // XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, as arena movement uses.
    const bool left=(buttons&4u)||x < -deadzone, right=(buttons&8u)||x > deadzone;
    const bool up=(buttons&1u)||y > deadzone, down=(buttons&2u)||y < -deadzone;
    buttons &= ~uint16_t(0x000Fu);
    if (left != right) buttons |= left ? 4u : 8u;
    if (up != down) buttons |= up ? 1u : 2u;
    return buttons;
}

// Canonicalize one melee pair only while the native capsule test runs. Keep
// radial reach, vertical hit levels, hit frames and native damage unchanged.
// No render pose or actor position escapes the temporary query.
PPC_FUNC(sub_822CB9F0)
{
    const uint32_t attacker=ctx.r3.u32, defender=ctx.r4.u32, output=ctx.r5.u32;
    const uint32_t count=ctx.r6.u32, shapes=ctx.r7.u32;
    if (!EncounterPair(base,attacker,defender) || !shapes || !count || count>4)
    {
        __imp__sub_822CB9F0(ctx,base);
        return;
    }
    const auto trace = [&](const char* path) {
        static unsigned shown=0;
        if (!std::getenv("TTT2_TF_DIAG") || shown++>=500) return;
        const uint32_t player=PlayerFighter(base), enemy=attacker==player?defender:attacker;
        const float dx=ReadFloat(base,enemy)-ReadFloat(base,player);
        const float dz=ReadFloat(base,enemy+8)-ReadFloat(base,player+8);
        const float angle=std::atan2(FloorDepth(dx,dz),dx*g_camera.rightX+dz*g_camera.rightZ);
        std::fprintf(stderr,"[tf-contact] %s attacker=%08X defender=%08X angle=%.1f radius=%.1f\n",
            path,attacker,defender,angle*180.0f/std::numbers::pi_v<float>,std::hypot(dx,dz));
    };
    // Preserve every hit the original 3D query already accepts. The side-view
    // query extends angular coverage and never replaces a valid native hit.
    PPCContext native=ctx;
    __imp__sub_822CB9F0(native,base);
    if (native.r3.u32)
    {
        trace("native");
        ctx=native;
        return;
    }
    const float ax=ReadFloat(base,attacker), az=ReadFloat(base,attacker+8);
    const float bx=ReadFloat(base,defender), bz=ReadFloat(base,defender+8);
    const float dx=bx-ax, dz=bz-az, radius=std::hypot(dx,dz);
    if (radius < 1.0f || !std::isfinite(radius))
    {
        __imp__sub_822CB9F0(ctx,base);
        return;
    }
    const auto facing = [&](uint32_t actor) {
        const float angle=ReadFloat(base,actor+184);
        const float dot=std::sin(angle)*g_camera.rightX+std::cos(angle)*g_camera.rightZ;
        return std::abs(dot)<.05f ? (GRead32(base,actor+1024) ? -1.0f:1.0f) : (dot>0.0f?1.0f:-1.0f);
    };
    const float along=dx*g_camera.rightX+dz*g_camera.rightZ;
    const float side=std::abs(along)<1.0f ? facing(attacker) : (along>0.0f?1.0f:-1.0f);
    const float vx=ax+side*radius*g_camera.rightX, vz=az+side*radius*g_camera.rightZ;
    std::array<uint8_t,96> savedAttacks{};
    std::array<uint8_t,336> savedHurt{};
    const uint32_t savedX=GRead32(base,defender), savedZ=GRead32(base,defender+8);
    std::memcpy(savedAttacks.data(),base+shapes,count*24);
    std::memcpy(savedHurt.data(),base+defender+2176,savedHurt.size());
    const auto transform = [&](uint32_t point,uint32_t actor,float ox,float oz,float nx,float nz,float sign) {
        const float angle=ReadFloat(base,actor+184);
        const float fx=std::sin(angle), fz=std::cos(angle);
        const float px=ReadFloat(base,point)-ox, pz=ReadFloat(base,point+8)-oz;
        const float forward=px*fx+pz*fz, across=-px*fz+pz*fx;
        WriteFloat(base,point,nx+sign*(forward*g_camera.rightX-across*g_camera.rightZ));
        WriteFloat(base,point+8,nz+sign*(forward*g_camera.rightZ+across*g_camera.rightX));
    };
    for (unsigned i=0;i<count;++i)
    {
        transform(shapes+i*24,attacker,ax,az,ax,az,facing(attacker));
        transform(shapes+i*24+12,attacker,ax,az,ax,az,facing(attacker));
    }
    // Hurt records are centers plus extents, not two endpoints. Preserve radii.
    // Use a consistent opposing hurt pose in the side-view query. Otherwise a
    // target leaning toward another enemy moves its body out of jab reach.
    for (unsigned i=0;i<14;++i) transform(defender+2176+i*24,defender,bx,bz,vx,vz,-side);
    WriteFloat(base,defender,vx); WriteFloat(base,defender+8,vz);
    __imp__sub_822CB9F0(ctx,base);
    std::memcpy(base+shapes,savedAttacks.data(),count*24);
    std::memcpy(base+defender+2176,savedHurt.data(),savedHurt.size());
    GWrite32(base,defender,savedX); GWrite32(base,defender+8,savedZ);
    if (ctx.r3.u32 && output)
    {
        trace("side");
        const float ux=dx/radius, uz=dz/radius;
        const auto unproject = [&](uint32_t point,bool vector) {
            const float px=ReadFloat(base,point)-(vector?0.0f:ax);
            const float pz=ReadFloat(base,point+8)-(vector?0.0f:az);
            const float forward=side*(px*g_camera.rightX+pz*g_camera.rightZ);
            const float across=side*(-px*g_camera.rightZ+pz*g_camera.rightX);
            WriteFloat(base,point,(vector?0.0f:ax)+forward*ux-across*uz);
            WriteFloat(base,point+8,(vector?0.0f:az)+forward*uz+across*ux);
        };
        unproject(output,false);
        if (!(GRead32(base,attacker+936)&0x1000u)) unproject(output+12,true);
    }
}

PPC_FUNC(sub_82314EA0)
{
    const uint32_t fighter = ctx.r3.u32;
    __imp__sub_82314EA0(ctx, base);
    if (!InPractice(base) || !g_camera.ready) return;
    if (fighter==PlayerFighter(base))
    {
        if (CanWalk(base,fighter) && !(TekkenForceReadButtons()&0xF000u) &&
            (g_controls.freeMovement || !g_controls.nativeVerticalHold) && ctx.r3.u32)
        {
            const uint32_t name=GRead32(base,ctx.r3.u32);
            const char* text=name?reinterpret_cast<const char*>(base+name):"";
            if (std::strstr(text,"sYOKE") || std::strstr(text,"sJUMP") ||
                std::strstr(text,"sSTEP") || std::strstr(text,"sWALK") ||
                std::strstr(text,"cKAM") || std::strstr(text,"cWALK"))
            {
                int horizontal,depth;
                TekkenForceReadMoveAxes(horizontal,depth);
                if (!g_controls.freeMovement) depth=g_controls.depthMotion;
                const uint32_t moveset=GRead32(base,fighter+136);
                const uint32_t neutral=GRead16(base,moveset+138);
                ctx.r3.u32=GRead32(base,moveset+416)+(neutral+((horizontal||depth)?1:0))*112;
            }
        }
        return;
    }
    if (!IsEnemy(base,fighter)) return;
    const uint32_t table = GRead32(base, GRead32(base, fighter + 136) + 416);
    // Native running can branch into an automatic tackle/chop without an attack input.
    // Entrance animations must never bypass the arena's exclusive attack turn.
    const uint32_t move = (ctx.r3.u32 - table) / 112;
    if (move == 144 || move == 146 || move == 285 || move == 286)
    {
        const auto enemies = Enemies(base);
        const auto slot = std::find(enemies.begin(), enemies.end(), fighter);
        const bool entering = g_enemies[slot - enemies.begin()].entering;
        ctx.r3.u32 = table + (entering ? kCombotRun : kCombotIdle) * 112;
    }
}

PPC_FUNC(sub_822A8160)
{
    const uint32_t sprite = ctx.r3.u32;
    std::array<uint32_t, 4> saved{};
    constexpr unsigned offsets[] = {0, 4, 32, 36};
    if (g_drawingEnemyGauge >= 0)
    {
        for (unsigned i = 0; i < saved.size(); ++i) saved[i] = GRead32(base, sprite + offsets[i]);
        static const bool diag = std::getenv("TTT2_TF_DIAG") != nullptr;
        static unsigned logged = 0;
        if (diag && logged++ < 16)
            std::fprintf(stderr, "[tf-gauge] enemy=%d mirror=%d x=%.1f y=%.1f w=%.1f h=%.1f sx=%.2f sy=%.2f\n",
                g_drawingEnemyGauge, g_gaugeMirrored, ReadFloat(base,sprite), ReadFloat(base,sprite+4),
                ReadFloat(base,sprite+24), ReadFloat(base,sprite+28),ReadFloat(base,sprite+32),ReadFloat(base,sprite+36));
        if (g_firstGaugeSprite)
        {
            g_firstGaugeSprite = false;
            g_gaugeOriginX = ReadFloat(base, sprite);
            g_gaugeOriginY = ReadFloat(base, sprite + 4);
            g_gaugeWidth = ReadFloat(base, sprite + 24) * ReadFloat(base, sprite + 32);
        }
        const float localX = g_gaugeMirrored ?
            g_gaugeOriginX + g_gaugeWidth - ReadFloat(base, sprite) : ReadFloat(base, sprite) - g_gaugeOriginX;
        WriteFloat(base, sprite, 92.0f + 294.0f * g_drawingEnemyGauge + localX * 0.48f);
        WriteFloat(base, sprite + 4, 672.0f + (ReadFloat(base, sprite + 4) - g_gaugeOriginY) * 0.60f);
        WriteFloat(base, sprite + 32, ReadFloat(base, sprite + 32) * (g_gaugeMirrored ? -0.48f : 0.48f));
        WriteFloat(base, sprite + 36, ReadFloat(base, sprite + 36) * 0.60f);
    }
    __imp__sub_822A8160(ctx, base);
    if (g_drawingEnemyGauge >= 0)
        for (unsigned i = 0; i < saved.size(); ++i) GWrite32(base, sprite + offsets[i], saved[i]);
}

PPC_FUNC(sub_823D4D38)
{
    if (InPractice(base))
    {
        const uint32_t cpuSide = OpponentSide(base);
        const bool portrait = ctx.lr == 0x823D56B4u && ctx.r5.u32 == 7 - cpuSide;
        const bool name = ctx.lr == 0x823D6964u && ctx.r5.u32 == 35 + cpuSide;
        const bool cpuLabel = ctx.lr == 0x823D6C64u && ctx.r5.u32 == 39 + cpuSide;
        if (portrait || name || cpuLabel) return;
    }
    __imp__sub_823D4D38(ctx, base);
}

PPC_FUNC(sub_824C75F8)
{
    const uint32_t slot = ctx.r4.u32, request = ctx.r5.u32;
    __imp__sub_824C75F8(ctx, base);
    if (Enabled() && slot == 4 && !ctx.r3.u32 && GRead32(base, kPracticeExtra + 88) == kCombotFighter)
    {
        // The extra Practice slot has no registered costume resources for these component pairs.
        // Let the native queue retire the missing requests so the base Combot can finish loading.
        const uint32_t kind = GRead32(base, request + 44), variant = GRead32(base, request + 48);
        if ((kind == 66 && variant == 0) || (kind == 51 && variant == 52) || (kind == 71 && variant == 72))
            ctx.r3.u32 = 1;
    }
}
