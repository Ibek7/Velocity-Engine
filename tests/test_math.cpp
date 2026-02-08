#include <cassert>
#include <cmath>
#include <iostream>

#include "../include/math/Vector2D.h"

// Simple test framework
#define ASSERT_TRUE(condition)                                                                   \
    if (!(condition)) {                                                                          \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ \
                  << std::endl;                                                                  \
        return 1;                                                                                \
    }

#define ASSERT_FLOAT_EQ(a, b)                                                                  \
    if (std::abs((a) - (b)) > 0.0001f) {                                                       \
        std::cerr << "Assertion failed: " << #a << " (" << (a) << ") != " << #b << " (" << (b) \
                  << ")" << " at " << __FILE__ << ":" << __LINE__ << std::endl;                \
        return 1;                                                                              \
    }

using namespace JJM::Math;

int main() {
    std::cout << "Running Vector2D tests..." << std::endl;

    // Test Constructor
    Vector2D v1(1.0f, 2.0f);
    ASSERT_FLOAT_EQ(v1.x, 1.0f);
    ASSERT_FLOAT_EQ(v1.y, 2.0f);

    // Test Addition
    Vector2D v2(3.0f, 4.0f);
    Vector2D v3 = v1 + v2;
    ASSERT_FLOAT_EQ(v3.x, 4.0f);
    ASSERT_FLOAT_EQ(v3.y, 6.0f);

    // Test Subtraction
    Vector2D v4 = v2 - v1;
    ASSERT_FLOAT_EQ(v4.x, 2.0f);
    ASSERT_FLOAT_EQ(v4.y, 2.0f);

    // Test Scala Multiplication
    Vector2D v5 = v1 * 2.0f;
    ASSERT_FLOAT_EQ(v5.x, 2.0f);
    ASSERT_FLOAT_EQ(v5.y, 4.0f);

    // Test Magnitude
    Vector2D v6(3.0f, 4.0f);
    ASSERT_FLOAT_EQ(v6.magnitude(), 5.0f);

    // Test Normalization
    v6.normalize();
    ASSERT_FLOAT_EQ(v6.magnitude(), 1.0f);
    ASSERT_FLOAT_EQ(v6.x, 0.6f);
    ASSERT_FLOAT_EQ(v6.y, 0.8f);

    std::cout << "All Vector2D tests passed!" << std::endl;
    return 0;
}
