#include <atomic>
#include <iostream>
#include <iomanip>

int main() {
    constexpr int COL_WIDTH = 30;
    std::cout << std::boolalpha << std::left
        << std::setw(COL_WIDTH) << "Atomic Type" << "Always Lock Free\n"
        << std::setw(COL_WIDTH) << "std::atomic<int>" << std::atomic<int>::is_always_lock_free << "\n"
        << std::setw(COL_WIDTH) << "std::atomic<bool>" << std::atomic<bool>::is_always_lock_free << "\n"
        << std::setw(COL_WIDTH) << "std::atomic<char>" << std::atomic<char>::is_always_lock_free << "\n"
        << std::setw(COL_WIDTH) << "std::atomic<long>" << std::atomic<long>::is_always_lock_free << "\n"
        << std::setw(COL_WIDTH) << "std::atomic<uintmax_t>" << std::atomic<uintmax_t>::is_always_lock_free << "\n";
}