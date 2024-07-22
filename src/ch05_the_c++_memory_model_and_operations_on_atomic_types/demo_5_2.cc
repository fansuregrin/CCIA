#include <atomic>
#include <iostream>
#include <iomanip>

int main() {
    constexpr int COL_WIDTH = 20;
    std::cout << std::boolalpha << std::left
        << std::setw(COL_WIDTH) << "Atomic Type" << "Lock Free\n"
        << std::setw(COL_WIDTH) << "std::atomic_bool" << std::atomic_bool(false).is_lock_free() << '\n'
        << std::setw(COL_WIDTH) << "std::atomic_int" << std::atomic_int(1).is_lock_free() << '\n'
        << std::setw(COL_WIDTH) << "std::atomic_char" << std::atomic_char('x').is_lock_free() << '\n'
        << std::setw(COL_WIDTH) << "std::atomic_long" << std::atomic_long(2L).is_lock_free() << '\n'
        << std::setw(COL_WIDTH) << "std::atomic_ulong" << std::atomic_ulong(4UL).is_lock_free() << '\n'
        << std::setw(COL_WIDTH) << "std::atomic_uint" << std::atomic_uint(3U).is_lock_free() << '\n';
}