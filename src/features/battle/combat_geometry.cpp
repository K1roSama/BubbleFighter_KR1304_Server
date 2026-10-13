#include "features/battle/combat_geometry.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace bubble_fighter::server::features::battle {
namespace {
using Vector = std::array<double, 3>;
Vector Subtract(const CombatVector& a, const CombatVector& b) {
  return {double(a[0]) - b[0], double(a[1]) - b[1], double(a[2]) - b[2]};
}
double Dot(const Vector& a, const Vector& b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
Vector Cross(const Vector& a, const Vector& b) {
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
          a[0] * b[1] - a[1] * b[0]};
}
void Require(const CombatVector& point) {
  if (!CombatGeometry::IsFinite(point)) {
    throw std::invalid_argument("Non-finite combat position.");
  }
}
void RequireRadius(float radius) {
  if (!std::isfinite(radius) || radius < 0) {
    throw std::invalid_argument("Invalid combat collision radius.");
  }
}
void KeepFirst(std::optional<double>& first, std::optional<double> candidate) {
  if (candidate && (!first || *candidate < *first)) {
    first = candidate;
  }
}
std::optional<double> SphereTime(const Vector& displacement,
                                 const Vector& direction, double radius) {
  const double c = Dot(displacement, displacement) - radius * radius;
  if (c <= 0) {
    return 0;
  }
  const double a = Dot(direction, direction), b = Dot(displacement, direction);
  if (a == 0 || b >= 0) {
    return {};
  }
  const double determinant = b * b - a * c;
  if (determinant < 0) {
    return {};
  }
  // Stable quadratic root (avoids cancellation for distant/small spheres).
  const double t = c / (-b + std::sqrt(determinant));
  return t >= 0 && t <= 1 ? std::optional{t} : std::nullopt;
}
}  // namespace

bool CombatGeometry::IsFinite(const CombatVector& point) noexcept {
  return std::all_of(point.begin(), point.end(),
                     [](float x) { return std::isfinite(x); });
}

CombatVector CombatGeometry::DecodeBinary16(
    const std::array<std::uint16_t, 3>& bits) {
  CombatVector point{};
  for (std::size_t i = 0; i < bits.size(); ++i) {
    const std::uint32_t exponent = (bits[i] >> 10) & 31U;
    const std::uint32_t mantissa = bits[i] & 1023U;
    if (exponent == 31) {
      throw std::invalid_argument("Non-finite compressed combat coordinate.");
    }
    float value = exponent == 0
                      ? std::ldexp(static_cast<float>(mantissa), -24)
                      : std::ldexp(static_cast<float>(1024U + mantissa),
                                   static_cast<int>(exponent) - 25);
    if (bits[i] & 0x8000U) {
      value = -value;
    }
    point[i] = value;
  }
  return point;
}

double CombatGeometry::DistanceSquared(const CombatVector& a,
                                       const CombatVector& b) {
  Require(a);
  Require(b);
  const auto delta = Subtract(a, b);
  return Dot(delta, delta);
}

std::optional<double> CombatGeometry::IntersectSphere(
    const CombatVector& start, const CombatVector& end,
    const CombatSphere& sphere) {
  Require(start);
  Require(end);
  Require(sphere.center);
  RequireRadius(sphere.radius);
  return SphereTime(Subtract(start, sphere.center), Subtract(end, start),
                    sphere.radius);
}

std::optional<double> CombatGeometry::SweepSpheres(
    const CombatSphere& first, const CombatVector& first_end,
    const CombatSphere& second, const CombatVector& second_end) {
  Require(first.center);
  Require(first_end);
  Require(second.center);
  Require(second_end);
  RequireRadius(first.radius);
  RequireRadius(second.radius);
  auto direction = Subtract(first_end, first.center);
  const auto other_direction = Subtract(second_end, second.center);
  for (std::size_t i = 0; i < 3; ++i) {
    direction[i] -= other_direction[i];
  }
  return SphereTime(Subtract(first.center, second.center), direction,
                    double(first.radius) + second.radius);
}

