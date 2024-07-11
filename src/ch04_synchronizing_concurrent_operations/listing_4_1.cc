// Listing 4.1 Waiting for data to process with `std::condition_variable`
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>
#include <iostream>
#include <random>
#include <chrono>

struct data_chunk {
    int id;
    bool last;

    data_chunk(int id_): id(id_), last(false) {}
};

data_chunk prepare_data() {
    // preparing...
    std::this_thread::sleep_for(std::chrono::milliseconds(rand()%1000));
    return data_chunk(rand());
}

void process(data_chunk &data) {
    std::cout << "processed data_chunk: " << data.id << std::endl;
}

bool is_last_chunk(data_chunk &data) { return data.last; }

std::mutex mtx;
std::queue<data_chunk> data_queue;
std::condition_variable data_cond;

void data_preparation() {
    const int num = 10;
    for (int i=0; i<num; ++i) {
        data_chunk data = prepare_data();
        if (i == num-1) { data.last = true; }
        {
            std::lock_guard<std::mutex> lk(mtx);
            data_queue.push(data);
        }
        data_cond.notify_one();
    }
}

void data_processing() {
    while (true) {
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk, [] { return !data_queue.empty(); });
        data_chunk data = data_queue.front();
        data_queue.pop();
        lk.unlock();
        process(data);
        if (is_last_chunk(data)) {
            break;
        }
    }
}

int main() {
    std::thread t1(data_processing);
    std::thread t2(data_preparation);
    t1.join();
    t2.join();
}