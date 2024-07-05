# Managing Threads

## The Outline
- Basic thread management
    - Launching a thread
    - Waiting for a thread to complete
    - Waiting in exceptional circumstances
    - Running threads in the background
- Passing arguments to a thread function
- Transferring ownership of a thread
- Choosing the number of threads at runtime
- Identifying threads

## 线程的基本操作
### 启动线程
通过构造 `std::thread` 对象来启动线程，该对象需要指定要在该线程上运行的任务 (即创建对象时，需要传入一个参数，这个参数的类型是一个可调用的类型)。

传入一个函数：
```cpp
#include <thread>

void do_some_work() {}

int main() {
    std::thread my_thread(do_some_work); // 传入一个函数
    my_thread.join();
}
```

传入一个重载了函数调用操作符 (function call operator) 的类的对象：
```cpp
#include <thread>

void do_something() {}
void do_something_else() {}

class background_task
{
public:
    void operator()() const {
        do_something();
        do_something_else();
    }
};

int main() {
    background_task f;
    std::thread my_thread(f);
    my_thread.join();
}
```

需要注意的是，如果在创建线程时传入的是一个临时的函数对象，需要避免 [*最令人烦恼的解析 (C++'s most vexing parse)*](https://en.wikipedia.org/wiki/Most_vexing_parse)。例如，`std::thread t(background_task());`会被编译器解析为一个函数声明，这个名为`t`的函数返回一个`std::thread`类型，接受一个类型为`background_task()`的参数，这个参数类型是一个函数指针，指向的函数返回类型是`background_task`，不接受参数。要解决这个解析问题，可以改写成：`std::thread t((background_task()));` 或 `std::thread t(background_task{});`。

传入一个 `lambda`：
```cpp
#include <thread>

void do_something(int x) {}
void do_something_else() {}

int main() {
    int a;
    std::thread t([=]{
        do_something(a);
        do_something_else();
    });
    t.join();
}
```

线程启动后，需要显式地决定是 *等待线程结束* 还是 *放任线程独自运行*。如果在线程对象销毁之前没有下决定，程序就会终止 (terminated)。
```cpp
#include <thread>

int main() {
    std::thread t([] {});
}
```
编译并运行：
```
terminate called without an active exception
Aborted (core dumped)
```

**如果不等待线程完成，则需要确保线程访问的数据在线程完成之前有效！** 否则，就会出现未定义的行为。
例如，[Listing 2.1](../../src/ch02_managing_threads/listing_2_1.cc) 中，主线程执完函数`oops()`后，局部变量 `some_local_state` 被销毁，而另一个线程继续通过引用来访问这个变量的内存地址，会导致未定义的行为。

### 等待线程完成
使用 `join()` 来等待线程完成。`join()` 会清除与这个线程相关的资源，这个 `std::thread` 对象也不再与这个已完成的线程有联系，并且这个线程对象不与任何线程相关联。因此，一个线程只能被 join 一次。

```cpp
#include <thread>

int main() {
    std::thread t([] {});
    t.join();
    t.join();
}
```
编译并运行：
```
terminate called after throwing an instance of 'std::system_error'
  what():  Invalid argument
Aborted (core dumped)
```

### 有异常发生的情况下的等待
在线程对象被销毁之前需要显式地调用`join()`或`detach()`，否则程序就会终止。如果需要让线程脱离，可能线程被创建后就立即调用`detach()`。但是，调用`join()`可能并不这样做，因为，在让这个新线程 join 之前，主线程还有其他事情要做。问题在于，在创建线程和调用`join()`之间的代码执行可能会发生异常，这就会跳过 `join()`，从而导致程序终止。

### 让线程在后台运行