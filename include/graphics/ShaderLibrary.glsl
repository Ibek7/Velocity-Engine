/**
 * @file ShaderLibrary.glsl
 * @brief Common shader functions and utilities
 * @version 1.0.0
 * @date 2026-01-12
 */

#ifndef SHADER_LIBRARY_GLSL
#define SHADER_LIBRARY_GLSL

// =============================================================================
// MATH UTILITIES
// =============================================================================

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float HALF_PI = 1.57079632679;
const float INV_PI = 0.31830988618;

float saturate(float x) {
    return clamp(x, 0.0, 1.0);
}

vec3 saturate(vec3 x) {
    return clamp(x, 0.0, 1.0);
}

float remap(float value, float inMin, float inMax, float outMin, float outMax) {
    return outMin + (outMax - outMin) * ((value - inMin) / (inMax - inMin));
}

// =============================================================================
// LIGHTING
// =============================================================================

struct Light {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    float spotAngle;
};

vec3 calculateDiffuse(vec3 normal, vec3 lightDir, vec3 lightColor) {
    float NdotL = max(dot(normal, lightDir), 0.0);
    return lightColor * NdotL;
}

vec3 calculateSpecular(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 lightColor, float shininess) {
    vec3 halfDir = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfDir), 0.0);
    float spec = pow(NdotH, shininess);
    return lightColor * spec;
}

// =============================================================================
// TONE MAPPING
// =============================================================================

vec3 tonemap_Reinhard(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 tonemap_ACES(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// =============================================================================
// COLOR SPACE
// =============================================================================

vec3 sRGBToLinear(vec3 srgb) {
    return pow(srgb, vec3(2.2));
}

vec3 linearToSRGB(vec3 linear) {
    return pow(linear, vec3(1.0 / 2.2));
}

// =============================================================================
// NOISE
// =============================================================================

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i + vec2(0.0, 0.0));
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

#endif // SHADER_LIBRARY_GLSL
