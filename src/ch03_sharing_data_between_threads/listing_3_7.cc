#include "listing_3_8.h"
#include <iostream>
#include <thread>

hierarchical_mutex high_level_mutex(10000);
hierarchical_mutex low_level_mutex(5000);
hierarchical_mutex other_mutex(6000);

int do_low_level_stuff() { return 1; }

int low_level_func() {
    std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
    return do_low_level_stuff();
}

void do_high_level_stuff(int some_param) {
    std::cout << "do_high_level_stuff: " << some_param << std::endl;
}

void high_level_func() {
    std::lock_guard<hierarchical_mutex> lk(high_level_mutex);
    do_high_level_stuff(low_level_func());
}

void do_other_stuff() {}

void other_func() {
    std::lock_guard<hierarchical_mutex> lk(other_mutex);
    high_level_func();
    do_other_stuff();
}

int main() {
    std::thread t1([] { high_level_func(); });

    std::thread t2([] { other_func(); });

    t1.join();
    t2.join();
}