std::optional<double> CombatGeometry::IntersectCapsule(
    const CombatVector& start, const CombatVector& end,
    const CombatCapsule& capsule) {
  Require(start);
  Require(end);
  Require(capsule.start);
  Require(capsule.end);
  RequireRadius(capsule.radius);
  const auto axis = Subtract(capsule.end, capsule.start),
             offset = Subtract(start, capsule.start);
  const auto direction = Subtract(end, start);
  const double length = Dot(axis, axis),
               radius2 = double(capsule.radius) * capsule.radius;
  if (length == 0) {
    return IntersectSphere(start, end, {capsule.start, capsule.radius});
  }
  const double oa = Dot(offset, axis), da = Dot(direction, axis);
  const double projected = std::clamp(oa / length, 0.0, 1.0);
  Vector closest{};
  for (std::size_t i = 0; i < 3; ++i) {
    closest[i] = offset[i] - projected * axis[i];
  }
  if (Dot(closest, closest) <= radius2) {
    return 0;
  }
  std::optional<double> result =
      IntersectSphere(start, end, {capsule.start, capsule.radius});
  KeepFirst(result, IntersectSphere(start, end, {capsule.end, capsule.radius}));
  // Remove the axis component before solving the finite cylinder.
  Vector radial_direction{}, radial_offset{};
  for (std::size_t i = 0; i < 3; ++i) {
    radial_direction[i] = direction[i] - axis[i] * da / length;
    radial_offset[i] = offset[i] - axis[i] * oa / length;
  }
  const double a = Dot(radial_direction, radial_direction),
               b = Dot(radial_direction, radial_offset);
  const double c = Dot(radial_offset, radial_offset) - radius2;
  const double determinant = b * b - a * c;
  if (a > 0 && determinant >= 0) {
    const double root = std::sqrt(determinant);
    for (double t : {(-b - root) / a, (-b + root) / a}) {
      const double along = oa + t * da;
      if (t >= 0 && t <= 1 && along >= 0 && along <= length) {
        KeepFirst(result, t);
      }
    }
  }
  return result;
}

std::optional<double> CombatGeometry::IntersectTriangle(
    const CombatVector& start, const CombatVector& end,
    const CombatTriangle& triangle) {
  Require(start);
  Require(end);
  Require(triangle.a);
  Require(triangle.b);
  Require(triangle.c);
  const auto direction = Subtract(end, start),
             ab = Subtract(triangle.b, triangle.a);
  const auto ac = Subtract(triangle.c, triangle.a),
             from_a = Subtract(start, triangle.a);
  const auto normal = Cross(ab, ac);
  const double normal2 = Dot(normal, normal);
  if (normal2 == 0) {
    throw std::invalid_argument("Degenerate combat world triangle.");
  }
  const double denominator = Dot(normal, direction);
  const double numerator = -Dot(normal, from_a);
  if (denominator == 0) {
    if (numerator != 0) {
      return {};
    }
    // In the plane, barycentric coordinates vary linearly along the
    // segment. Clip the interval against the three triangle edges.
    const double u0 = Dot(Cross(from_a, ac), normal) / normal2;
    const double v0 = Dot(Cross(ab, from_a), normal) / normal2;
    const double du = Dot(Cross(direction, ac), normal) / normal2;
    const double dv = Dot(Cross(ab, direction), normal) / normal2;
    double low = 0, high = 1;
    const auto clip = [&](double value, double change) {
      if (change == 0) {
        return value >= 0;
      }
      const double crossing = -value / change;
      if (change > 0) {
        low = std::max(low, crossing);
      } else {
        high = std::min(high, crossing);
      }
      return low <= high;
    };
    return clip(u0, du) && clip(v0, dv) && clip(1 - u0 - v0, -du - dv)
               ? std::optional{low}
               : std::nullopt;
  }
  const double t = numerator / denominator;
  if (t < 0 || t > 1) {
    return {};
  }
  Vector point{};
  for (std::size_t i = 0; i < 3; ++i) {
    point[i] = from_a[i] + t * direction[i];
  }
  const double u = Dot(Cross(point, ac), normal) / normal2;
  const double v = Dot(Cross(ab, point), normal) / normal2;
  constexpr double edge_tolerance = 1e-10;
  return u >= -edge_tolerance && v >= -edge_tolerance &&
                 u + v <= 1 + edge_tolerance
             ? std::optional{t}
             : std::nullopt;
}

std::optional<double> CombatGeometry::FirstObstruction(
    const CombatVector& start, const CombatVector& end,
    std::span<const CombatTriangle> triangles) {
  if (triangles.empty()) {
    throw std::invalid_argument("Combat map collision data is unavailable.");
  }
  std::optional<double> result;
  for (const auto& triangle : triangles) {
    KeepFirst(result, IntersectTriangle(start, end, triangle));
  }
  return result;
}
}  // namespace bubble_fighter::server::features::battle
