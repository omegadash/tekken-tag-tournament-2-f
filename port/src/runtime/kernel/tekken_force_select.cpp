// Tekken Force Test: the player chooses one fighter and the rest of the Practice setup is made for them.
//
// The test reuses the game's own Practice character select (one CharaSelectMode, a cursor per side, a stage grid on the
// select controller). This file drives that screen the way a player would, through the port's synthetic presses:
//   * the TAG / SOLO window is answered (TAG: the roster actors need the second slot of each side), and the player's own
//     pick is the only thing they see;
//   * the player's partner and the CPU side's two picks are Combot (distinct costumes), the stage is Tulip Festival;
//   * the screen is held black from the moment the player has picked until the match is on, so no other select or stage
//     screen shows (TekkenForceBlankScreen in kernel_video.cpp).
// The cursors, the grids and the pick rules are the game's: a side may not take a fighter twice, so the Combot's bit is
// given back to the side's free list before each repeated pick.
#include "kernel_shared.h"
#include "online_2v2.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

void TekkenForceBlankScreen(bool on);   // kernel_video.cpp

namespace online2v2 {
uint32_t SelectCursor(int side);
uint32_t SelectController();
bool Selecting();
}

namespace {
constexpr uint32_t kGrids = 0x8200C8BCu;          // {table, count} per grid: the character grid is [controller + 76]
constexpr uint32_t kStageGrid = 4;
constexpr int kCombot = 49;                       // the roster's Combot (character grid cell 53)
constexpr int kTulipFestival = 46;                // the stage id of the Tulip Festival cell (stage grid cell 26)
constexpr uint16_t kUp = 0x0001, kDown = 0x0002, kLeft = 0x0004, kRight = 0x0008;
constexpr uint16_t kA = 0x1000, kB = 0x2000, kX = 0x4000, kY = 0x8000;
// The confirm button of each automatic pick: the game picks a costume with it. Player's partner, CPU lead, CPU partner.
constexpr uint16_t kCostumeButtons[3] = { kA, kA, kA };

// Cursor object fields (the same ones online_2v2_pick.cpp uses).
constexpr uint32_t kCell = 0x38, kFreeHigh = 0x148, kPicks = 0x154, kTurn = 0x158, kState = 312;
constexpr uint32_t kPicking = 19, kSideOnePicking = 33, kPlayStyleWindow = 2, kSideOnePlayStyleWindow = 32;
// Select controller fields: the stage phase (1: choosing) and the stage cursor.
constexpr uint32_t kStagePhase = 0x138, kStageCursor = 0x12C;

bool TfOn()
{
    static const bool on = [] { const char* e = std::getenv("TTT2_TEKKEN_FORCE"); return e && std::strcmp(e, "1") == 0; }();
    return on;
}

bool Diag()
{
    static const bool on = [] { const char* e = std::getenv("TTT2_TF_SELDIAG"); return e && *e == '1'; }();
    return on;
}

int64_t Ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

int GridCell(uint8_t* base, uint32_t grid, uint32_t field, int value)
{
    const uint32_t table = GRead32(base, kGrids + 8 * grid), count = GRead32(base, kGrids + 8 * grid + 4);
    for (uint32_t c = 0; table && c < count && c < 64; ++c)
        if (int32_t(GRead32(base, table + 32 * c + field)) == value) return int(c);
    return -1;
}

// The first press of the shortest path through the grid's neighbour table (up, down, left, right at +8).
uint16_t GridStep(uint8_t* base, uint32_t grid, int from, int to)
{
    const uint32_t table = GRead32(base, kGrids + 8 * grid);
    const int count = int((std::min)(GRead32(base, kGrids + 8 * grid + 4), 64u));
    if (!table || from < 0 || to < 0 || from >= count || to >= count) return 0;
    constexpr uint16_t kDirs[4] = { kUp, kDown, kLeft, kRight };
    uint16_t first[64] = {};
    int queue[64], head = 0, tail = 0;
    bool seen[64] = {};
    seen[from] = true;
    queue[tail++] = from;
    while (head < tail)
    {
        const int c = queue[head++];
        if (c == to) return first[c];
        for (int d = 0; d < 4; ++d)
        {
            const int n = int(GRead32(base, table + 32 * uint32_t(c) + 8 + 4 * uint32_t(d)));
            if (n < 0 || n >= count || seen[n]) continue;
            seen[n] = true;
            first[n] = c == from ? kDirs[d] : first[c];
            queue[tail++] = n;
        }
    }
    return 0;
}

enum Phase { kOff, kWindow, kUser, kPartner, kCpuLead, kCpuPartner, kStage, kDone, kGaveUp };
Phase g_phase = kOff;
bool g_wasSelecting = false;
int64_t g_pressAt = 0, g_phaseAt = 0;
int g_stageConfirmed = 0;

void Press(uint16_t buttons)
{
    online2v2::SynthPress(0, buttons, 70, 0);
    g_pressAt = Ms();
}

void SetPhase(Phase p)
{
    g_phase = p;
    g_phaseAt = Ms();
    if (Diag()) std::fprintf(stderr, "[tf-select] phase %d\n", int(p));
}

// One Combot pick on a side's cursor: go to its cell, give the side's free list the bit back, confirm.
void PickCombot(uint8_t* base, uint32_t controller, uint32_t cursor, uint16_t button, int64_t now)
{
    const int target = GridCell(base, GRead32(base, controller + 76), 0, kCombot);
    if (target < 0) return;
    constexpr int kBit = kCombot - 32;
    GWrite32(base, cursor + kFreeHigh, GRead32(base, cursor + kFreeHigh) | (1u << kBit));
    if (now - g_pressAt < 140) return;
    const int cell = int32_t(GRead32(base, cursor + kCell));
    if (cell != target)
    {
        if (const uint16_t step = GridStep(base, GRead32(base, controller + 76), cell, target)) Press(step);
        return;
    }
    Press(button);
}
}

