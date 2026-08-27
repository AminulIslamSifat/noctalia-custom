#include "render/animation/animation.h"

#include <algorithm>
#include <cmath>

namespace {

/// Solve cubic bezier B(t) for a given x using Newton-Raphson with bisection fallback.
/// Returns the parameter u such that Bx(u) ≈ x, then returns By(u).
float solveCubicBezier(float x1, float y1, float x2, float y2, float x) {
  if (x <= 0.0F) return 0.0F;
  if (x >= 1.0F) return 1.0F;

  // Newton-Raphson to find parameter u where Bx(u) = x
  float u = x; // Initial guess
  for (int i = 0; i < 8; ++i) {
    // Bx(u) = 3*(1-u)^2*u*x1 + 3*(1-u)*u^2*x2 + u^3
    const float oneMinusU = 1.0F - u;
    const float bx = 3.0F * oneMinusU * oneMinusU * u * x1
                   + 3.0F * oneMinusU * u * u * x2
                   + u * u * u;
    const float dbx = 3.0F * oneMinusU * oneMinusU * x1
                    + 6.0F * oneMinusU * u * (x2 - x1)
                    + 3.0F * u * u * (1.0F - x2);

    const float diff = bx - x;
    if (std::abs(diff) < 1e-6F) break;
    if (std::abs(dbx) < 1e-8F) break;
    u -= diff / dbx;
    u = std::clamp(u, 0.0F, 1.0F);
  }

  // Bisection fallback if Newton diverged
  {
    float lo = 0.0F, hi = 1.0F;
    for (int i = 0; i < 16 && (hi - lo) > 1e-6F; ++i) {
      const float mid = (lo + hi) * 0.5F;
      const float oneMinusMid = 1.0F - mid;
      const float bx = 3.0F * oneMinusMid * oneMinusMid * mid * x1
                     + 3.0F * oneMinusMid * mid * mid * x2
                     + mid * mid * mid;
      if (bx < x) lo = mid;
      else hi = mid;
    }
    // Use bisection result only if Newton's result is worse
    const float oneMinusU = 1.0F - u;
    const float bxNewton = 3.0F * oneMinusU * oneMinusU * u * x1
                         + 3.0F * oneMinusU * u * u * x2
                         + u * u * u;
    if (std::abs(bxNewton - x) > std::abs(((lo + hi) * 0.5F) - x)) {
      u = (lo + hi) * 0.5F;
    }
  }

  // Evaluate By(u)
  const float oneMinusU = 1.0F - u;
  return 3.0F * oneMinusU * oneMinusU * u * y1
       + 3.0F * oneMinusU * u * u * y2
       + u * u * u;
}

} // namespace

BezierCurve getBezierCurve(Easing easing) {
  switch (easing) {
  case Easing::ExpressiveFastSpatial:    return {0.42F, 1.67F, 0.21F, 0.9F};
  case Easing::ExpressiveDefaultSpatial: return {0.38F, 1.21F, 0.22F, 1.0F};
  case Easing::ExpressiveSlowSpatial:    return {0.39F, 1.29F, 0.35F, 0.98F};
  case Easing::ExpressiveFastEffects:    return {0.31F, 0.94F, 0.34F, 1.0F};
  case Easing::ExpressiveDefaultEffects: return {0.34F, 0.8F,  0.34F, 1.0F};
  case Easing::ExpressiveSlowEffects:    return {0.34F, 0.88F, 0.34F, 1.0F};
  default:                               return {0.0F,  0.0F,  1.0F,  1.0F};
  }
}

float applyEasing(Easing easing, float t) {
  t = std::clamp(t, 0.0F, 1.0F);

  switch (easing) {
  case Easing::Linear:
    return t;

  case Easing::EaseInQuad:
    return t * t;

  case Easing::EaseOutQuad:
    return t * (2.0F - t);

  case Easing::EaseInOutQuad:
    if (t < 0.5F) {
      return 2.0F * t * t;
    }
    return -1.0F + (4.0F - 2.0F * t) * t;

  case Easing::EaseOutCubic: {
    const float f = t - 1.0F;
    return f * f * f + 1.0F;
  }

  case Easing::EaseInOutCubic:
    if (t < 0.5F) {
      return 4.0F * t * t * t;
    } else {
      const float f = 2.0F * t - 2.0F;
      return 0.5F * f * f * f + 1.0F;
    }

  case Easing::EaseOutBack: {
    constexpr float c1 = 1.70158F;
    constexpr float c3 = c1 + 1.0F;
    const float f = t - 1.0F;
    return 1.0F + c3 * f * f * f + c1 * f * f;
  }

  // Cubic bezier expressive curves
  case Easing::ExpressiveFastSpatial:
  case Easing::ExpressiveDefaultSpatial:
  case Easing::ExpressiveSlowSpatial:
  case Easing::ExpressiveFastEffects:
  case Easing::ExpressiveDefaultEffects:
  case Easing::ExpressiveSlowEffects: {
    const auto [x1, y1, x2, y2] = getBezierCurve(easing);
    return solveCubicBezier(x1, y1, x2, y2, t);
  }
  }

  return t;
}
