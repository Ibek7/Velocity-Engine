/**
 * @file MathUtils.h
 * @brief Mathematical utility functions and curve interpolation
 * @version 1.0.0
 * @date 2026-01-16
 */

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <algorithm>
#include <cmath>
#include <vector>

#include "math/Vector2D.h"

namespace JJM {
namespace Math {

// =============================================================================
// Interpolation Functions
// =============================================================================

/**
 * @brief Linear interpolation
 * @param a Start value
 * @param b End value
 * @param t Interpolation factor (0-1)
 * @return Interpolated value
 */
template <typename T>
inline T lerp(T a, T b, float t) {
    return a + (b - a) * t;
}

/**
 * @brief Clamp value between min and max
 * @param value Input value
 * @param min Minimum value
 * @param max Maximum value
 * @return Clamped value
 */
template <typename T>
inline T clamp(T value, T min, T max) {
    return (value < min) ? min : (value > max) ? max : value;
}

/**
 * @brief Smooth interpolation (smoothstep)
 * @param a Start value
 * @param b End value
 * @param t Interpolation factor (0-1)
 * @return Smoothly interpolated value
 */
template <typename T>
inline T smoothstep(T a, T b, float t) {
    t = t * t * (3.0f - 2.0f * t);
    return lerp(a, b, t);
}

/**
 * @brief Smoother interpolation (smootherstep)
 * @param a Start value
 * @param b End value
 * @param t Interpolation factor (0-1)
 * @return Extra smooth interpolated value
 */
template <typename T>
inline T smootherstep(T a, T b, float t) {
    t = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    return lerp(a, b, t);
}

/**
 * @brief Inverse linear interpolation
 * @param a Start value
 * @param b End value
 * @param v Value to find t for
 * @return Interpolation factor
 */
inline float inverseLerp(float a, float b, float v) { return (v - a) / (b - a); }

/**
 * @brief Remap value from one range to another
 * @param value Input value
 * @param inMin Input range minimum
 * @param inMax Input range maximum
 * @param outMin Output range minimum
 * @param outMax Output range maximum
 * @return Remapped value
 */
inline float remap(float value, float inMin, float inMax, float outMin, float outMax) {
    float t = inverseLerp(inMin, inMax, value);
    return lerp(outMin, outMax, t);
}

// =============================================================================
// Easing Functions
// =============================================================================

namespace Easing {
// Quadratic
inline float easeInQuad(float t) { return t * t; }
inline float easeOutQuad(float t) { return t * (2.0f - t); }
inline float easeInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

// Cubic
inline float easeInCubic(float t) { return t * t * t; }
inline float easeOutCubic(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}
inline float easeInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

// Quartic
inline float easeInQuart(float t) { return t * t * t * t; }
inline float easeOutQuart(float t) {
    float f = t - 1.0f;
    return 1.0f - f * f * f * f;
}
inline float easeInOutQuart(float t) {
    if (t < 0.5f) {
        return 8.0f * t * t * t * t;
    } else {
        float f = t - 1.0f;
        return 1.0f - 8.0f * f * f * f * f;
    }
}

// Quintic
inline float easeInQuint(float t) { return t * t * t * t * t; }
inline float easeOutQuint(float t) {
    float f = t - 1.0f;
    return f * f * f * f * f + 1.0f;
}
inline float easeInOutQuint(float t) {
    if (t < 0.5f) {
        return 16.0f * t * t * t * t * t;
    } else {
        float f = (2.0f * t) - 2.0f;
        return 0.5f * f * f * f * f * f + 1.0f;
    }
}

// Sine
inline float easeInSine(float t) { return 1.0f - std::cos(t * M_PI / 2.0f); }
inline float easeOutSine(float t) { return std::sin(t * M_PI / 2.0f); }
inline float easeInOutSine(float t) { return -(std::cos(M_PI * t) - 1.0f) / 2.0f; }

// Exponential
inline float easeInExpo(float t) { return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f)); }
inline float easeOutExpo(float t) { return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t); }
inline float easeInOutExpo(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    if (t < 0.5f) {
        return std::pow(2.0f, 20.0f * t - 10.0f) / 2.0f;
    } else {
        return (2.0f - std::pow(2.0f, -20.0f * t + 10.0f)) / 2.0f;
    }
}

// Circular
inline float easeInCirc(float t) { return 1.0f - std::sqrt(1.0f - t * t); }
inline float easeOutCirc(float t) { return std::sqrt(1.0f - (t - 1.0f) * (t - 1.0f)); }
inline float easeInOutCirc(float t) {
    if (t < 0.5f) {
        return (1.0f - std::sqrt(1.0f - 4.0f * t * t)) / 2.0f;
    } else {
        return (std::sqrt(1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f)) + 1.0f) / 2.0f;
    }
}

