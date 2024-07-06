#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

struct A {
    A(const char *p) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "conversion happens in thread: " << std::this_thread::get_id() << std::endl;
        m_str = p;
        std::cout << "conversion finished" << std::endl;
    }

    std::string m_str;
};

void f(unsigned i, A a) {
    for (unsigned j = 0; j < i; ++j) {
        std::cout << a.m_str << std::endl;
    }
}

void oops(int some_param) {
    std::cout << "main thread: " << std::this_thread::get_id() << std::endl;
    char buffer[1024];
    sprintf(buffer, "%d", some_param);
#ifdef EXCONV
    std::thread t(f, 3, A(buffer));
#else    
    std::thread t(f, 3, buffer);
#endif
    t.detach();
    sprintf(buffer, "abcdefghijk");
    std::cout << "opps exited" << std::endl;
}

int main() {
    oops(12345);
    std::this_thread::sleep_for(std::chrono::seconds(5));
}