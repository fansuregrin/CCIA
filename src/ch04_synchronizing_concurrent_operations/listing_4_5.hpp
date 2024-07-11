// Listing 4.5 Full class definition of a thread-safe queue using condition
// variables
#ifndef THREADSAFE_QUEUE_HPP
#define THREADSAFE_QUEUE_HPP
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

template <typename T> class threadsafe_queue {
private:
    std::queue<T> data_queue;
    std::condition_variable data_cond;
    mutable std::mutex mtx;

public:
    threadsafe_queue() {}

    threadsafe_queue(const threadsafe_queue &other) {
        std::lock_guard<std::mutex> lk(other.mtx);
        data_queue = other.data_queue;
    }

    threadsafe_queue &operator=(const threadsafe_queue &rhs) = delete;

    void push(T new_value) {
        std::lock_guard<std::mutex> lk(mtx);
        data_queue.push(new_value);
        data_cond.notify_one();
    }

    void wait_and_pop(T &value) {
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        value = data_queue.front();
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    bool try_pop(T &value) {
        std::lock_guard<std::mutex> lk(mtx);
        if (data_queue.empty()) {
            return false;
        }
        value = data_queue.front();
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mtx);
        if (data_queue.empty()) {
            return std::shared_ptr<T>();
        }
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mtx);
        return data_queue.empty();
    }
};

#endif // end of THREADSAFE_QUEUE_HPP