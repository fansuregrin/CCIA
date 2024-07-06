# Managing Threads

## The Outline
- [Basic thread management](#线程的基本操作)
    - [Launching a thread](#启动线程)
    - [Waiting for a thread to complete](#等待线程完成)
    - [Waiting in exceptional circumstances](#有异常发生的情况下的等待)
    - [Running threads in the background](#让线程在后台运行)
- [Passing arguments to a thread function](#向线程函数传递参数)
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

```cpp
#include <thread>

void do_something_in_current_thread() {
    throw 1;
}

int main() {
    int a;
    std::thread t([&] {
        for (int i=0; i<100000; ++i) {
            ++a;
        }
    });
    do_something_in_current_thread();
    t.join();
}
```

因此，为了避免有异常抛出时程序被终止，在处理异常时也需要调用 `join()`。
```cpp
#include <thread>

void do_something_in_current_thread() {
    throw 1;
}

int main() {
    int a;
    std::thread t([&] {
        for (int i=0; i<100000; ++i) {
            ++a;
        }
    });
    try {
        do_something_in_current_thread();
    } catch (...) {
        t.join();
        throw;
    }
    t.join();
}
```

但是，`try/catch` 块的使用非常冗长，而且很容易让范围稍微出错，所以这不是一个理想的解决办法。一种更好的办法是，利用 *RAII机制 (资源获取即初始化, Resource Acquisition Is Initialization)* 来提供一个类，这个类在析构函数中来调用 `join()`。这样的类对创建的线程对象起到了守卫的作用。案例请见 [listing 2.2](../../src/ch02_managing_threads/listing_2_2.cc)。

### 让线程在后台运行

在线程对象上调用 `detach()` 会让线程在后台运行，无法通过直接的方式与其通信，也不能通过 `join()` 来等待其完成。detached 后的线程的所有权和控制权都移交给 C++ Runtime Library，它会确保当脱离线程退出时与其相关的资源会被正确的回收。

```cpp
#include <thread>

int main() {
    std::thread t([] {});
    t.detach();
    assert(!t.joinable());  // 脱离后的线程是不能被join的
}
```

不能在没有关联执行线程的 `std::thread` 对象上调用 `detach()`：
```cpp
#include <thread>

int main() {
    std::thread t;
    t.detach(); // 运行出错
}
```

一个使用脱离线程的案例：[listing 2.4](../../src/ch02_managing_threads/listing_2_4.cc)。

补充说明:
主线程退出后，被分离的线程依然在继续运行吗？

当主线程（即main函数所在的那个线程）退出时，操作系统一般会将这个进程下的其他线程结束掉。

在 Linux 系统上，主线程返回时，其他线程会被终止。`pthread_detach` 的 man 手册有一段话验证了这一点：
> The detached attribute merely determines the behavior of the system when the thread terminates; it does not prevent the thread from being terminated if the process terminates using exit(3) (or equivalently, if the main thread returns).

在 Windows 系统上，应该也是同样的行为。因为，通过下面的例子可以验证。

请看示例代码 [demo_2_1](../../src/ch02_managing_threads/demo_2_1.cc)，如果 `main` 函数在返回前没有执行 `std::this_thread::sleep_for(std::chrono::seconds(5));` 的话，是看不到 `output.txt` 中有写入内容的，甚至 `output.txt` 都来不及创建，写文件的那个线程就被结束掉了。如果主线程延迟退出，就能看到创建了文件 `output.txt` 以及其中写入了 3 行 "hello"。

## 向线程函数传递参数
`std::thread` 的构造函数原型如下：
```cpp
template< class F, class... Args >
explicit thread( F&& f, Args&&... args );
```

它创建一个新的 `std::thread` 对象并将其与执行线程关联。新的执行线程开始执行：
```cpp
// until c++23
INVOKE(decay-copy(std::forward<F>(f)),
       decay-copy(std::forward<Args>(args))...)
// since c++23
std::invoke(auto(std::forward<F>(f)),
            auto(std::forward<Args>(args))...)
```
`decay-copy` 的调用在当前线程中进行求值，因此在求值和复制/移动参数期间引发的任何异常都会在当前线程中引发，而无需启动新线程。当对构造函数的调用完成后，新的执行线程开始调用 `f`。

默认情况下，参数被复制到内部存储中，新创建的执行线程可以访问它们，然后将它们作为右值传递给可调用对象或函数，就好像它们是临时的一样。

```cpp
void f(int i, const std::string &s);

std::thread t(f, 3, "hello");
```
上面这段代码中，"hello" 存储在 .rodata，通过类型 `char const (&) [6]` 传递给 `std::thread` 的构造函数，经过 `decay-copy` 之后以类型 `char *` 传递给函数 `f`。所以，在新的执行线程中，会将 `char *` 转化成 `std::string`，再调用 `f`。

```cpp
void f(int i, const std::string &s);

void oops(int some_param) {
    char buffer[1024];
    sprintf(buffer, "%i", some_param);
    std::thread t(f, 3, buffer);
    t.detach();
}
```
而对于这段代码，`buffer` 是 `oops` 中的一个局部变量，当 `oops` 返回时可能新线程中还没有完成从 `char *` 转化成 `std::string` 的操作，而局部变量 `buffer` 已经不存在，会导致未定义的行为。

解决办法就是在传递 `buffer` 给 `std::thread` 的构造函数之前，将 `buffer` 转化成 `std::string`：
```cpp
void f(int i, const std::string &s);

void not_oops(int some_param) {
    char buffer[1024];
    sprintf(buffer, "%i", some_param);
    std::thread t(f, 3, std::string(buffer));
    t.detach();
}
```

### 向线程执行函数传递参数时，参数类型的隐式转换发生在哪个线程

参数类型的隐式转换应该发生在新启动的线程中。请见示例代码 [demo_2_2.cc](../../src/ch02_managing_threads/demo_2_2.cc)。线程执行函数 `f` 接受一个 `int` 类型参数 `i` 和 `A` 类型参数 `a`，它会将 `a` 中的字符串输出 `i` 次。`A` 类型支持从 `char *` 类型隐式转换而来。在函数 `oops` 中，如果给线程构造函数传入的是 `buffer`，那么就会发生隐式的类型转换，程序的执行结果如下：

```
main thread: 140022842861376
opps exited
conversion happens in thread: 140022842857216
conversion finished
abcdefghijk
abcdefghijk
abcdefghijk
```

可以看到，当函数 `oops` 退出时，从 `char *` 到 `A` 类型的转换还没有结束，但是 `buffer` 指向的那块字符数组的内容已经被改变了，新线程的执行函数 `f` 输出的结果并不是我们期望的 "12345"，这就发生了未定义行为。

如果我们在给线程函数传递参数时，显示的将 `buffer` 转换成 `A` 类型，就不存在这个问题了：
```
main thread: 139829774890816
conversion happens in thread: 139829774890816
conversion finished
opps exited
12345
12345
12345
```

这个示例同样说明了：向线程执行函数传递参数时，参数类型的隐式转换发生在新启动的线程中。

参考链接：
- [std::thread::thread - cppref](https://en.cppreference.com/w/cpp/thread/thread/thread)
- [when-does-the-conversion-happen-when-passing-arguments-to-thread-function](https://stackoverflow.com/questions/73725046/when-does-the-conversion-happen-when-passing-arguments-to-thread-function)