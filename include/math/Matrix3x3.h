#ifndef MATRIX3X3_H
#define MATRIX3X3_H

#include "math/Vector2D.h"
#include <iostream>

namespace JJM {
namespace Math {

class Matrix3x3 {
public:
    float m[3][3];

    // Constructors
    Matrix3x3();
    Matrix3x3(const float values[3][3]);
    Matrix3x3(const Matrix3x3& mat);

    // Assignment operator
    Matrix3x3& operator=(const Matrix3x3& mat);

    // Matrix operations
    Matrix3x3 operator+(const Matrix3x3& mat) const;
    Matrix3x3 operator-(const Matrix3x3& mat) const;
    Matrix3x3 operator*(const Matrix3x3& mat) const;
    Matrix3x3 operator*(float scalar) const;
    
    Matrix3x3& operator+=(const Matrix3x3& mat);
    Matrix3x3& operator-=(const Matrix3x3& mat);
    Matrix3x3& operator*=(const Matrix3x3& mat);
    Matrix3x3& operator*=(float scalar);

    // Matrix-vector multiplication
    Vector2D transform(const Vector2D& v) const;
    Vector2D transformDirection(const Vector2D& v) const;

    // Matrix properties
    float determinant() const;
    Matrix3x3 transpose() const;
    Matrix3x3 inverse() const;

    // Element access
    float& at(int row, int col);
    const float& at(int row, int col) const;

    // Static factory methods
    static Matrix3x3 Identity();
    static Matrix3x3 Zero();
    static Matrix3x3 Translation(float x, float y);
    static Matrix3x3 Translation(const Vector2D& v);
    static Matrix3x3 Rotation(float angle);
    static Matrix3x3 Scale(float sx, float sy);
    static Matrix3x3 Scale(float s);
    static Matrix3x3 Shear(float sx, float sy);

    // Friend functions
    friend Matrix3x3 operator*(float scalar, const Matrix3x3& mat);
    friend std::ostream& operator<<(std::ostream& os, const Matrix3x3& mat);
};

} // namespace Math
} // namespace JJM

#endif // MATRIX3X3_H
