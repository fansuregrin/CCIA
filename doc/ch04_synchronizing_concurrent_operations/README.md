# Synchronizing Concurrent Operations

有时不仅需要保护数据，还需要**同步不同线程上的操作**。例如，一个线程可能需要等待另一个线程完成任务，然后才能开始完成自己的任务。通常，希望线程等待特定事件发生或条件成立是很常见的。虽然可以通过定期检查“任务完成”标志或存储在共享数据中的类似内容来实现这一点，但这并不是最佳的做法。像这样需要在线程之间同步操作的情况非常常见，C++ 标准库以 *条件变量 (condition variables)* 和 *futures* 的形式提供了处理该问题的工具。这些工具在并发技术规范 (Technical Specification, TS) 中进行了扩展，它为 futures 提供了更多操作，同时以 *latches* 和 *barriers* 的形式提供了新的同步工具。

# The Outline
- [Waiting for an event or other condition](#等待事件或其他条件)
    - [Waiting for a condition with condition variables](#使用条件变量等待条件)
    - Building a thread-safe queue with condition variables
- Waiting for one-off events with futures
    - Returning values from background tasks
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