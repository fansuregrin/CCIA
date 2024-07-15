// Listing 4.12 A sequential implementation of Quicksort
#include <algorithm>
#include <list>
#include <utility>
#include <random>
#include <iostream>

template <typename T>
std::list<T> sequential_quicksort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin()); // 1
    const T &pivot = *result.begin();            // 2
    auto divide_point = std::partition(input.begin(), input.end(),
                                       [&](const T &t) { return t < pivot; }); // 3
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point); // 4
    auto new_lower(sequential_quicksort(std::move(lower_part))); // 5
    auto new_higher(sequential_quicksort(std::move(input))); // 6
    result.splice(result.end(), new_higher); // 7
    result.splice(result.begin(), new_lower); // 8
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
    std::cout << "after  sort: " << sequential_quicksort(x) << std::endl;
}