// Elastic
inline float easeInElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    float p = 0.3f;
    return -std::pow(2.0f, 10.0f * (t - 1.0f)) *
           std::sin((t - 1.0f - p / 4.0f) * (2.0f * M_PI) / p);
}
inline float easeOutElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    float p = 0.3f;
    return std::pow(2.0f, -10.0f * t) * std::sin((t - p / 4.0f) * (2.0f * M_PI) / p) + 1.0f;
}
inline float easeInOutElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    float p = 0.3f * 1.5f;
    if (t < 0.5f) {
        return -0.5f * std::pow(2.0f, 20.0f * t - 10.0f) *
               std::sin((2.0f * t - 1.0f - p / 4.0f) * (2.0f * M_PI) / p);
    } else {
        return 0.5f * std::pow(2.0f, -20.0f * t + 10.0f) *
                   std::sin((2.0f * t - 1.0f - p / 4.0f) * (2.0f * M_PI) / p) +
               1.0f;
    }
}

// Back
inline float easeInBack(float t) {
    float c1 = 1.70158f;
    return (c1 + 1.0f) * t * t * t - c1 * t * t;
}
inline float easeOutBack(float t) {
    float c1 = 1.70158f;
    return 1.0f + (c1 + 1.0f) * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}
inline float easeInOutBack(float t) {
    float c1 = 1.70158f;
    float c2 = c1 * 1.525f;
    return t < 0.5f
               ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
               : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) /
                     2.0f;
}

// Bounce
inline float easeOutBounce(float t) {
    if (t < 1.0f / 2.75f) {
        return 7.5625f * t * t;
    } else if (t < 2.0f / 2.75f) {
        t -= 1.5f / 2.75f;
        return 7.5625f * t * t + 0.75f;
    } else if (t < 2.5f / 2.75f) {
        t -= 2.25f / 2.75f;
        return 7.5625f * t * t + 0.9375f;
    } else {
        t -= 2.625f / 2.75f;
        return 7.5625f * t * t + 0.984375f;
    }
}
inline float easeInBounce(float t) { return 1.0f - easeOutBounce(1.0f - t); }
inline float easeInOutBounce(float t) {
    return t < 0.5f ? (1.0f - easeOutBounce(1.0f - 2.0f * t)) / 2.0f
                    : (1.0f + easeOutBounce(2.0f * t - 1.0f)) / 2.0f;
}
}  // namespace Easing

// =============================================================================
// Curve Utilities
// =============================================================================

/**
 * @brief Bezier curve interpolation
 */
class BezierCurve {
   public:
    /**
     * @brief Quadratic Bezier curve
     * @param p0 Start point
     * @param p1 Control point
     * @param p2 End point
     * @param t Parameter (0-1)
     * @return Point on curve
     */
    static Vector2D quadratic(const Vector2D& p0, const Vector2D& p1, const Vector2D& p2, float t) {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;

        Vector2D p = p0 * uu;
        p = p + p1 * (2.0f * u * t);
        p = p + p2 * tt;

        return p;
    }

    /**
     * @brief Cubic Bezier curve
     * @param p0 Start point
     * @param p1 First control point
     * @param p2 Second control point
     * @param p3 End point
     * @param t Parameter (0-1)
     * @return Point on curve
     */
    static Vector2D cubic(const Vector2D& p0, const Vector2D& p1, const Vector2D& p2,
                          const Vector2D& p3, float t) {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;

        Vector2D p = p0 * uuu;
        p = p + p1 * (3.0f * uu * t);
        p = p + p2 * (3.0f * u * tt);
        p = p + p3 * ttt;

        return p;
    }
};

/**
 * @brief Catmull-Rom spline interpolation
 */
class CatmullRomSpline {
   public:
    /**
     * @brief Interpolate between p1 and p2 using p0 and p3 for tangent calculation
     * @param p0 Point before p1
     * @param p1 Start point
     * @param p2 End point
     * @param p3 Point after p2
     * @param t Parameter (0-1)
     * @return Point on spline
     */
    static Vector2D interpolate(const Vector2D& p0, const Vector2D& p1, const Vector2D& p2,
                                const Vector2D& p3, float t) {
        float t2 = t * t;
        float t3 = t2 * t;

        Vector2D result;
        result.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t +
                           (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                           (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);

        result.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t +
                           (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                           (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);

        return result;
    }
};

/**
 * @brief Hermite spline interpolation
 */
class HermiteSpline {
   public:
    /**
     * @brief Interpolate with explicit tangents
     * @param p0 Start point
     * @param t0 Start tangent
     * @param p1 End point
     * @param t1 End tangent
     * @param t Parameter (0-1)
     * @return Point on spline
     */
    static Vector2D interpolate(const Vector2D& p0, const Vector2D& t0, const Vector2D& p1,
                                const Vector2D& t1, float t) {
        float t2 = t * t;
        float t3 = t2 * t;

        float h1 = 2.0f * t3 - 3.0f * t2 + 1.0f;
        float h2 = -2.0f * t3 + 3.0f * t2;
        float h3 = t3 - 2.0f * t2 + t;
        float h4 = t3 - t2;

        return p0 * h1 + p1 * h2 + t0 * h3 + t1 * h4;
    }
};

}  // namespace Math
}  // namespace JJM

#endif  // MATH_UTILS_H
