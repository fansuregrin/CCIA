# The Outline
- What is concurrency?
    - Concurrency in computer systems
    - Approaches to concurrency
    - Concurrency vs. parallelism
- Why use concurrency?
    - Using concurrency for separation of concerns
    - Using concurrency for performance: task and data parallelism
    - When not to use concurrency
- Concurrency and multithreading in C++
    - History of multithreading in C++
    - Concurrency support in the C++11 standard
    - More support for concurrency and parallelism in C++14 and C++17
    - Efficiency in the C++ Thread Library
    - Platform-specific facilities
- Getting started
    - Hello, Concurrent World

## 什么是并发
"CCIA" 中对并发的解释：
> 简单来说，*并发* 就是指两个活动 (activity) 同时发生。在计算机系统中，并发是指单个系统并行执行多个独立的活动，而不是依次或一个接一个地执行。

并发的形式可以分为两种：
- 通过 *任务切换 (task switching)* 展现出来的并发 (the illusion of concurrency)
- 在多个处理器或多个处理核心上并行地运行任务 (hardware concurrency)

![two forms of concurrency](../imgs/fig-1.1-two_approaches_to_concurrency.png)

