#include <chrono>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <thread>

void f(int i) {
    std::ofstream ofs("output.txt", std::ios_base::out | std::ios_base::trunc);
    if (!ofs) {
        std::cerr << "Cannot open output.txt" << std::endl;
        return;
    }
    for (int j = 0; j < i; ++j) {
        ofs << "hello" << '\n';
    }
}

int main() {
    std::thread t(f, 3);
    t.detach();
    // std::this_thread::sleep_for(std::chrono::seconds(5));
}