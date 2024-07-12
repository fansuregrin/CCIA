#include <exception>
#include <future>
#include <iostream>

std::future<int> foo() {
    std::promise<int> p;
    return p.get_future();
}

std::future<double> bar() {
    std::packaged_task<double()> t([]{return 1.2;});
    return t.get_future();
}

int main() {
    auto f1 = foo();
    auto f2 = bar();
    
    try {
        auto y1 = f1.get();
        std::cout << y1 << std::endl;
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    try {
        auto y2 = f2.get();
        std::cout << y2 << std::endl;
    } catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }
}