#include "features/battle/combat_scene.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "features/battle/generated/combat_scene_data.h"

namespace bubble_fighter::server::features::battle {
CombatScene::CombatScene(std::span<const CombatSceneFace> faces) {
  if (faces.empty()) {
    throw std::invalid_argument("Empty combat scene.");
  }
  faces_.reserve(faces.size());
  for (const auto& source : faces) {
    const auto& t = source.triangle;
    if (!CombatGeometry::IsFinite(t.a) || !CombatGeometry::IsFinite(t.b) ||
        !CombatGeometry::IsFinite(t.c) || source.queries == 0 ||
        source.queries > 7) {
      throw std::invalid_argument("Invalid combat scene face.");
    }
    Face face{source, {}, {}};
    for (std::size_t i = 0; i < 3; ++i) {
      face.minimum[i] = std::min({t.a[i], t.b[i], t.c[i]});
      face.maximum[i] = std::max({t.a[i], t.b[i], t.c[i]});
    }
    faces_.push_back(face);
  }
}

std::optional<double> CombatScene::FirstObstruction(
    const CombatVector& start, const CombatVector& end,
    CombatSceneQuery query) const {
  if (!CombatGeometry::IsFinite(start) || !CombatGeometry::IsFinite(end)) {
    throw std::invalid_argument("Non-finite combat scene ray.");
  }
  if (query != CombatSceneQuery::kBullet &&
      query != CombatSceneQuery::kGrenade &&
      query != CombatSceneQuery::kActor) {
    throw std::invalid_argument("Unknown combat scene query.");
  }
  std::optional<double> nearest;
  for (const auto& face : faces_) {
    if ((face.source.queries & static_cast<std::uint8_t>(query)) == 0) {
      continue;
    }
    double first = 0, last = nearest.value_or(1.0);
    bool candidate = true;
    for (std::size_t i = 0; i < 3; ++i) {
      const double delta = double(end[i]) - start[i];
      if (delta == 0) {
        if (start[i] < face.minimum[i] || start[i] > face.maximum[i]) {
          candidate = false;
          break;
        }
        continue;
      }
      const double a = (double(face.minimum[i]) - start[i]) / delta;
      const double b = (double(face.maximum[i]) - start[i]) / delta;
      first = std::max(first, std::min(a, b));
      last = std::min(last, std::max(a, b));
      if (first > last) {
        candidate = false;
        break;
      }
    }
    if (!candidate) {
      continue;
    }
    // 10458D0 ignores parallel/coplanar rays; the generic geometry
    // primitive also supports coplanar clipping, which is not used here.
    const auto& t = face.source.triangle;
    std::array<double, 3> ab{}, ac{}, direction{};
    for (std::size_t i = 0; i < 3; ++i) {
      ab[i] = double(t.b[i]) - t.a[i];
      ac[i] = double(t.c[i]) - t.a[i];
      direction[i] = double(end[i]) - start[i];
    }
    const double denominator = (ab[1] * ac[2] - ab[2] * ac[1]) * direction[0] +
                               (ab[2] * ac[0] - ab[0] * ac[2]) * direction[1] +
                               (ab[0] * ac[1] - ab[1] * ac[0]) * direction[2];
    if (std::abs(denominator) < double(1.0e-9f)) {
      continue;
    }
    const auto hit =
        CombatGeometry::IntersectTriangle(start, end, face.source.triangle);
    if (hit && (!nearest || *hit < *nearest)) {
      nearest = hit;
    }
  }
  return nearest;
}

const CombatScene* CombatScene::Find(std::u16string_view map_key) {
  if (map_key != u"rel_Village01_Byway") {
    return nullptr;
  }
  static const CombatScene kByway(kBywayStaticFaces);
  return &kByway;
}
}  // namespace bubble_fighter::server::features::battle
