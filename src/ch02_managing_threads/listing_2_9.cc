// Listing 2.9 A naïve parallel version of std::accumulate
#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>
#include <thread>
#include <vector>
#include <chrono>

using steady_clock = std::chrono::steady_clock;

template <typename Iterator, typename T> struct accumulate_block {
    void operator()(Iterator first, Iterator last, T &result) {
        result = std::accumulate(first, last, result);
    }
};

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init) {
    const unsigned long length = std::distance(first, last);
    const unsigned long min_per_thread = 25;
    const unsigned long max_threads =
        (length + min_per_thread - 1) / min_per_thread;
    const unsigned long hardware_threads = std::thread::hardware_concurrency();
    const unsigned long num_threads =
        std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
    const unsigned long block_size = length / num_threads;

    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);
    Iterator block_start = first;
    for (unsigned long i = 0; i < (num_threads - 1); ++i) {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = std::thread(
            accumulate_block<Iterator, T>(), block_start, block_end,
            std::ref(results[i]));
        block_start = block_end;
    }
    accumulate_block<Iterator, T>()(block_start, last,
                                    results[num_threads - 1]);

    for (auto &thread : threads) {
        thread.join();
    }

    return std::accumulate(results.begin(), results.end(), init);
}

int main() {
    std::vector<long> vi;
    const long n = 10'000'000;
    for (long i = 0; i < n; ++i) {
        vi.push_back(1);
    }

    long sum;
    auto t_start = steady_clock::now();
    sum = parallel_accumulate(vi.begin(), vi.end(), 0);
    auto elapse = std::chrono::duration_cast<std::chrono::milliseconds>(
        steady_clock::now() - t_start);
    std::cout << "parallel version: sum = " << sum << ", took "
#if __cplusplus >= 202002L
              << elapse
#else
              << elapse.count() << "ms"
#endif
              << std::endl;

    t_start = steady_clock::now();
    sum = std::accumulate(vi.begin(), vi.end(), 0);
    elapse = std::chrono::duration_cast<std::chrono::milliseconds>(
        steady_clock::now() - t_start);
    std::cout << "serial version: sum = " << sum << ", took "
#if __cplusplus >= 202002L
              << elapse
#else
              << elapse.count() << "ms"
#endif
              << std::endl;
}