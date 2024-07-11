# Synchronizing Concurrent Operations

有时不仅需要保护数据，还需要**同步不同线程上的操作**。例如，一个线程可能需要等待另一个线程完成任务，然后才能开始完成自己的任务。通常，希望线程等待特定事件发生或条件成立是很常见的。虽然可以通过定期检查“任务完成”标志或存储在共享数据中的类似内容来实现这一点，但这并不是最佳的做法。像这样需要在线程之间同步操作的情况非常常见，C++ 标准库以 *条件变量 (condition variables)* 和 *futures* 的形式提供了处理该问题的工具。这些工具在并发技术规范 (Technical Specification, TS) 中进行了扩展，它为 futures 提供了更多操作，同时以 *latches* 和 *barriers* 的形式提供了新的同步工具。

# The Outline
- Waiting for an event or other condition
    - Waiting for a condition with condition variables
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
