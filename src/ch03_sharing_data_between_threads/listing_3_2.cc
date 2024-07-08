// Listing 3.2 Accidentally passing out a reference to protected data
#include <mutex>
#include <string>

class some_data {
public:
    void do_something() {}
private:
    int a;
    std::string b;
};

class data_wrapper {
public:
    template <typename Func>
    void process_data(Func func) {
        std::lock_guard<std::mutex> lck(mtx);
        func(data); // 将被保护的数据传递给用户提供的函数
    }
private:
    std::mutex mtx;
    some_data data;
};

some_data *unproteced;
void malicious_function(some_data &protected_data) {
    unproteced = &protected_data;
}

data_wrapper x;
void foo() {
    x.process_data(malicious_function);  // 传入有恶意的函数
    unproteced->do_something(); // 不受保护地去访问受保护的数据
}

int main() {
    foo();
}