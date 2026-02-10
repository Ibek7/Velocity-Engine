#include <iostream>
#include <string>
#include <vector>

#include "../include/utils/StringUtils.h"

// Simple test framework
#define ASSERT_EQ(a, b)                                                                        \
    if ((a) != (b)) {                                                                          \
        std::cerr << "Assertion failed: " << #a << " (" << (a) << ") != " << #b << " (" << (b) \
                  << ")" << " at " << __FILE__ << ":" << __LINE__ << std::endl;                \
        return 1;                                                                              \
    }

#define ASSERT_TRUE(condition)                                                                   \
    if (!(condition)) {                                                                          \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ \
                  << std::endl;                                                                  \
        return 1;                                                                                \
    }

using namespace JJM::Utils;

int main() {
    std::cout << "Running StringUtils tests..." << std::endl;

    // Test Trim
    std::string s1 = "  hello  ";
    ASSERT_EQ(StringUtils::trim(s1), "hello");

    std::string s2 = "hello";
    ASSERT_EQ(StringUtils::trim(s2), "hello");

    std::string s3 = "   ";
    ASSERT_EQ(StringUtils::trim(s3), "");

    // Test Split
    std::string s4 = "a,b,c";
    std::vector<std::string> parts = StringUtils::split(s4, ',');
    ASSERT_EQ(parts.size(), 3);
    ASSERT_EQ(parts[0], "a");
    ASSERT_EQ(parts[1], "b");
    ASSERT_EQ(parts[2], "c");

    // Test ToUpper
    std::string s5 = "hello";
    ASSERT_EQ(StringUtils::toUpper(s5), "HELLO");

    // Test ToLower
    std::string s6 = "HELLO";
    ASSERT_EQ(StringUtils::toLower(s6), "hello");

    std::cout << "All StringUtils tests passed!" << std::endl;
    return 0;
}
