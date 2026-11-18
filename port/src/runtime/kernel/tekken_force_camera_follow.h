#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tekken_force {
// The camera observes committed arena walking, not target-relative native
// combat roots. Selection, attacks and native pose corrections are not travel.
struct HorizontalCameraFollow
{
    static constexpr float kHalfDeadZone = 1000.0f;
    static constexpr float kMaxStep = 24.0f;
    float center = 0.0f, reference = 0.0f;
    uint64_t lastTick = ~uint64_t(0);

    void Reset(float along)
    {
        center = reference = along;
        lastTick = ~uint64_t(0);
    }
    void ObserveWalk(float before, float after)
    {
        if (std::isfinite(after) && std::abs(after-before) > 0.001f) reference = after;
    }
    float Update(uint64_t tick)
    {
        if (tick == lastTick) return 0.0f;
        lastTick = tick;
        const float offset = reference-center;
        const float excess = offset > kHalfDeadZone ? offset-kHalfDeadZone :
            offset < -kHalfDeadZone ? offset+kHalfDeadZone : 0.0f;
        const float step = std::clamp(excess*0.20f,-kMaxStep,kMaxStep);
        center += step;
        return step;
    }
};
}
