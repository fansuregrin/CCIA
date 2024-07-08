// Listing 3.1 Protecting a list with a mutex
#include <algorithm>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>
#include <chrono>

std::list<int> some_list;
std::mutex some_mtx;

void add_to_list(int new_value) {
    std::lock_guard<std::mutex> guard(some_mtx);
    some_list.push_back(new_value);
}

bool list_contains(int value_to_find) {
    std::lock_guard<std::mutex> guard(some_mtx);
    return std::find(some_list.begin(), some_list.end(), value_to_find) !=
           some_list.end();
}

int main() {
    const int N = 20;
    std::thread t1([] {
        for (int i = 0; i <= N; ++i) {
            add_to_list(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread t2([] {
        for (int i = N; i >= 0; --i) {
            std::cout << "list contains " << i << ": " << std::boolalpha
                      << list_contains(i) << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    t1.join();
    t2.join();
}