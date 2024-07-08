// Listing 3.5 A fleshed-out class definition for a thread-safe stack
#include <exception>
#include <memory>
#include <mutex>
#include <stack>
#include <thread>
#include <chrono>
#include <iostream>

struct empty_stack : std::exception {
    const char *what() const noexcept {
        return "empty stack";
    }
};

template <typename T>
class threadsafe_stack {
public:
    threadsafe_stack() {}
    
    threadsafe_stack(const threadsafe_stack &other) {
        std::lock_guard<std::mutex> lck(mtx);
        data = other.data;
    }

    threadsafe_stack &operator=(const threadsafe_stack &) = delete;

    void push(T new_value) {
        std::lock_guard<std::mutex> lck(mtx);
        data.push(std::move(new_value));
    }

    void pop(T &value) {
        std::lock_guard<std::mutex> lck(mtx);
        if (data.empty()) {
            throw empty_stack();
        }
        value = data.top();
        data.pop();
    }

    std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> lck(mtx);
        if (data.empty()) {
            throw empty_stack();
        }
        const std::shared_ptr<T> res(std::make_shared<T>(data.top()));
        data.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lck(mtx);
        return data.empty();
    }
private:
    std::stack<T> data;
    mutable std::mutex mtx;
};

int main()
{
    threadsafe_stack<int> si;
    std::thread t1([&] {
        for (int i=0; i<10; ++i) {
            si.push(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    std::thread t2([&] {
        for (int i=0; i<10; ++i) {
            while (si.empty()) {
                std::this_thread::yield();
            }
            std::cout << *si.pop() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    t1.join();
    t2.join();
}