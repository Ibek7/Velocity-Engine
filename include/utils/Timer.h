#ifndef TIMER_H
#define TIMER_H

#include <chrono>

namespace JJM {
namespace Utils {

class Timer {
   private:
    std::chrono::high_resolution_clock::time_point startTime;

   public:
    Timer() { reset(); }

    void reset() { startTime = std::chrono::high_resolution_clock::now(); }

    float getElapsedSeconds() const {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = currentTime - startTime;
        return duration.count();
    }

    float getElapsedMilliseconds() const { return getElapsedSeconds() * 1000.0f; }
};

}  // namespace Utils
}  // namespace JJM

#endif  // TIMER_H
