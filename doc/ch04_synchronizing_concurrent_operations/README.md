# Synchronizing Concurrent Operations

有时不仅需要保护数据，还需要**同步不同线程上的操作**。例如，一个线程可能需要等待另一个线程完成任务，然后才能开始完成自己的任务。通常，希望线程等待特定事件发生或条件成立是很常见的。虽然可以通过定期检查“任务完成”标志或存储在共享数据中的类似内容来实现这一点，但这并不是最佳的做法。像这样需要在线程之间同步操作的情况非常常见，C++ 标准库以 *条件变量 (condition variables)* 和 *futures* 的形式提供了处理该问题的工具。这些工具在并发技术规范 (Technical Specification, TS) 中进行了扩展，它为 futures 提供了更多操作，同时以 *latches* 和 *barriers* 的形式提供了新的同步工具。

# The Outline
- [Waiting for an event or other condition](#等待事件或其他条件)
    - [Waiting for a condition with condition variables](#使用条件变量等待条件)
    - [Building a thread-safe queue with condition variables](#使用条件变量构建线程安全队列)
- [Waiting for one-off events with futures](#使用-future-等待一次性事件)
    - [Returning values from background tasks](#从后台任务返回值)
    - Associating a task with a future
    - Making (std::)promises
    - Saving an exception for the future
    - Waiting from multiple threads
- Waiting with a time limit
    - Clocks
    - Durations
    - Time points
    - Functions that accept timeouts
- Using synchronization of operations to simplify code
    - Functional programming with futures
    - Synchronizing operations with message passing
    - Continuation-style concurrency with the Concurrency TS
    - Chaining continuations
    - Waiting for more than one future
    - Waiting for the first future in a set with when_any
    - Latches and barriers in the Concurrency TS
    - A basic latch type: `std::experimental::latch`
    - `std::experimental::barrier`: a basic barrier
    - `std::experimental::flex_barrier` — `std::experimental::barrier`’s flexible friend

## 等待事件或其他条件
当一个线程等待另一个线程完成某项任务时，有这几种选择：

**选择 1**：
等待线程持续地检查共享数据中的完成标志（被 mutex 保护着），被等待的那个线程完成任务后设置标志。但这种做法是一种浪费：该线程消耗了宝贵的处理器时间来反复检查标志，并且当互斥锁被等待线程锁定时，它就不能被任何其他线程锁定。

**选择 2**：
在等待线程检查标志后，如果标志没有被设置，就释放锁让线程睡眠一段时间。这样做比第一种做法有提升，在线程睡眠的时间段不占用 CPU 时间，但是睡眠的时长无法掌控。睡的时间太短，还是会浪费处理器时间去做检查标志的工作；睡的时间太长，响应被等待线程就有延迟。
```cpp
bool flag;
std::mutex m;
void wait_for_flag() {
    std::unique_lock<std::mutex> lk(m);
    while (!flag) {
        lk.unlock(); // 解锁
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 睡100毫秒
        lk.lock(); // 接着上锁，检查标志
    }
}
```

**选择 3**：使用 C++ 标准库提供的**条件变量 (condition variables)**。从概念上讲，条件变量与事件或其他条件相关联，并且一个或多个线程可以等待该条件得到满足。当线程确定条件得到满足时，它可以通知一个或多个等待条件变量的线程，以唤醒它们并允许它们继续处理。

### 使用条件变量等待条件
C++ 标准库提供两种条件变量：`std::condition_variable` 和 `std::condition_variable_any`。它们都需要和 mutex 配套使用才能提供合适的同步功能：`std::condition_variable` 仅适用于 `std::unique_lock<std::mutex>`，目的是为了性能最大化；而 `std::condition_variable_any` 适用于所有满足 *BasicLockable* 要求的对象，更加灵活，同时开销也更大一些。

一个使用条件变量的例子（具体代码请见 [listing 4.1](../../src/ch04_synchronizing_concurrent_operations/listing_4_1.cc)）：
```cpp
std::mutex mtx;
std::queue<data_chunk> data_queue;  // 1
std::condition_variable data_cond;

void data_preparation_thread() {
    while (more_data_to_prepare()) {
        const data_chunk data = prepare_data();
        {
            std::lock_guard<std::mutex> lk(mtx);
            data_queue.push(data);  // 2
        }
        // 这行代码在锁之外，当等待线程被通知唤醒后，不必再去等待锁。
        // 如果将这行代码移动到锁的作用范围内，等待线程被通知唤醒后，还需要等待
        // 这个线程这边释放锁
        data_cond.notify_one();  // 3 通知一个等待线程
    }
}

void data_processing_thread() {
    while (true) {
        std::unique_lock<std::mutex> lk(mtx);  // 4
        // `wait()` 接受一个 `std::unique_lock` 类型参数 和 一个 `Predicate` 类型参数
        // 这里提供的 `Predicate` 是一个 lambda funciton
        //
        // wait 会检查 `Predicate` 是否成立（lambda 函数返回 true）：
        //   - 成立的话就返回
        //   - 不成立的话就解锁 mutex，让线程阻塞
        //
        // 当被另一个线程通过 条件变量 通知后，此线程被唤醒
        // 先尝试获取锁，接着检查条件是否成立：
        //   - 获取不到锁，阻塞
        //   - 获取了锁，条件也成立，就返回
        //   - 获取了锁，条件不成立，解锁接着阻塞
        data_cond.wait(lk, [] { return !data_queue.empty(); });  // 5
        data_chunk data = data_queue.front();
        data_queue.pop();
        lk.unlock();  // 6
        process(data);
        if (is_last_chunk(data)) {
            break;
        }
    }
}
```

从根本上讲，`std::condition_variable::wait` 是对 *忙等待* 的优化。事实上，符合要求（尽管不太理想）的实现技术只是一个简单的循环：
```cpp
template <typename Predicate>
void minimal_wait(std::unique_lock<std::mutex> &lk, Predicate pred) {
    while (!pred()) {
        lk.unlock();
        lk.lock();
    }
}
```

### 使用条件变量构建线程安全队列
`std::queue` 的接口：
```cpp
template <class T, class Container = std::deque<T> >
class queue {
public:
    explicit queue(const Container&);
    explicit queue(Container&& = Container());
    template <class Alloc> explicit queue(const Alloc&);
    template <class Alloc> queue(const Container&, const Alloc&);
    template <class Alloc> queue(Container&&, const Alloc&);
    template <class Alloc> queue(queue&&, const Alloc&);
    void swap(queue& q);
    bool empty() const;
    size_type size() const;
    T& front();
    const T& front() const;
    T& back();
    const T& back() const;
    void push(const T& x);
    void push(T&& x);
    void pop();
    template <class... Args> void emplace(Args&&... args);
};
```

需要实现的 `threadsafe_queue` 的接口：
```cpp
template<typename T>
class threadsafe_queue {
public:
    threadsafe_queue();
    threadsafe_queue(const threadsafe_queue &);
    threadsafe_queue& operator=(const threadsafe_queue &) = delete;
    void push(T new_value);
    bool try_pop(T& value);
    std::shared_ptr<T> try_pop();
    void wait_and_pop(T& value);
    std::shared_ptr<T> wait_and_pop();
    bool empty() const;
}
```

完整的实现代码请见 [listing 4.5](../../src/ch04_synchronizing_concurrent_operations/listing_4_5.hpp)。

## 使用 `future` 等待一次性事件
如果等待线程只等待一次，那么当条件为真时，它将永远不会再等待此条件变量，条件变量可能不是同步机制的最佳选择。如果等待的条件是特定数据的可用性，则尤其如此。在这种情况下，`future` 可能更合适。

C++ 标准库使用一种称为 `future` 的东西来建模这种一次性事件。如果线程需要等待特定的一次性事件，它会以某种方式获取代表该事件的 `future`。然后，线程可以定期在短时间内等待 `future`，以查看事件是否已发生，同时在检查之间执行其他任务。或者，它可以执行另一项任务，直到需要事件发生后才能继续，然后只需等待 `future` 准备就绪即可。

C++ 标准库提供两种 `future`s：`std::future<>` 和 `std::shared_future<>`。`std::future` 的实例是引用其关联事件的唯一实例，而 `std::shared_future` 的多个实例可能引用同一个事件。当事件发生后，`future` 实例准备就绪，可以通过它来获取与事件相关的数据。与事件相关的数据的类型也就是 `future` 的模板参数。在没有关联数据的地方应该使用 `std:future<void>` 和 `std::shared_future<void>` 模板特化。

```cpp
// defined in header <future>

template< class T > class future;      // (1) (since C++11)
template< class T > class future<T&>;  // (2) (since C++11)
template<> class future<void>;         // (3) (since C++11)

template< class T > class shared_future;      // (1) (since C++11)
template< class T > class shared_future<T&>;  // (2) (since C++11)
template<> class shared_future<void>;         // (3) (since C++11)
```

Concurrency TS 在 `std::experimental` 命名空间中提供了这些类模板的扩展版本： `std::experimental::future<>` 和 `std::experimental::shared_future<>`。它们的行为与 std 命名空间中的对应版本相同，但它们具有额外的成员函数来提供额外的功能。

### 从后台任务返回值
假设有一项耗时的计算任务，这个任务会返回一个期望的结果，但是现在并不着急需要这个结果。这种场景下，应该启动一个新线程来处理这项计算任务，等到结果就绪后或者主动需要结果时去获取这个结果。但是，`std::thread` 不能将结果传递给其他线程，这时候就需要 `std::async` 登场。

使用 `std::async` 启动异步任务后，不需要立即获得结果。`std::async` 不会给出一个 `std::thread` 对象让你等待，而是返回一个 `std::future` 对象，该对象最终将保存函数的返回值。当你需要该值时，只需在 `future` 对象上调用 `get()`，线程就会阻塞，直到 `future` 准备就绪，然后返回该值。

一个简单的使用 `std::async` 的例子（具体代码请见 [listing 4.6](../../src/ch04_synchronizing_concurrent_operations/listing_4_6.cc)）：
```cpp
int find_the_answer();
void do_other_stuff();

int main() {
    // the type of `ans` is: `std::future<int>`
    auto ans = std::async(find_the_answer);
    do_other_stuff();
    std::cout << "answer: " << ans.get() << std::endl;
}
```

与 `std::thread` 的构造函数类似，可以给 `std::async` 传递任务函数（不一定是函数，也可以是可调用的对象）的参数：
```cpp
struct X {
    void foo(int, const std::string &);
    std::string bar(const std::string &);
};
X x;
// 调用 p->foo(42, "hello")，这里的 p 是 &x
auto f1 = std::async(&X::foo, &x, 42, "hello");
// 调用 tmpx.bar("goodbye")，这里的 tmpx 是 x 的拷贝
auto f2 = std::async(&X::bar, x, "goodbye");

struct Y {
    double operator()(double);
};
Y y;
// 调用 tmpy(3.14)，这里的 tmpy 是从 Y() 移动构造的
auto f3 = std::async(Y(), 3.141);
// 调用 y(2.718)
auto f4 = std::async(std::ref(y), 2.718);

X baz(X &);
// 调用 baz(x)
std::async(baz, std::ref(x));

class move_only {
public:
    move_only();
    move_only(move_only&&)
    move_only(move_only const&) = delete;
    move_only& operator=(move_only&&);
    move_only& operator=(move_only const&) = delete;
    void operator()();
};
// 调用 tmp()，这里的 tmp 是从 std::move(move_only()) 构造的
auto f5 = std::async(move_only());
```