void TekkenForceSelectFrame(PPCContext& ctx, uint8_t* base)
{
    (void)ctx;
    if (!TfOn()) return;
    const bool selecting = online2v2::Selecting();
    if (selecting != g_wasSelecting)
    {
        g_wasSelecting = selecting;
        if (selecting)
        {
            g_stageConfirmed = 0;
            SetPhase(kWindow);
            TekkenForceBlankScreen(true);
        }
        else if (g_phase != kDone)
        {
            // Left the select without starting a match (the player backed out).
            SetPhase(kOff);
            TekkenForceBlankScreen(false);
        }
    }
    if (!selecting) return;
    const uint32_t c = online2v2::SelectController();
    const uint32_t s0 = online2v2::SelectCursor(0), s1 = online2v2::SelectCursor(1);
    if (!c || !s0 || !s1) return;
    const int64_t now = Ms();

    if (Diag())
    {
        static uint32_t last[16] = {};
        const uint32_t cur[16] = { GRead32(base, c + kStagePhase), GRead32(base, c + kStageCursor), GRead32(base, s0 + kState),
                                   GRead32(base, s0 + kCell), GRead32(base, s0 + kPicks), GRead32(base, s0 + kTurn),
                                   GRead32(base, s1 + kState), GRead32(base, s1 + kCell), GRead32(base, s1 + kPicks),
                                   GRead32(base, s1 + kTurn) };
        if (std::memcmp(cur, last, sizeof(cur)) != 0)
        {
            std::memcpy(last, cur, sizeof(cur));
            std::fprintf(stderr, "[tf-sel] phase=%d stage=%u/%u | s0 st=%u cell=%u picks=%u turn=%u | s1 st=%u cell=%u picks=%u turn=%u\n",
                         int(g_phase), cur[0], cur[1], cur[2], cur[3], cur[4], cur[5], cur[6], cur[7], cur[8], cur[9]);
        }
        static uint32_t lastPicks[2] = {};
        for (int s = 0; s < 2; ++s)
        {
            const uint32_t k = s ? s1 : s0, picks = GRead32(base, k + kPicks);
            if (picks == lastPicks[s]) continue;
            lastPicks[s] = picks;
            std::string line;
            for (uint32_t o = 0x140; o < 0x1C0; o += 4)
            {
                char b[32];
                std::snprintf(b, sizeof(b), " %X=%X", o, GRead32(base, k + o));
                line += b;
            }
            std::fprintf(stderr, "[tf-pick] side %d picks=%u%s\n", s, picks, line.c_str());
        }
    }

    // A pick that has not gone through in 25 s stops the automation: the player finishes the select themself.
    if (g_phase >= kPartner && g_phase < kDone && now - g_phaseAt > 25000)
    {
        std::fprintf(stderr, "[tf-select] gave up in phase %d\n", int(g_phase));
        g_phase = kGaveUp;
        TekkenForceBlankScreen(false);
    }

    switch (g_phase)
    {
    case kWindow:
        // The side's TAG / SOLO / PAIR window: TAG is the highlighted choice.
        if (GRead32(base, s0 + kState) == kPicking && GRead32(base, s0 + kPicks) == 0)
        {
            TekkenForceBlankScreen(false);
            SetPhase(kUser);
        }
        else if (GRead32(base, s0 + kState) == kPlayStyleWindow && now - g_pressAt > 450)
            Press(kA);
        break;
    case kUser:
        // Test hook: TTT2_TF_AUTOUSER=<ms> confirms the player's pick by itself after that long.
        if (static const long auto_ms = [] { const char* e = std::getenv("TTT2_TF_AUTOUSER"); return e ? std::atol(e) : 0L; }();
            auto_ms > 0 && now - g_phaseAt > auto_ms && now - g_pressAt > 1500 && GRead32(base, s0 + kPicks) == 0)
            Press(kA);
        if (GRead32(base, s0 + kPicks) >= 1)
        {
            TekkenForceBlankScreen(true);
            SetPhase(kPartner);
        }
        break;
    case kPartner:
        if (GRead32(base, s0 + kPicks) >= 2) { SetPhase(kCpuLead); break; }
        if (GRead32(base, s0 + kPicks) == 1 && GRead32(base, s0 + kTurn) == 1 && GRead32(base, s0 + kState) == kPicking)
            PickCombot(base, c, s0, kCostumeButtons[0], now);
        break;
    case kCpuLead:
        if (GRead32(base, s1 + kPicks) >= 1) { SetPhase(kCpuPartner); break; }
        if (GRead32(base, s1 + kState) == kSideOnePlayStyleWindow && now - g_pressAt > 450) Press(kA);
        else if (GRead32(base, s1 + kState) == kSideOnePicking) PickCombot(base, c, s1, kCostumeButtons[1], now);
        break;
    case kCpuPartner:
        if (GRead32(base, s1 + kPicks) >= 2) { SetPhase(kStage); break; }
        if (GRead32(base, s1 + kPicks) == 1 && GRead32(base, s1 + kState) == kSideOnePicking)
            PickCombot(base, c, s1, kCostumeButtons[2], now);
        break;
    case kStage:
        if (GRead32(base, c + kStagePhase) == 1 && now - g_pressAt > 160)
        {
            const int target = GridCell(base, kStageGrid, 4, kTulipFestival);
            const int cell = int32_t(GRead32(base, c + kStageCursor));
            if (target < 0) break;
            if (cell != target)
            {
                if (const uint16_t step = GridStep(base, kStageGrid, cell, target)) Press(step);
            }
            else if (++g_stageConfirmed > 2)
            {
                Press(kA);
                SetPhase(kDone);
            }
        }
        break;
    default:
        break;
    }
}
