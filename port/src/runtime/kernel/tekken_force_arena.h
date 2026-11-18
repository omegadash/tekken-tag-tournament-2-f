#pragma once
#include <algorithm>

namespace tekken_force {
// Stay on the central Tulip floor around the native starting positions. The
// side camera does not chase depth, so the fighting band must remain readable.
constexpr float kArenaHalfDepth = 900.0f;
constexpr float kCameraBaseDistance = 4800.0f;
constexpr float kCameraBaseRise = 1800.0f;
constexpr float kCameraAimHeight = 700.0f;

inline void ClampArenaDepth(float& x,float& z,float centerX,float centerZ,float rightX,float rightZ)
{
    const float depth=-(x-centerX)*rightZ+(z-centerZ)*rightX;
    const float excess=depth-std::clamp(depth,-kArenaHalfDepth,kArenaHalfDepth);
    x+=rightZ*excess;
    z-=rightX*excess;
}
}
