// deadlock with no locks
#include <thread>

void func(std::thread &t) {
    t.join();  // 等待另一个线程结束
}

int main() {
    std::thread t1, t2;
    t1 = std::thread(func, std::ref(t2));
    t2 = std::thread(func, std::ref(t1));

    t1.join();
    t2.join();
}