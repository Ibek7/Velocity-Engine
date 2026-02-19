#include <cassert>
#include <cmath>
#include <iostream>

#include "math/MathUtils.h"

int main() {
    std::cout << "Running MathUtils tests..." << std::endl;

    // Test clamp
    assert(JJM::Math::MathUtils::clamp(10, 0, 5) == 5);
    assert(JJM::Math::MathUtils::clamp(-10, 0, 5) == 0);
    assert(JJM::Math::MathUtils::clamp(3, 0, 5) == 3);
    assert(JJM::Math::MathUtils::clamp(0, 0, 5) == 0);
    assert(JJM::Math::MathUtils::clamp(5, 0, 5) == 5);

    // Test clamp with float
    assert(std::abs(JJM::Math::MathUtils::clamp(10.0f, 0.0f, 5.0f) - 5.0f) < 0.0001f);
    assert(std::abs(JJM::Math::MathUtils::clamp(-10.0f, 0.0f, 5.0f) - 0.0f) < 0.0001f);
    assert(std::abs(JJM::Math::MathUtils::clamp(3.5f, 0.0f, 5.0f) - 3.5f) < 0.0001f);

    std::cout << "MathUtils clamp tests passed!" << std::endl;

    // Test lerp
    assert(std::abs(JJM::Math::MathUtils::lerp(0.0f, 10.0f, 0.0f) - 0.0f) < 0.0001f);
    assert(std::abs(JJM::Math::MathUtils::lerp(0.0f, 10.0f, 1.0f) - 10.0f) < 0.0001f);
    assert(std::abs(JJM::Math::MathUtils::lerp(0.0f, 10.0f, 0.5f) - 5.0f) < 0.0001f);
    assert(std::abs(JJM::Math::MathUtils::lerp(0.0f, 10.0f, 0.25f) - 2.5f) < 0.0001f);

    std::cout << "MathUtils lerp tests passed!" << std::endl;

    std::cout << "All MathUtils tests passed!" << std::endl;
    return 0;
}
