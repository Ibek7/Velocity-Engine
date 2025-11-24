#include "math/Vector2D.h"
#include <stdexcept>

namespace JJM {
namespace Math {

// Constructors
Vector2D::Vector2D() : x(0.0f), y(0.0f) {}

Vector2D::Vector2D(float x, float y) : x(x), y(y) {}

Vector2D::Vector2D(const Vector2D& v) : x(v.x), y(v.y) {}

// Assignment operator
Vector2D& Vector2D::operator=(const Vector2D& v) {
    if (this != &v) {
        x = v.x;
        y = v.y;
    }
    return *this;
}

// Arithmetic operators
Vector2D Vector2D::operator+(const Vector2D& v) const {
    return Vector2D(x + v.x, y + v.y);
}

Vector2D Vector2D::operator-(const Vector2D& v) const {
    return Vector2D(x - v.x, y - v.y);
}

Vector2D Vector2D::operator*(float scalar) const {
    return Vector2D(x * scalar, y * scalar);
}

Vector2D Vector2D::operator/(float scalar) const {
    if (std::abs(scalar) < 1e-6f) {
        throw std::runtime_error("Division by zero in Vector2D");
    }
    return Vector2D(x / scalar, y / scalar);
}

// Compound assignment operators
Vector2D& Vector2D::operator+=(const Vector2D& v) {
    x += v.x;
    y += v.y;
    return *this;
}

Vector2D& Vector2D::operator-=(const Vector2D& v) {
    x -= v.x;
    y -= v.y;
    return *this;
}

Vector2D& Vector2D::operator*=(float scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
}

Vector2D& Vector2D::operator/=(float scalar) {
    if (std::abs(scalar) < 1e-6f) {
        throw std::runtime_error("Division by zero in Vector2D");
    }
    x /= scalar;
    y /= scalar;
    return *this;
}

// Comparison operators
bool Vector2D::operator==(const Vector2D& v) const {
    return std::abs(x - v.x) < 1e-6f && std::abs(y - v.y) < 1e-6f;
}

bool Vector2D::operator!=(const Vector2D& v) const {
    return !(*this == v);
}

// Unary operators
Vector2D Vector2D::operator-() const {
    return Vector2D(-x, -y);
}

// Vector operations
float Vector2D::magnitude() const {
    return std::sqrt(x * x + y * y);
}

float Vector2D::magnitudeSquared() const {
    return x * x + y * y;
}

Vector2D Vector2D::normalized() const {
    float mag = magnitude();
    if (mag < 1e-6f) {
        return Vector2D(0, 0);
    }
    return *this / mag;
}

void Vector2D::normalize() {
    float mag = magnitude();
    if (mag > 1e-6f) {
        x /= mag;
        y /= mag;
    }
}

float Vector2D::dot(const Vector2D& v) const {
    return x * v.x + y * v.y;
}

float Vector2D::cross(const Vector2D& v) const {
    return x * v.y - y * v.x;
}

float Vector2D::distance(const Vector2D& v) const {
    return (*this - v).magnitude();
}

float Vector2D::distanceSquared(const Vector2D& v) const {
    return (*this - v).magnitudeSquared();
}

float Vector2D::angle() const {
    return std::atan2(y, x);
}

float Vector2D::angleTo(const Vector2D& v) const {
    float dot = this->dot(v);
    float mag = magnitude() * v.magnitude();
    if (mag < 1e-6f) return 0.0f;
    return std::acos(std::max(-1.0f, std::min(1.0f, dot / mag)));
}

Vector2D Vector2D::project(const Vector2D& v) const {
    float dotProduct = dot(v);
    float vMagSquared = v.magnitudeSquared();
    if (vMagSquared < 1e-6f) return Vector2D(0, 0);
    return v * (dotProduct / vMagSquared);
}

Vector2D Vector2D::reflect(const Vector2D& normal) const {
    return *this - normal * (2.0f * dot(normal));
}

Vector2D Vector2D::lerp(const Vector2D& v, float t) const {
    return *this + (v - *this) * t;
}

Vector2D Vector2D::rotate(float angle) const {
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);
    return Vector2D(x * cos_a - y * sin_a, x * sin_a + y * cos_a);
}

void Vector2D::rotateInPlace(float angle) {
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);
    float newX = x * cos_a - y * sin_a;
    float newY = x * sin_a + y * cos_a;
    x = newX;
    y = newY;
}

Vector2D Vector2D::perpendicular() const {
    return Vector2D(-y, x);
}

// Static utility functions
float Vector2D::Dot(const Vector2D& a, const Vector2D& b) {
    return a.dot(b);
}

float Vector2D::Cross(const Vector2D& a, const Vector2D& b) {
    return a.cross(b);
}

float Vector2D::Distance(const Vector2D& a, const Vector2D& b) {
    return a.distance(b);
}

Vector2D Vector2D::Lerp(const Vector2D& a, const Vector2D& b, float t) {
    return a.lerp(b, t);
}

Vector2D Vector2D::Zero() {
    return Vector2D(0, 0);
}

Vector2D Vector2D::One() {
    return Vector2D(1, 1);
}

Vector2D Vector2D::Up() {
    return Vector2D(0, -1);
}

Vector2D Vector2D::Down() {
    return Vector2D(0, 1);
}

Vector2D Vector2D::Left() {
    return Vector2D(-1, 0);
}

Vector2D Vector2D::Right() {
    return Vector2D(1, 0);
}

// Friend functions
Vector2D operator*(float scalar, const Vector2D& v) {
    return v * scalar;
}

std::ostream& operator<<(std::ostream& os, const Vector2D& v) {
    os << "(" << v.x << ", " << v.y << ")";
    return os;
}

} // namespace Math
} // namespace JJM
