#include "math/Matrix3x3.h"
#include <stdexcept>
#include <cstring>

namespace JJM {
namespace Math {

// Constructors
Matrix3x3::Matrix3x3() {
    std::memset(m, 0, sizeof(m));
}

Matrix3x3::Matrix3x3(const float values[3][3]) {
    std::memcpy(m, values, sizeof(m));
}

Matrix3x3::Matrix3x3(const Matrix3x3& mat) {
    std::memcpy(m, mat.m, sizeof(m));
}

// Assignment operator
Matrix3x3& Matrix3x3::operator=(const Matrix3x3& mat) {
    if (this != &mat) {
        std::memcpy(m, mat.m, sizeof(m));
    }
    return *this;
}

// Matrix operations
Matrix3x3 Matrix3x3::operator+(const Matrix3x3& mat) const {
    Matrix3x3 result;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result.m[i][j] = m[i][j] + mat.m[i][j];
        }
    }
    return result;
}

Matrix3x3 Matrix3x3::operator-(const Matrix3x3& mat) const {
    Matrix3x3 result;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result.m[i][j] = m[i][j] - mat.m[i][j];
        }
    }
    return result;
}

Matrix3x3 Matrix3x3::operator*(const Matrix3x3& mat) const {
    Matrix3x3 result;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result.m[i][j] = 0;
            for (int k = 0; k < 3; ++k) {
                result.m[i][j] += m[i][k] * mat.m[k][j];
            }
        }
    }
    return result;
}

Matrix3x3 Matrix3x3::operator*(float scalar) const {
    Matrix3x3 result;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result.m[i][j] = m[i][j] * scalar;
        }
    }
    return result;
}

Matrix3x3& Matrix3x3::operator+=(const Matrix3x3& mat) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m[i][j] += mat.m[i][j];
        }
    }
    return *this;
}

Matrix3x3& Matrix3x3::operator-=(const Matrix3x3& mat) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m[i][j] -= mat.m[i][j];
        }
    }
    return *this;
}

Matrix3x3& Matrix3x3::operator*=(const Matrix3x3& mat) {
    *this = *this * mat;
    return *this;
}

Matrix3x3& Matrix3x3::operator*=(float scalar) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            m[i][j] *= scalar;
        }
    }
    return *this;
}

// Matrix-vector multiplication
Vector2D Matrix3x3::transform(const Vector2D& v) const {
    float x = m[0][0] * v.x + m[0][1] * v.y + m[0][2];
    float y = m[1][0] * v.x + m[1][1] * v.y + m[1][2];
    float w = m[2][0] * v.x + m[2][1] * v.y + m[2][2];
    
    if (std::abs(w) > 1e-6f) {
        return Vector2D(x / w, y / w);
    }
    return Vector2D(x, y);
}

Vector2D Matrix3x3::transformDirection(const Vector2D& v) const {
    float x = m[0][0] * v.x + m[0][1] * v.y;
    float y = m[1][0] * v.x + m[1][1] * v.y;
    return Vector2D(x, y);
}

// Matrix properties
float Matrix3x3::determinant() const {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
         - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
         + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

Matrix3x3 Matrix3x3::transpose() const {
    Matrix3x3 result;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result.m[i][j] = m[j][i];
        }
    }
    return result;
}

Matrix3x3 Matrix3x3::inverse() const {
    float det = determinant();
    if (std::abs(det) < 1e-6f) {
        throw std::runtime_error("Matrix is not invertible (determinant is zero)");
    }

    Matrix3x3 result;
    float invDet = 1.0f / det;

    result.m[0][0] = (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * invDet;
    result.m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invDet;
    result.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invDet;

    result.m[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invDet;
    result.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invDet;
    result.m[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * invDet;

    result.m[2][0] = (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * invDet;
    result.m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * invDet;
    result.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * invDet;

    return result;
}

// Element access
float& Matrix3x3::at(int row, int col) {
    if (row < 0 || row >= 3 || col < 0 || col >= 3) {
        throw std::out_of_range("Matrix index out of range");
    }
    return m[row][col];
}

const float& Matrix3x3::at(int row, int col) const {
    if (row < 0 || row >= 3 || col < 0 || col >= 3) {
        throw std::out_of_range("Matrix index out of range");
    }
    return m[row][col];
}

// Static factory methods
Matrix3x3 Matrix3x3::Identity() {
    Matrix3x3 result;
    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    return result;
}

Matrix3x3 Matrix3x3::Zero() {
    return Matrix3x3();
}

Matrix3x3 Matrix3x3::Translation(float x, float y) {
    Matrix3x3 result = Identity();
    result.m[0][2] = x;
    result.m[1][2] = y;
    return result;
}

Matrix3x3 Matrix3x3::Translation(const Vector2D& v) {
    return Translation(v.x, v.y);
}

Matrix3x3 Matrix3x3::Rotation(float angle) {
    Matrix3x3 result = Identity();
    float cos_a = std::cos(angle);
    float sin_a = std::sin(angle);
    result.m[0][0] = cos_a;
    result.m[0][1] = -sin_a;
    result.m[1][0] = sin_a;
    result.m[1][1] = cos_a;
    return result;
}

Matrix3x3 Matrix3x3::Scale(float sx, float sy) {
    Matrix3x3 result = Identity();
    result.m[0][0] = sx;
    result.m[1][1] = sy;
    return result;
}

Matrix3x3 Matrix3x3::Scale(float s) {
    return Scale(s, s);
}

Matrix3x3 Matrix3x3::Shear(float sx, float sy) {
    Matrix3x3 result = Identity();
    result.m[0][1] = sx;
    result.m[1][0] = sy;
    return result;
}

// Friend functions
Matrix3x3 operator*(float scalar, const Matrix3x3& mat) {
    return mat * scalar;
}

std::ostream& operator<<(std::ostream& os, const Matrix3x3& mat) {
    os << "[";
    for (int i = 0; i < 3; ++i) {
        os << "[";
        for (int j = 0; j < 3; ++j) {
            os << mat.m[i][j];
            if (j < 2) os << ", ";
        }
        os << "]";
        if (i < 2) os << ", ";
    }
    os << "]";
    return os;
}

} // namespace Math
} // namespace JJM
