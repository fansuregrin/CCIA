// Listing 2.6 scoped_thread and example usage
#include <stdexcept>
#include <thread>
#include <utility>

class scoped_thread {
public:
    explicit scoped_thread(std::thread t_) : t(std::move(t_)) {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }

    ~scoped_thread() { t.join(); }

    scoped_thread(const scoped_thread &) = delete;
    scoped_thread &operator=(const scoped_thread &) = delete;

private:
    std::thread t;
};

void do_something(int &i) { ++i; }

struct func {
    int &i;

    func(int &i_) : i(i_) {}

    void operator()() {
        for (unsigned j = 0; j < 1000000; ++j) {
            do_something(i);
        }
    }
};

void do_something_in_current_thread() {}

void f() {
    int some_local_state;
    scoped_thread t(std::thread(func{some_local_state}));
}

int main() { f(); }
