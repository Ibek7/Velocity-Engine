#ifndef VECTOR2D_H
#define VECTOR2D_H

#include <cmath>
#include <iostream>

namespace JJM {
namespace Math {

class Vector2D {
public:
    float x, y;

    // Constructors
    Vector2D();
    Vector2D(float x, float y);
    Vector2D(const Vector2D& v);

    // Assignment operator
    Vector2D& operator=(const Vector2D& v);

    // Arithmetic operators
    Vector2D operator+(const Vector2D& v) const;
    Vector2D operator-(const Vector2D& v) const;
    Vector2D operator*(float scalar) const;
    Vector2D operator/(float scalar) const;

    // Compound assignment operators
    Vector2D& operator+=(const Vector2D& v);
    Vector2D& operator-=(const Vector2D& v);
    Vector2D& operator*=(float scalar);
    Vector2D& operator/=(float scalar);

    // Comparison operators
    bool operator==(const Vector2D& v) const;
    bool operator!=(const Vector2D& v) const;

    // Unary operators
    Vector2D operator-() const;

    // Vector operations
    float magnitude() const;
    float magnitudeSquared() const;
    Vector2D normalized() const;
    void normalize();
    float dot(const Vector2D& v) const;
    float cross(const Vector2D& v) const;
    float distance(const Vector2D& v) const;
    float distanceSquared(const Vector2D& v) const;
    float angle() const;
    float angleTo(const Vector2D& v) const;
    
    // Projection and reflection
    Vector2D project(const Vector2D& v) const;
    Vector2D reflect(const Vector2D& normal) const;
    
    // Interpolation
    Vector2D lerp(const Vector2D& v, float t) const;
    
    // Rotation
    Vector2D rotate(float angle) const;
    void rotateInPlace(float angle);
    
    // Perpendicular vectors
    Vector2D perpendicular() const;
    
    // Static utility functions
    static float Dot(const Vector2D& a, const Vector2D& b);
    static float Cross(const Vector2D& a, const Vector2D& b);
    static float Distance(const Vector2D& a, const Vector2D& b);
    static Vector2D Lerp(const Vector2D& a, const Vector2D& b, float t);
    static Vector2D Zero();
    static Vector2D One();
    static Vector2D Up();
    static Vector2D Down();
    static Vector2D Left();
    static Vector2D Right();

    // Friend function for scalar multiplication
    friend Vector2D operator*(float scalar, const Vector2D& v);
    
    // Stream operators
    friend std::ostream& operator<<(std::ostream& os, const Vector2D& v);
};

} // namespace Math
} // namespace JJM

#endif // VECTOR2D_H
