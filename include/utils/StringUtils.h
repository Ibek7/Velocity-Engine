#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace JJM {
namespace Utils {

class StringUtils {
   public:
    static std::string trim(const std::string& str) {
        auto start = str.begin();
        while (start != str.end() && std::isspace(*start)) {
            start++;
        }

        auto end = str.end();
        do {
            end--;
        } while (std::distance(start, end) > 0 && std::isspace(*end));

        return std::string(start, end + 1);
    }

    static std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);

        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }

        return tokens;
    }

    static std::string toUpper(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return result;
    }

    static std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }
};

}  // namespace Utils
}  // namespace JJM

#endif  // STRING_UTILS_H
