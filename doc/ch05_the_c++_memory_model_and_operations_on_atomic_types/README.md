# The C++ Memory Model and Operations on Atomic Types
## The Outline
- [Memory model basics](#内存模型基础)
    - [Objects and memory locations](#对象和内存位置)
    - [Objects, memory locations, and concurrency](#对象内存位置和并发)
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

### 对象和内存位置
C++ 程序中的所有数据都是由对象（object）组成的。这是关于 C++ 中数据构建块的陈述。C++ 标准将对象定义为“存储区域”，尽管它继续为这些对象分配属性，例如它们的类型和生命周期。例如：有些对象是基础类型的值（如一个 `int` 或一个 `float`）；有些对象是用户定义的类型的实例；有些对象还包含子对象等。

无论对象是什么类型，它都会存储在一个或多个内存位置（memory location）中。每个内存位置要么是标量类型的对象（或子对象），例如 `unsigned short` 或 `my_class*`，要么是相邻位域（adjacent bit fields）的序列。在使用位域时，尽管相邻的位域是不同的对象，但是它们仍然被视为同一个内存位置。

下面的例子展示了一个 struct 分割为对象和内存位置（具体代码请见[demo 5.1](../../src/ch05_the_c++_memory_model_and_operations_on_atomic_types/demo_5_1.cc)）：
```cpp
struct my_data {
    int i;
    double d;
    unsigned bf1:10;
    int bf2:25;
    // int bf3:0;  // zero-length bit field must be unnamed
    int :0;
    int bf4:9;
    int i2;
    char c1, c2;
    std::string s;
};

my_data x;
```
首先 `my_data` 的实例 `x` 是一个对象，它由多个子对象组成，每个子对象就是它的一个成员。`bf1` 和 `bf2` 共享同一个内存位置；`s` 由多个内部内存位置组成。`bf3` 是一个长度为 0 的位域，它会将 `bf4` 分隔开，也就是说 `bf4` 在另一个独立的内存位置，而 `bf3` 没有内存位置。除此之外，`my_data` 的其他成员都有自己独立的内存位置。
```
------------------------
|           i          |
|----------------------|
|           d          |
|----------------------|
|      bf1  |    bf2   |
|----------------------|
|          bf4         |
|----------------------|
|          i2          |
|----------------------|
|          c1          |
|----------------------|
|          c2          |
|----------------------|
|          s           |
| ---     ---     ---  |
| | | ... | | ... | |  |
| ---     ---     ---  |
|----------------------|
```

综上，有四件重要的事情需要注意：
- 每个变量都是一个对象，包括那些作为其他对象的成员的变量。
- 每个对象至少占用一个内存位置。
- 基本类型的变量（如 `int` 或 `char`）只占用一个内存位置，无论其大小如何，即使它们是相邻的或属于数组的一部分。
- 相邻的位域是同一内存位置的一部分。

### 对象、内存位置和并发
如果来自不同线程对单个内存位置的两次访问之间没有强制排序，则其中一个或两个访问都不是原子的，并且如果其中一个或两个都是写入，那么这就是一个数据争用并导致未定义的行为。

- 未定义的行为会造成不可预料的问题。
- 可以通过原子操作来访问参与竞争的内存位置来避免未定义的行为。
