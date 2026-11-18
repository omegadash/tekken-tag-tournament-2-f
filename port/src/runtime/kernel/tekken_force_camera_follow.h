#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tekken_force {
// Track arena walking and settled combat displacement. A single-frame native
// root spike is not a travel reference; target selection alone supplies none.
struct HorizontalCameraFollow
{
    static constexpr float kHalfDeadZone = 700.0f;
    static constexpr float kMaxStep = 48.0f;
    static constexpr float kMaxAcceleration = 1.5f;
    float center = 0.0f, reference = 0.0f;
    float velocity = 0.0f;
    float candidate = 0.0f;
    unsigned stableSamples = 0;
    uint64_t lastTick = ~uint64_t(0);
    uint64_t lastCombatTick = ~uint64_t(0);

    void Reset(float along)
    {
        center = reference = along;
        velocity = 0.0f;
        candidate=along;
        stableSamples=0;
        lastTick = ~uint64_t(0);
        lastCombatTick = ~uint64_t(0);
    }
    void ObserveWalk(float before, float after)
    {
        if (std::isfinite(after) && std::abs(after-before) > 0.001f)
        {
            reference=candidate=after;
            stableSamples=2;
        }
    }
    void ObserveCombat(float along,uint64_t tick)
    {
        if(tick==lastCombatTick || !std::isfinite(along)) return;
        lastCombatTick=tick;
        stableSamples=std::abs(along-candidate)<=300.0f ? std::min(2u,stableSamples+1) : 1u;
        candidate=along;
        if(stableSamples>=2) reference=along;
    }
    float Update(uint64_t tick)
    {
        if (tick == lastTick) return 0.0f;
        lastTick = tick;
        const float offset = reference-center;
        const float excess = offset > kHalfDeadZone ? offset-kHalfDeadZone :
            offset < -kHalfDeadZone ? offset+kHalfDeadZone : 0.0f;
        // Begin braking while there is still room for the following ticks;
        // clipping only at the boundary would itself introduce a sharp stop.
        const float distance=std::abs(excess);
        const float brakingTicks=std::max(1.0f,std::ceil(
            (std::sqrt(1.0f+8.0f*distance/kMaxAcceleration)-1.0f)*0.5f));
        const float brakingSpeed=(distance+kMaxAcceleration*brakingTicks*(brakingTicks-1.0f)*0.5f)/brakingTicks;
        const float speed=std::min({distance*0.20f,kMaxStep,brakingSpeed});
        const float desired=std::copysign(speed,excess);
        velocity += std::clamp(desired-velocity,-kMaxAcceleration,kMaxAcceleration);
        center += velocity;
        return velocity;
    }
};
}
