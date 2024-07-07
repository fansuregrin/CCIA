# Sharing Data between Threads
## The Outline
- [Problems with sharing data between threads](#线程间共享数据所带来的问题)
    - [Race conditions](#竞态条件)
    - [Avoiding problematic race conditions](#避免竞态条件)
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

## 线程间共享数据所带来的问题
当线程之间的共享数据是只读的话，是没问题的；但是，当一个或多个线程修改共享数据时就会产生问题。一个被广泛用于帮助程序员推理代码的概念是不变量 (invariants)：关于特定数据结构始终正确的语句。修改线程之间共享的数据时最简单的潜在问题是破坏不变量 (broken of invariants)。破坏不变量的后果是：一个线程可能读到了部分修改的数据；或者另一个线程试图修改数据时导致数据结构坍塌或程序崩溃。无论结果如何，这都是并发代码中最常见的错误原因之一：竞争条件 (race condition)。

看一个具体例子：从一个双向链表中删除一个节点

```cpp
template <typename T>
struct ListNode {
    ListNode *next, *prev;
    T data;
};
```

![deleting a node from a doubly linked list](../imgs/fig-3.1-deleting_a_node_from_a_doubly_linked_list.png)

从一个双向链表中删除一个节点的步骤是：
- a. 定位到要删除的那个节点：N
- b. 更新 N 的前一个节点 (P) 的 next 指针，将其指向 N 的后一个节点 (Q)
- c. 更新 N 的后一个节点 (Q) 的 prev 指针，将其指向 N 的前一个节点 (P)
- d. 删除节点 N

其中，从步骤 b 到步骤 c 就会破坏不变量：在两个步骤之间，**P 的后一个节点是 Q，但 Q 的前一个节点却是 N**，这种不一致性就代表着不变量的破坏。如果此时另一个线程访问到了 P Q N 这三个节点，就会造成某些问题。比如，如果线程从前往后访问链表就会跳过节点 N，如果线程从后往前访问链表就不会跳过节点N；如果 Q 是链表的最后一个节点，另一个线程此时想删除它，这个线程会将 N 的 next 指针设为 `nullptr`，而 P 的 next 指针依然指向将要删除的节点 Q，这就会导致链表结构坍塌、程序出错。

### 竞态条件
竞态条件 (race conditon) 又叫做竞争条件，也被称为竞争冒险 (race hazard)。在并发中，竞争条件是指结果依赖于两个或多个线程操作执行的相对顺序；线程争先执行各自的操作。大多数情况下，这种竞争是良性的，因为产出的结果是可接受的，只是相对顺序有些不同。比如，向一个队列中添加元素，通常情况下，哪个元素先被添加哪个元素后被添加，我们并不关心，只要最终这些元素都被添加进去了就行了；因为不变量没有被破坏。而当竞争条件导致不变量被破坏时，就会导致不好的问题，例如删除双向链表中的某个节点。因此，在并发中，提到竞态条件，通常是指产生问题的竞态条件 (problematic race condition)。这种竞态条件经常给程序造成 bug。

有问题的竞争条件通常发生在完成操作需要修改两个或更多不同的数据（例如示例中的两个节点指针）的情况下。由于操作必须访问两个单独的数据，因此必须在单独的指令中修改这些数据，并且当只有一个数据完成时，另一个线程可能会访问数据结构（此时的数据结构是部分修改的）。

竞态条件通常很难发现并且更难复现，因为发生的机会窗口很短。随着系统负载的增加，以及操作执行次数的增加，出现有问题的执行序列的可能性也会增加。竞态条件通常是 timing-sensitive (时序敏感？) 的，在 debug 模式下可能会消失，因为 bebuger 改变了程序执行的 timing (时序？)。

总而言之，竞态条件是编写多线程程序时的一个**痛苦之源**，在并发中需要花费大量时间和精力来避免竞态条件。

### 避免竞态条件
#### 提供保护机制
为数据提供保护机制，当不变量被破坏时，只有修改线程能看到中间状态，而其他访问线程要么是还没有开始操作、要么是已经操作完成了。

#### 原子操作
另一种选择是修改数据结构及其不变量的设计，以便将修改作为**一系列不可分割的更改**来完成，每个更改都保持了不变量。(这通常涉及到 *无锁编程* 和 *内存模型* )。

#### 事务
将对数据的修改看成一项事务 (transaction)，所需的一系列数据修改和读取存储在事务日志中，然后通过单个步骤提交。如果由于数据结构已被另一个线程修改而无法继续提交，则事务将重新启动。