// Listing 2.5 Returning a std::thread from a function
#include <thread>

void some_function() {}
void some_other_function(int x) {}

std::thread f() {
    void some_function();
    return std::thread(some_function);
}

std::thread g() {
    void some_other_function(int);
    std::thread t(some_other_function, 42);
    return t;
}

int main() {
    std::thread t1 = f();
    std::thread t2 = g();
    t1.join();
    t2.join();
}