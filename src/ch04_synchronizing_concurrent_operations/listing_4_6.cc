// Listing 4.6 Using std::future to get the return value of an asynchronous task
#include <future>
#include <iostream>
#include <random>

int find_the_answer() {
    int ans = rand() % 1000;
    while (ans < 512) {
        ans = rand() % 1000;
    }
    return ans;
}

void do_other_stuff() { std::cout << "do other sutff..." << std::endl; }

int main() {
    // the type of `ans` is: `std::future<int>`
    auto ans = std::async(find_the_answer);
    do_other_stuff();
    std::cout << "answer: " << ans.get() << std::endl;
}