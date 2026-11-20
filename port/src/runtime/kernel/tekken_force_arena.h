#pragma once
#include <algorithm>
#include <cmath>

namespace tekken_force {
// Stay on the central Tulip floor around the native starting positions. The
// side camera does not chase depth, so the fighting band must remain readable.
constexpr float kArenaHalfDepth = 900.0f;
constexpr float kCameraBaseDistance = 4800.0f;
constexpr float kCameraBaseRise = 1800.0f;
constexpr float kCameraAimHeight = 700.0f;
constexpr float kBodyContactDistance = 420.0f;

struct FloorPoint { float x, z; };

// Sweep one fighter's root against a stationary body. A fighter already
// touching or overlapping can move out, but cannot move deeper through it.
inline float BodyTravelFraction(FloorPoint start, FloorPoint end, FloorPoint body)
{
    const double dx = double(end.x) - start.x, dz = double(end.z) - start.z;
    const double x = double(start.x) - body.x, z = double(start.z) - body.z;
    const double toward = x * dx + z * dz;
    if (toward >= 0.0) return 1.0f;
    const double clearance = x * x + z * z - kBodyContactDistance * kBodyContactDistance;
    if (clearance <= 0.0) return 0.0f;
    const double lengthSquared = dx * dx + dz * dz;
    const double discriminant = toward * toward - lengthSquared * clearance;
    if (discriminant <= 0.0) return 1.0f;
    return float(std::min(1.0, clearance / (-toward + std::sqrt(discriminant))));
}

inline void ClampArenaDepth(float& x,float& z,float centerX,float centerZ,float rightX,float rightZ)
{
    const float depth=-(x-centerX)*rightZ+(z-centerZ)*rightX;
    const float excess=depth-std::clamp(depth,-kArenaHalfDepth,kArenaHalfDepth);
    x+=rightZ*excess;
    z-=rightX*excess;
}
}
