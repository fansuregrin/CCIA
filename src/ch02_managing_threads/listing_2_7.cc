// Listing 2.7 A joining_thread class
#include <thread>

class joining_thread {
public:
    joining_thread() = default;

    template <typename Callable, typename... Args>
    explicit joining_thread(Callable &&func, Args &&...args)
        : t(std::forward<Callable>(func), std::forward<Args>(args)...) {}

    explicit joining_thread(std::thread t_) noexcept : t(std::move(t_)) {}

    explicit joining_thread(joining_thread &&other) noexcept
        : t(std::move(other.t)) {}

    joining_thread &operator=(joining_thread &&rhs) noexcept {
        if (joinable()) {
            join();
        }
        t = std::move(rhs.t);
        return *this;
    }

    joining_thread &operator=(std::thread rhs) noexcept {
        if (joinable()) {
            join();
        }
        t = std::move(rhs);
        return *this;
    }

    ~joining_thread() {
        if (joinable()) {
            join();
        }
    }

    void swap(joining_thread &other) noexcept { t.swap(other.t); }

    std::thread::id get_id() const noexcept { return t.get_id(); }

    bool joinable() const noexcept { return t.joinable(); }

    void join() { t.join(); }

    void detach() { t.detach(); }

    std::thread &as_thread() noexcept { return t; }

    const std::thread &as_thread() const noexcept { return t; }

private:
    std::thread t;
};