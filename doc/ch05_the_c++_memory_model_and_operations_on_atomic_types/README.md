# The C++ Memory Model and Operations on Atomic Types
## The Outline
- [Memory model basics](#内存模型基础)
    - Objects and memory locations
    - Objects, memory locations, and concurrency
    - Modification orders
- Atomic operations and types in C++
    - The standard atomic types
    - Operations on `std::atomic_flag`
    - Operations on `std::atomic<bool>`
    - Operations on `std::atomic<T*>`: pointer arithmetic
    - Operations on standard atomic integral types
    - The `std::atomic<>` primary class template
    - Free functions for atomic operations
- Synchronizing operations and enforcing ordering
    - The synchronizes-with relationship
    - The happens-before relationship
    - Memory ordering for atomic operations
    - Release sequences and synchronizes-with
    - Fences
    - Ordering non-atomic operations with atomics
    - Ordering non-atomic operations

## 内存模型基础
内存模型有两个方面：基本结构方面（涉及事物在内存中的布局方式）和并发方面。
