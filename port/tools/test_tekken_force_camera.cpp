#include "../src/runtime/kernel/tekken_force_camera_follow.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    using tekken_force::HorizontalCameraFollow;
    HorizontalCameraFollow camera;
    camera.Reset(0);
    camera.ObserveWalk(0,900);
    assert(camera.Update(1)==0 && camera.center==0);
    camera.ObserveWalk(900,1024);
    assert(std::abs(camera.Update(2)-4.8f)<.001f);
    assert(camera.Update(2)==0); // More than one publication cannot advance follow.
    camera.Reset(0);
    camera.ObserveWalk(9000,9024); // A settled walking position after native repositioning.
    assert(camera.Update(3)==24); // The reference can never cause an unbounded pan.
    for(uint64_t i=4;i<1000;++i) assert(std::abs(camera.Update(i))<=24);
    assert(std::abs(camera.center-8024)<.01f);
    const float stable=camera.center;
    camera.ObserveWalk(-5000,-5000); // Selection/pose changes without walking do not re-anchor.
    assert(std::abs(camera.Update(1001))<.001f && std::abs(camera.center-stable)<.001f);
    camera.Reset(0);
    camera.ObserveWalk(0,-1100);
    assert(camera.Update(1002)==-20);
    camera.Reset(42);
    assert(camera.Update(1003)==0 && camera.center==42);
    std::puts("PASS horizontal dead zone, bounded follow, idle reference and once-per-tick update");
}
