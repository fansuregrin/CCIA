#include <chrono>
#include <iostream>
#include <locale>

template <typename Clock, typename Duration>
std::string print_timepoint(const std::chrono::time_point<Clock, Duration> &tp) {
    auto t = Clock::to_time_t(tp);
    char buf[1024] = {0};
    std::strftime(buf, sizeof(buf), "%c %Z", std::localtime(&t));
    return buf;
}

template <typename Rep, typename Period>
std::string print_duration(const std::chrono::duration<Rep, Period> &dur) {
    return std::to_string(dur.count());
}

template <typename Rep>
std::string print_duration(const std::chrono::duration<Rep, std::nano> &dur) {
    return std::to_string(dur.count()) + "ns";
}

template <typename Rep>
std::string print_duration(const std::chrono::duration<Rep, std::ratio<1>> &dur) {
    return std::to_string(dur.count()) + "s";
}

template <typename Rep>
std::string print_duration(const std::chrono::duration<Rep, std::ratio<60>> &dur) {
    return std::to_string(dur.count()) + "min";
}

int main() {
    auto tm1 = std::chrono::system_clock::now();
    auto tm2 = tm1 + std::chrono::seconds(10);
    auto tm3 = tm1 - std::chrono::minutes(2);
    auto d1 = std::chrono::duration_cast<std::chrono::minutes>(tm1 - tm3);
    auto d2 = std::chrono::duration_cast<std::chrono::seconds>(tm1 - tm2);
#if __cplusplus >= 202002L
    std::cout << "tm1 since epoch: " << tm1.time_since_epoch() << std::endl;
    std::cout << "tm1: " << tm1 << std::endl;
    std::cout << "tm2: " << tm2 << std::endl;
    std::cout << "tm3: " << tm3 << std::endl;
    std::cout << "d1: " << d1 << std::endl;
    std::cout << "d2: " << d2 << std::endl;
#else
    std::cout << "tm1 since epoch: " << print_duration(tm1.time_since_epoch()) << std::endl;
    std::cout << "tm1: " << print_timepoint(tm1) << std::endl;
    std::cout << "tm2: " << print_timepoint(tm2) << std::endl;
    std::cout << "tm3: " << print_timepoint(tm3) << std::endl;
    std::cout << "d1: " << print_duration(d1) << std::endl;
    std::cout << "d2: " << print_duration(d2) << std::endl;
#endif

};