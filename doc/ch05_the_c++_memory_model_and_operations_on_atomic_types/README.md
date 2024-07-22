# The C++ Memory Model and Operations on Atomic Types
## The Outline
- [Memory model basics](#内存模型基础)
    - [Objects and memory locations](#对象和内存位置)
    - [Objects, memory locations, and concurrency](#对象内存位置和并发)
    - [Modification orders](#修改顺序)
- [Atomic operations and types in C++](#c-原子操作和原子类型)
    - [The standard atomic types](#标准原子类型)
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

### 修改顺序
在 C++ 程序中，每一个对象都有一个修改顺序，它由程序中所有线程对这个对象的写入操作组成。在大多数情况下，多次运行一个程序，每一次运行中对象的修改顺序可能不同；但是，在某一次运行中，所有线程都同意对这个对象的修改顺序。也就是说，每次运行过程中，所有线程看到某个对象的变化序列是一样的。使用原子操作，编译器会确保这种同步。

## C++ 原子操作和原子类型
原子操作是不可分割的操作，不能从任何一个线程观察到操作的中间状态，这个操作要么完成要么还未开始。在 C++ 中，在大多数情况下需要通过原子类型来实现原子操作。

### 标准原子类型
标准原子类型可以在头文件 `<atomic>` 中找到，在这些原子类型上的所有操作都是原子的。这些原子类型几乎都有一个成员函数：`is_lock_free()`，它可以判定在某个类型上的原子操作是直接由原子指令完成（`is_lock_free()` 返回 `true`）还是由编译器或标准库提供的锁完成（`is_lock_free()` 返回 `false`），示例代码请见[demo 5.2](../../src/ch05_the_c++_memory_model_and_operations_on_atomic_types/demo_5_2.cc)。在大多数情况下，使用原子操作是为了替代锁来实现同步，如果原子操作本身是用内部锁来实现的话，那么使用原子操作的性能优势就可能无法达成。

自从 C++ 17，所有的原子类型都有一个 `static consexpr` 成员变量：`X::is_always_lock_free`，仅当原子类型 `X` 在所有受支持的硬件是都是无锁的，`is_always_lock_free` 才为 `true`。示例代码请见 [demo 5.3](../../src/ch05_the_c++_memory_model_and_operations_on_atomic_types/demo_5_3.cc)。

C++ 标准库还提供了一系列宏在编译时识别整型原子类型是否无锁。这些宏是：
- `ATOMIC_BOOL_LOCK_FREE`
- `ATOMIC_CHAR_LOCK_FREE`
- `ATOMIC_CHAR16_T_LOCK_FREE`
- `ATOMIC_CHAR32_T_LOCK_FREE`
- `ATOMIC_WCHAR_T_LOCK_FREE`
- `ATOMIC_SHORT_LOCK_FREE`
- `ATOMIC_INT_LOCK_FREE`
- `ATOMIC_LONG_LOCK_FREE`
- `ATOMIC_LLONG_LOCK_FREE`
- `ATOMIC_POINTER_LOCK_FREE`
宏的取值有 0、1、2，0 表示原子类型永远不是无锁的，2 表示原子类型总是无锁的，1 表示有锁无锁取决于运行时。代码示例请见 [demo 5.4](../../src/ch05_the_c++_memory_model_and_operations_on_atomic_types/demo_5_4.cc)。

C++ 中提供的原子类型有 `std::atomic_flag` 和 `std::atomic<>`。可以直接使用 `std::atomic<>` 模板，也可以使用别名。原子类型的别名和对应的特化模板如下表所示：

| 原子类型 | 对应的模板特化 |
| :-: | :-: |
| atomic_bool | std::atomic\<bool> |
| atomic_char | std::atomic\<char> |
| atomic_schar | std::atomic\<signed char> |
| atomic_uchar | std::atomic\<unsigned char> |
| atomic_int | std::atomic\<int> |
| atomic_uint | std::atomic\<uint> |
| atomic_short | std::atomic\<short> |
| atomic_ushort | std::atomic\<unsigned short> |
| atomic_long | std::atomic\<long> |
| atomic_ulong | std::atomic\<unsigned long> |
| atomic_llong | std::atomic\<long long> |
| atomic_ullong | std::atomic\<unsigned long long> |
| atomic_char16_t | std::atomic\<char16_t> |
| atomic_char32_t | std::atomic\<char32_t> |
| atomic_wchar_t | std::atomic\<wchar_t> |

这些原子类型没有传统意义上的可拷贝性和可赋值性，即它们没有拷贝构造函数和赋值操作符。但是，它们支持从内置类型赋值以及隐式转化成内置类型。这些原子类型有：`store()`、`load()`、`exchange()`、`compare_exchange_weak()`、`compare_exchange_strong()` 成员函数，支持复合赋值操作符：`+=`、`-=`、`*=`、`|=` 等。

原子类型上的操作都接受一个可选的参数，它是一个 `std::memory_order` 枚举类型，它指定内存顺序语义。这个参数的取值有 6 个:
- `std::memory_order_relaxed`
- `std::memory_order_acquire`
- `std::memory_order_consume`
- `std::memory_order_acq_rel`
- `std::memory_order_release`
- `std::memory_order_seq_cst`
没有指定这个参数的话，默认值是 `std::memory_order_seq_cst`。不同种类的操作可以指定的内存顺序如下所示：
- *Store* 操作：可以指定 `std::memory_order_relaxed`、`std::memory_order_release`、`std::memory_order_seq_cst`。
- *Load* 操作：可以指定 `std::memory_order_relaxed`、`std::memory_order_acquire`、`std::memory_order_seq_cst`。
- *Read-modify-write* 操作：可以指定 `std::memory_order_relaxed`、`std::memory_order_consume`、`std::memory_order_acquire`、`std::memory_order_release`、`std::memory_order_acq_rel`、`std::memory_order_seq_cst`。