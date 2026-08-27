#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>

enum class Easing : std::uint8_t {
  Linear,
  EaseInQuad,
  EaseOutQuad,
  EaseInOutQuad,
  EaseOutCubic,
  EaseInOutCubic,
  EaseOutBack,
  // Material Design 3 Expressive curves (cubic bezier)
  ExpressiveFastSpatial,    // 0.42, 1.67, 0.21, 0.9  — snappy overshoot
  ExpressiveDefaultSpatial, // 0.38, 1.21, 0.22, 1    — balanced overshoot (launcher default)
  ExpressiveSlowSpatial,    // 0.39, 1.29, 0.35, 0.98 — gentle overshoot
  ExpressiveFastEffects,    // 0.31, 0.94, 0.34, 1    — quick smooth decel
  ExpressiveDefaultEffects, // 0.34, 0.8, 0.34, 1     — standard smooth decel
  ExpressiveSlowEffects,    // 0.34, 0.88, 0.34, 1    — relaxed smooth decel
};

/// Cubic bezier control points for an easing curve.
struct BezierCurve {
  float x1, y1, x2, y2;
};

/// Returns the bezier control points for expressive easing types.
/// Returns {0,0,1,1} (linear) for non-bezier types.
BezierCurve getBezierCurve(Easing easing);

float applyEasing(Easing easing, float t);

struct Animation {
  float startValue = 0.0F;
  float endValue = 0.0F;
  float durationMs = 0.0F;
  float elapsedMs = 0.0F;
  std::chrono::steady_clock::time_point startedAt;
  Easing easing = Easing::EaseOutQuad;
  std::function<void(float)> setter;
  std::function<void()> onComplete;
  bool finished = false;
};
