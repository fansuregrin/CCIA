#include <thread>
#include <utility>

void some_function() {}
void some_other_function() {}

int main() {
    // t1 与一个执行线程相关联
    std::thread t1(some_function);
    
    // t1 对相关联的执行线程的所有权 转移至 t2
    std::thread t2 = std::move(t1);
    
    // 一个临时 thread 对象被构造，它与一个执行线程相关联
    // 接着这个临时对象对执行线程的所有权转移至 t1
    t1 = std::thread(some_other_function);
    
    // 默认构造，不代表一个线程
    std::thread t3;
    // t2 对相关联的执行线程的所有权 转移至 t3
    t3 = std::move(t2);

    // 等待 t1 相关联的执行线程执行结束 
    t1.join();

    // 将 t3 相关联的执行线程的所有权转移至 t1
    // t1 相关联的执行线程已经结束了，不会调用 std::terminate()
    t1 = std::move(t3);

    t1.join();
}