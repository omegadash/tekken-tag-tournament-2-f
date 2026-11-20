#pragma once
#include <algorithm>
#include <array>
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
struct BodySphere
{
    FloorPoint offset{};
    float height = 0.0f;
    float radius = 0.0f;
};

// The native upper body and pelvis push spheres carry the pose's lean.
using BodyCore = std::array<BodySphere, 2>;

inline float HorizontalContactRadius(const BodySphere& a, const BodySphere& b)
{
    if (a.radius <= 0.0f || b.radius <= 0.0f) return 0.0f;
    const float reach = a.radius + b.radius;
    const float height = b.height - a.height;
    return std::sqrt(std::max(0.0f, reach * reach - height * height));
}

// Sweep one fighter's root against a stationary body. A fighter already
// touching or overlapping can move out, but cannot move deeper through it.
inline float BodyTravelFraction(FloorPoint start, FloorPoint end, FloorPoint body,
                                float radius = kBodyContactDistance)
{
    const double dx = double(end.x) - start.x, dz = double(end.z) - start.z;
    const double x = double(start.x) - body.x, z = double(start.z) - body.z;
    const double toward = x * dx + z * dz;
    if (toward >= 0.0) return 1.0f;
    const double clearance = x * x + z * z - double(radius) * radius;
    if (clearance <= 0.0) return 0.0f;
    const double lengthSquared = dx * dx + dz * dz;
    const double discriminant = toward * toward - lengthSquared * clearance;
    if (discriminant <= 0.0) return 1.0f;
    return float(std::min(1.0, clearance / (-toward + std::sqrt(discriminant))));
}

inline float BodyTravelFraction(FloorPoint start, FloorPoint end, const BodyCore& moving,
                                FloorPoint obstacle, const BodyCore& resting)
{
    float travel = BodyTravelFraction(start, end, obstacle);
    for (const auto& a : moving)
        for (const auto& b : resting)
        {
            const float radius = HorizontalContactRadius(a, b);
            if (radius <= 0.0f) continue;
            // Match the separate sphere centers without changing the root path.
            const FloorPoint center{obstacle.x + b.offset.x - a.offset.x,
                                    obstacle.z + b.offset.z - a.offset.z};
            travel = std::min(travel, BodyTravelFraction(start, end, center, radius));
        }
    return travel;
}

// Distance needed to separate existing contact along a chosen unit direction
// from the first fighter towards the second, including purely pose-driven overlap.
inline float BodySeparation(FloorPoint first, const BodyCore& a, FloorPoint second,
                            const BodyCore& b, FloorPoint direction)
{
    const auto separation = [direction](float x, float z, float radius) {
        const float overlap = radius * radius - x * x - z * z;
        if (overlap <= 0.0f) return 0.0f;
        const float along = x * direction.x + z * direction.z;
        return std::sqrt(along * along + overlap) - along;
    };
    const float dx = second.x - first.x, dz = second.z - first.z;
    float distance = separation(dx, dz, kBodyContactDistance);
    for (const auto& one : a)
        for (const auto& two : b)
            distance = std::max(distance,
                separation(dx + two.offset.x - one.offset.x, dz + two.offset.z - one.offset.z,
                           HorizontalContactRadius(one, two)));
    return distance;
}

inline void ClampArenaDepth(float& x,float& z,float centerX,float centerZ,float rightX,float rightZ)
{
    const float depth=-(x-centerX)*rightZ+(z-centerZ)*rightX;
    const float excess=depth-std::clamp(depth,-kArenaHalfDepth,kArenaHalfDepth);
    x+=rightZ*excess;
    z-=rightX*excess;
}
}
