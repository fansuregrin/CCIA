// Listing 4.13 Parallel Quicksort using futures
#include <algorithm>
#include <list>
#include <utility>
#include <random>
#include <future>
#include <iostream>

template <typename T>
std::list<T> parallel_quicksort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    const T &pivot = *result.begin();
    auto divide_point = std::partition(input.begin(), input.end(),
                                       [&](const T &t) { return t < pivot; });
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
    std::future<std::list<T>> new_lower(
        std::async(&parallel_quicksort<T>, std::move(lower_part))
    ); // 1
    auto new_higher(parallel_quicksort(std::move(input))); // 2
    result.splice(result.end(), new_higher); // 3
    result.splice(result.begin(), new_lower.get()); // 4
    return result;
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::list<T> &lst) {
    const auto begin = lst.begin();
    const auto end = lst.end();
    for (auto iter = begin; iter != end; ++iter) {
        if (iter == begin) {
            os << *iter;
        } else {
            os << ' ' << *iter;
        }
    }
    return os;
}

int main() {
    std::list<int> x;
    for (int i=0; i<10; ++i) {
        x.push_back(std::rand() % 100);
    }

    std::cout << "before sort: " << x << std::endl;
    std::cout << "after  sort: " << parallel_quicksort(x) << std::endl;
}