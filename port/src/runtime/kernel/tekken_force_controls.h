#pragma once
#include <cstdint>

namespace tekken_force {
enum class TargetAction { None, Cycle, Nearest, Release };

// Host-time gesture recognition. Sampling twice in one game frame is safe:
// edges and deadlines, rather than a per-call counter, drive every gesture.
struct PlayerControls
{
    static constexpr uint64_t kDoubleTapMs = 250;
    static constexpr uint64_t kStepMs = 180;
    static constexpr uint64_t kTargetHoldMs = 300;

    bool freeMovement = false;
    int depthMotion = 0;
    bool nativeVerticalHold = false;

    TargetAction Sample(uint64_t now, bool rb, int depth, bool attack)
    {
        TargetAction action = TargetAction::None;
        if (rb && !rbHeld_) { rbAt_ = now; holdUsed_ = false; }
        if (rb && !holdUsed_ && now - rbAt_ >= kTargetHoldMs)
        {
            freeMovement = true;
            holdUsed_ = true;
            action = TargetAction::Release;
        }
        if (!rb && rbHeld_ && !holdUsed_)
        {
            if (now - rbAt_ >= kTargetHoldMs)
            {
                freeMovement = true;
                action = TargetAction::Release;
            }
            else if (freeMovement)
            {
                freeMovement = false;
                action = TargetAction::Nearest;
            }
            else action = TargetAction::Cycle;
        }
        rbHeld_ = rb;
        if (action != TargetAction::None) ClearDepth();

        if (freeMovement)
        {
            depthMotion = depth;
            nativeVerticalHold = false;
            return action;
        }
        if (depth != previousDepth_)
        {
            if (previousDepth_ && tapDirection_ == previousDepth_)
                released_ = now - tapAt_ <= kDoubleTapMs;
            if (depth)
            {
                if (stepDirection_ && depth != stepDirection_) stepDirection_ = 0;
                if (!attack && released_ && tapDirection_ == depth && now - tapAt_ <= kDoubleTapMs)
                {
                    stepDirection_ = depth;
                    stepUntil_ = now + kStepMs;
                    tapDirection_ = 0;
                    released_ = false;
                }
                else
                {
                    tapDirection_ = attack ? 0 : depth;
                    tapAt_ = now;
                    released_ = false;
                }
                heldAt_ = now;
            }
            previousDepth_ = depth;
        }
        if (attack)
        {
            tapDirection_ = stepDirection_ = 0;
            released_ = false;
        }
        if (stepDirection_ && depth != stepDirection_ && now >= stepUntil_) stepDirection_ = 0;
        depthMotion = stepDirection_;
        nativeVerticalHold = depth && !depthMotion && now - heldAt_ >= kDoubleTapMs;
        return action;
    }

private:
    bool rbHeld_ = false, holdUsed_ = false, released_ = false;
    int previousDepth_ = 0, tapDirection_ = 0, stepDirection_ = 0;
    uint64_t rbAt_ = 0, tapAt_ = 0, heldAt_ = 0, stepUntil_ = 0;

    void ClearDepth()
    {
        previousDepth_ = tapDirection_ = stepDirection_ = depthMotion = 0;
        released_ = nativeVerticalHold = false;
        tapAt_ = heldAt_ = stepUntil_ = 0;
    }
};
}
