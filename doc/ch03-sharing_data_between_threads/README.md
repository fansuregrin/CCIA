# Sharing Data between Threads
## The Outline
- Problems with sharing data between threads
    - Race conditions
    - Avoiding problematic race conditions
- Protecting shared data with mutexes
    - Using mutexes in C++
    - Structuring code for protecting shared data
    - Spotting race conditions inherent in interfaces
    - Deadlock: the problem and a solution
    - Further guidelines for avoiding deadlock
    - Flexible locking with `std::unique_lock`
    - Transferring mutex ownership between scopes
    - Locking at an appropriate granularity
- Alternative facilities for protecting shared data
    - Protecting shared data during initialization
    - Protecting rarely updated data structures
    - Recursive locking

