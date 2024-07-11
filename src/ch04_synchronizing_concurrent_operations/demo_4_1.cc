#include <future>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

std::string f() {
    std::ostringstream oss;
    oss << "thread " << std::this_thread::get_id();
    return oss.str();
}

int main() {
    std::cout << "main thread: " << std::this_thread::get_id() << std::endl;
    auto f1 = std::async(f);
    auto f2 = std::async(std::launch::async, f);
    auto f3 = std::async(std::launch::deferred, f);
    auto f4 = std::async(std::launch::async | std::launch::deferred, f);

    std::cout << "f1 runs in " << f1.get() << std::endl;
    std::cout << "f2 runs in " << f2.get() << std::endl;
    std::cout << "f3 runs in " << f3.get() << std::endl;
    std::cout << "f4 runs in " << f4.get() << std::endl;
}