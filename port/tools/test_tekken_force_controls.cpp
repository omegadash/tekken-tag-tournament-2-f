#include "../src/runtime/kernel/tekken_force_controls.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

using tekken_force::PlayerControls;
using tekken_force::TargetAction;

int main()
{
    for (int direction : {-1, 1})
    {
        PlayerControls c;
        c.Sample(1000, false, direction, false);
        c.Sample(1060, false, 0, false);
        assert(c.depthMotion == 0); // One tap never moves into depth.
        c.Sample(1150, false, direction, false);
        assert(c.depthMotion == direction);
        c.Sample(1190, false, 0, false);
        assert(c.depthMotion == direction); // A short second tap completes a step.
        c.Sample(1331, false, 0, false);
        assert(c.depthMotion == 0);

        c = {};
        c.Sample(2000, false, direction, false);
        c.Sample(2050, false, 0, false);
        c.Sample(2150, false, direction, false);
        c.Sample(3200, false, direction, false);
        assert(c.depthMotion == direction); // Holding tap two keeps walking.
        assert(!c.nativeVerticalHold);
        c.Sample(3250, false, 0, false);
        assert(c.depthMotion == 0);

        c = {};
        c.Sample(4000, false, direction, false);
        c.Sample(4300, false, direction, false);
        assert(c.depthMotion == 0 && c.nativeVerticalHold);
        c.Sample(4350, false, 0, false);
        c.Sample(4400, false, direction, false);
        assert(c.depthMotion == 0); // A held first press is not tap one.

        c = {};
        c.Sample(5000, false, direction, false);
        c.Sample(5050, false, 0, false);
        c.Sample(5150, false, direction, true);
        assert(c.depthMotion == 0); // Directional attacks don't initiate steps.

        c = {};
        c.Sample(6000, false, direction, false);
        c.Sample(6060, false, 0, false);
        c.Sample(6251, false, direction, false);
        assert(c.depthMotion == 0); // A late second tap starts a new gesture.
        c.Sample(6300, false, 0, false);
        c.Sample(6380, false, -direction, false);
        assert(c.depthMotion == 0); // Up then down is not a double tap.
        c.Sample(6420, false, 0, false);
        c.Sample(6490, false, -direction, false);
        assert(c.depthMotion == -direction);
        c.Sample(6500, false, -direction, true);
        assert(c.depthMotion == 0); // An attack cancels an ongoing step.
    }
    PlayerControls c;
    c.Sample(1000, true, 0, false);
    assert(c.Sample(1080, false, 0, false) == TargetAction::Cycle);
    assert(!c.freeMovement);
    c.Sample(1200, true, 0, false);
    assert(c.Sample(1500, true, 0, false) == TargetAction::Release);
    assert(c.freeMovement);
    assert(c.Sample(1700, true, 1, false) == TargetAction::None);
    assert(c.depthMotion == 1);
    assert(c.Sample(1700, true, 1, false) == TargetAction::None);
    assert(c.depthMotion == 1); // Multiple readers cannot repeat an RB action.
    assert(c.Sample(1800, false, 1, false) == TargetAction::None);
    assert(c.freeMovement); // Releasing the hold doesn't reacquire a target.
    c.Sample(2000, true, 0, false);
    assert(c.Sample(2080, false, 0, false) == TargetAction::Nearest);
    assert(!c.freeMovement);
    assert(c.Sample(2080, false, 0, false) == TargetAction::None);
    c.Sample(2200, true, 0, false);
    assert(c.Sample(2600, false, 0, false) == TargetAction::Release);
    assert(c.freeMovement); // A low-rate sampler still recognizes a hold.
    std::puts("PASS depth tap/hold gestures and RB target/free transitions");
}
