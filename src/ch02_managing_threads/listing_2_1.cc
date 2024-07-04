// Listing 2.1 A function that returns while a thread still has access to local
// variables
#include <thread>

void do_something(int &i) {
    ++i;
}

struct func {
    int &i;
    func(int &i_) : i(i_) {}
    void operator()() {
        for (int j = 0; j < 1000000; ++j) {
            // potential access to dangling reference
            do_something(i);
        }
    }
};

void oops() {
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread my_thread(my_func);
    my_thread.detach();  // don't wait for thread to finish
}  // new thread might still be running

int main() {
    oops();
}