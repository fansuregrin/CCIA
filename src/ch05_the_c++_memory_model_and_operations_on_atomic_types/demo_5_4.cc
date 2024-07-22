#include <atomic>
#include <iostream>
#include <iomanip>

int main() {
    constexpr int COL_WIDTH = 15;
    // 0: never lock-free
    // 1: is run-time property
    // 2: always lock-free
    std::cout << std::boolalpha << std::left
        << std::setw(COL_WIDTH) << "Atomic Type" << "Lock-free Status\n"
        << std::setw(COL_WIDTH) << "bool" << ATOMIC_BOOL_LOCK_FREE << "\n"
        << std::setw(COL_WIDTH) << "char" << ATOMIC_CHAR_LOCK_FREE << "\n"
        << std::setw(COL_WIDTH) << "wchar" << ATOMIC_WCHAR_T_LOCK_FREE << "\n"
        << std::setw(COL_WIDTH) << "char16" << ATOMIC_CHAR16_T_LOCK_FREE << "\n"
        << std::setw(COL_WIDTH) << "char32" << ATOMIC_CHAR32_T_LOCK_FREE << "\n"
        << std::setw(COL_WIDTH) << "short" << ATOMIC_SHORT_LOCK_FREE << "\n"
        << std::setw(COL_WIDTH) << "int" << ATOMIC_INT_LOCK_FREE << "\n"
        << std::setw(COL_WIDTH) << "long" << ATOMIC_LONG_LOCK_FREE << "\n"
        << std::setw(COL_WIDTH) << "long long" << ATOMIC_LLONG_LOCK_FREE << "\n"
        << std::setw(COL_WIDTH) << "pointer" << ATOMIC_POINTER_LOCK_FREE << "\n";
}