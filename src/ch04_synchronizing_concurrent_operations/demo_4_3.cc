#include <chrono>
#include <iostream>

int main() {
    std::chrono::minutes d1(60);
    std::chrono::seconds d2;
    std::chrono::hours d3;
    d2 = d1; // implicit conversion occurs

    // 从 minutes 到 hours 的转换需要截断，不能隐式转换
    // d3 = d1; // compile error

    std::cout << "d1: " << d1.count() << " minutes" << std::endl;
    std::cout << "d2: " << d2.count() << " seconds" << std::endl;
    // std::cout << "d3: " << d3.count() << " hours" << std::endl;
}