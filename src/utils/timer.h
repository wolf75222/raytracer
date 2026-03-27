#ifndef RT_UTILS_TIMER_H
#define RT_UTILS_TIMER_H

#include <chrono>

class Timer {
public:
    Timer() : start_(std::chrono::high_resolution_clock::now()) {}

    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    // Returns elapsed time in seconds
    double elapsed() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - start_).count();
    }

    // Returns elapsed time in milliseconds
    double elapsed_ms() const {
        return elapsed() * 1000.0;
    }

private:
    std::chrono::high_resolution_clock::time_point start_;
};

#endif // RT_UTILS_TIMER_H
