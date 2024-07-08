// Listing 3.6 Using std::lock() and std::lock_guard in a swap operation
#include <mutex>
#include <thread>
#include <utility>
#include <iostream>

class some_big_object {
public:
    some_big_object() : m_data(nullptr) {}
    some_big_object(int data) : m_data(new int(data)) {}
    some_big_object(const some_big_object &other)
        : m_data(new int(*other.m_data)) {}

    some_big_object &operator=(const some_big_object &rhs) {
        if (this == &rhs)
            return *this;
        free();
        m_data = new int(*rhs.m_data);
        return *this;
    }

    friend void swap(some_big_object &lhs, some_big_object &rhs);
    friend std::ostream &operator<<(std::ostream & os, const some_big_object &obj);

private:
    int *m_data;

    void free() {
        if (m_data) {
            delete m_data;
        }
    }
};

void swap(some_big_object &lhs, some_big_object &rhs) {
    if (&lhs == &rhs)
        return;
    std::swap(lhs.m_data, rhs.m_data);
}

class X {
public:
    X(const some_big_object &sd) : some_detail(sd) {}
    friend void swap(X &lhs, X &rhs);
    friend std::ostream &operator<<(std::ostream &os, const X &x);

private:
    some_big_object some_detail;
    mutable std::mutex mtx;
};

void swap(X &lhs, X &rhs) {
    if (&lhs == &rhs) {
        return;
    }
    std::lock(lhs.mtx, rhs.mtx);                                  // 1
    std::lock_guard<std::mutex> lock_a(lhs.mtx, std::adopt_lock); // 2
    std::lock_guard<std::mutex> lock_b(rhs.mtx, std::adopt_lock); // 3
    swap(lhs.some_detail, rhs.some_detail);
}

std::ostream &operator<<(std::ostream & os, const some_big_object &obj) {
    return os << *obj.m_data;
}

std::ostream &operator<<(std::ostream &os, const X &x) {
    std::lock_guard<std::mutex> lck(x.mtx);
    return os << x.some_detail;
}

int main() {
    X x1(some_big_object(1)), x2(some_big_object(2)), x3(some_big_object(3));
    
    std::thread t1([&] {
        swap(x1, x2);
    });

    std::thread t2([&] {
        swap(x2, x3);
    });

    t1.join();
    t2.join();

    std::cout << "x1: " << x1 << std::endl;
    std::cout << "x2: " << x2 << std::endl;
    std::cout << "x3: " << x3 << std::endl;
}