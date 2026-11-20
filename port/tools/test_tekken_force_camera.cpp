#include "../src/runtime/kernel/tekken_force_camera_follow.h"
#include "../src/runtime/kernel/tekken_force_arena.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main()
{
    using tekken_force::HorizontalCameraFollow;
    HorizontalCameraFollow camera;
    camera.Reset(0);
    camera.ObserveWalk(0,600);
    assert(camera.Update(1)==0 && camera.center==0);
    camera.ObserveWalk(600,724);
    assert(std::abs(camera.Update(2)-1.5f)<.001f);
    assert(camera.Update(2)==0); // More than one publication cannot advance follow.
    camera.Reset(0);
    camera.ObserveWalk(9000,9024); // A settled walking position after native repositioning.
    assert(camera.Update(3)==1.5f); // A reference jump cannot instantly accelerate the pan.
    float previous=1.5f;
    for(uint64_t i=4;i<1000;++i)
    {
        const float step=camera.Update(i);
        if(std::abs(step)>HorizontalCameraFollow::kMaxStep || std::abs(step-previous)>1.501f)
            std::fprintf(stderr,"tick %llu step %.6f previous %.6f center %.6f reference %.6f\n",
                static_cast<unsigned long long>(i),step,previous,camera.center,camera.reference);
        assert(std::abs(step)<=HorizontalCameraFollow::kMaxStep && std::abs(step-previous)<=1.501f);
        previous=step;
    }
    assert(std::abs(camera.center-8324)<.01f);
    const float stable=camera.center;
    camera.ObserveWalk(-5000,-5000); // Selection/pose changes without walking do not re-anchor.
    assert(std::abs(camera.Update(1001))<.001f && std::abs(camera.center-stable)<.001f);
    camera.Reset(0);
    camera.ObserveWalk(0,-1100);
    assert(camera.Update(1002)==-1.5f);
    camera.Reset(42);
    assert(camera.Update(1003)==0 && camera.center==42);
    previous=0;
    for(uint64_t i=1004;i<1120;++i)
    {
        camera.ObserveWalk(camera.reference,(i<1040?5000.0f:-5000.0f));
        const float step=camera.Update(i);
        assert(std::abs(step)<=HorizontalCameraFollow::kMaxStep && std::abs(step-previous)<=1.501f);
        previous=step;
    }
    for(float angle : {0.0f,0.2f,-0.7f})
    {
        const float rx=std::cos(angle),rz=std::sin(angle);
        float x=rx*1234-rz*3000,z=rz*1234+rx*3000;
        tekken_force::ClampArenaDepth(x,z,0,0,rx,rz);
        assert(std::abs(x*rx+z*rz-1234)<.01f);
        assert(std::abs(-x*rz+z*rx-tekken_force::kArenaHalfDepth)<.01f);
    }
    camera.Reset(0);
    camera.ObserveCombat(2000,1); // A one-frame root spike is not tracking intent.
    camera.ObserveCombat(2000,1); // Repeated callbacks are still one sample.
    assert(camera.reference==0);
    camera.ObserveCombat(0,2);
    camera.ObserveCombat(0,3);
    assert(camera.reference==0);
    camera.ObserveCombat(2000,4);
    camera.ObserveCombat(2000,5); // Persistent committed displacement must stay visible.
    assert(camera.reference==2000);
    // An endpoint beyond the other fighter must not tunnel through its body.
    using tekken_force::BodyTravelFraction;
    assert(std::abs(BodyTravelFraction({-900, 0}, {900, 0}, {0, 0}) - 480.0f / 1800.0f) < .0001f);
    assert(BodyTravelFraction({-420, 0}, {-390, 0}, {0, 0}) == 0);
    assert(BodyTravelFraction({-419, 0}, {-450, 0}, {0, 0}) == 1);
    assert(BodyTravelFraction({-420, 0}, {-420, 24}, {0, 0}) == 1);
    assert(BodyTravelFraction({-900, 421}, {900, 421}, {0, 0}) == 1);
    std::puts("PASS dead zone, bounded camera acceleration, idle reference, once-per-tick follow and floor band");
}
