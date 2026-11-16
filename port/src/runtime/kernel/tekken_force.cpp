#include "kernel_shared.h"
#include <cstdlib>
#include <cstring>

extern "C" PPC_FUNC(__imp__sub_823A70F0);

// Practice initializes its settings here and then copies them to the pause menu.
PPC_FUNC(sub_823A70F0)
{
    const uint32_t settings = ctx.r3.u32;
    __imp__sub_823A70F0(ctx, base);

    const char* enabled = std::getenv("TTT2_TEKKEN_FORCE");
    if (!enabled || std::strcmp(enabled, "1") != 0) return;

    // TU3's own Battle Position selector (sub_823AD198) stores 1 for Default
    // Position and sets the position-reset bit on the Practice controller.
    constexpr uint32_t kBattlePosition = 24;
    constexpr uint32_t kPracticeFlags = 0x82DD6034u;
    constexpr uint32_t kPositionReset = 0x00200000u;
    GWrite32(base, settings + kBattlePosition, 1);
    GWrite32(base, kPracticeFlags, GRead32(base, kPracticeFlags) | kPositionReset);
}
