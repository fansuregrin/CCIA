# Sharing Data between Threads
## The Outline
- [Problems with sharing data between threads](#线程间共享数据所带来的问题)
    - [Race conditions](#竞态条件)
    - [Avoiding problematic race conditions](#避免竞态条件)
- [Protecting shared data with mutexes](#使用互斥量保护共享数据)
    - [Using mutexes in C++](#在-c-中使用互斥量)
    - [Structuring code for protecting shared data](#组织代码来保护数据)
    - [Spotting race conditions inherent in interfaces](#发现接口中固有的竞争条件)
    - [Deadlock: the problem and a solution](#死锁问题和解决方案)
    - [Further guidelines for avoiding deadlock](#避免死锁的进一步指导)
    - [Flexible locking with `std::unique_lock`](#灵活的锁stdunique_lock)
    - [Transferring mutex ownership between scopes](#在作用域之间传递互斥量的所有权)
    - [Locking at an appropriate granularity](#以适当的粒度进行锁定)
- [Alternative facilities for protecting shared data](#保护共享数据的替代设施)
    - [Protecting shared data during initialization](#保护共享数据的初始化)
    - [Protecting rarely updated data structures](#保护很少更新的数据结构)
    - [Recursive locking](#递归锁)

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

## 使用互斥量保护共享数据
将访问数据结构的所有代码片段标记为互斥的 (mutually exclusive)，这样如果任何线程正在运行其中一个，则任何尝试访问该数据结构的其他线程都必须等到第一个线程完成。这样就能让其他线程（除了正在修改数据的那个线程之外的线程）看不到被破坏的不变量，从而避免竞态条件的产生。

提供这种数据保护机制之一的是同步原语：*互斥量 (mutex)* (源自 **mut**ual **ex**clusion)。当访问共享数据时，将与之相关的互斥量锁住，当结束访问后解锁。线程库会确保，当一个线程锁住了一个互斥量后，其他想要获取这个互斥量的锁的线程只能等待互斥量被解锁。

但是，互斥量不是万能的 (mutexes are not *silver bullet*)。使用互斥量时，需要组织代码来保护正确的数据、需要避免接口间的竞态条件、需要避免死锁、以及考虑锁的粒度。

### 在 C++ 中使用互斥量
在 C++ 中，可以通过类 `std::mutex` 来创建一个互斥量对象，调用其成员函数 `lock()` 来锁住互斥量，调用其成员函数 `unlock()` 来解锁互斥量。但是，一般不会直接这么用，因为编写代码时容易忘记解锁而导致一些问题。通常，会使用 `std::lock_guard` 来锁住互斥量：这个类为 mutex 实现了 RAII 机制，在构造函数中会对提供的互斥量进行加锁，然后当对象离开作用域时在析构函数中将互斥量解锁。下面是一个使用 `std::mutext` 和 `std::lock_guard` 的例子 (具体代码请见 [listing 3.1](../../src/ch03_sharing_data_between_threads/listing_3_1.cc))：

```cpp
#include <list>
#include <mutex>
#include <algorithm>

std::list<int> some_list;
std::mutex some_mtx;

void add_to_list(int new_value) {
    std::lock_guard<std::mutex> guard(some_mtx);
    some_list.push_back(new_value);
}

bool list_contains(int value_to_find) {
    std::lock_guard<std::mutex> guard(some_mtx);
    return std::find(some_list.begin(), some_list.end(), value_to_find) !=
           some_list.end();
}
```

上面的例子将需要保护的数据 (即一个 `std::list<int>`) 和互斥量都定义为全局变量，这样做没什么问题，但是不符合 OOP 的思想。通常，会将互斥量和需要保护的数据包装在一个类中 (作为私有成员)，将需要访问被保护数据的一些操作封装起来作为成员函数。如果该类的所有成员函数在访问任何其他数据成员之前锁住互斥量，并在完成后解锁，则可以很好地保护数据。

但是，这也不是万无一失的！如果其中一个成员函数返回指向受保护数据的指针或引用，那么所有成员函数以良好、有序的方式锁定互斥量就无关紧要了，因为这已经在保护罩上炸开了一个大洞。任何有权访问该指针或引用的代码现在都可以访问（并可能修改）受保护的数据，而无需锁定互斥量。因此，**使用互斥量来保护数据需要谨慎地设计良好的接口**！

### 组织代码来保护数据
使用互斥量来保护数据，并不是粗暴地将 `std::lock_guard` 扔进每一个成员函数那样简单；只要有一个 stray pointer/reference，一切保护都是白费。那么，是不是只要不把对被保护数据的指针或引用返回（传回）给调用者，就可以保证数据的安全性呢？答案依然是否定的！因为将对被保护数据的指针或引用传递给要调用的函数，也会让数据陷入危险。下面这个例子展示了这一点：

```cpp
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
```

造成上面这种问题出现的根本原因是：**没有保证所有访问共享数据的代码都是 mutually exclusive 的**。不幸的是，这种问题 C++ 标准库不能帮你解决，责任在于编写代码的程序员！因此，程序员应该遵循一条指南：*不要将指针和对受保护数据的引用传递到锁的范围之外，无论是从函数返回它们，将它们存储在外部可见的内存中，还是将它们作为参数传递给用户提供的函数！*

### 发现接口中固有的竞争条件
考虑数据结构 `stack`，除了构造函数和 `swap()`，还有这些接口：
- `push`：将一个新元素压入栈中
- `pop`：从栈中弹出一个元素
- `top`：获取栈顶元素
- `empty`：检查栈是否为空
- `size`：获取栈中保存的元素个数

```cpp
template <typename T, typename Container=std::deque<T> >
class stack {
public:
    explicit stack(const Container&);
    explicit stack(Container&& = Container());
    template <class Alloc> explicit stack(const Alloc&);
    template <class Alloc> stack(const Container&, const Alloc&);
    template <class Alloc> stack(Container&&, const Alloc&);
    template <class Alloc> stack(stack&&, const Alloc&);

    bool empty() const;
    size_t size() const;
    T& top();
    T const& top() const;
    void push(T const&);
    void push(T&&);
    void pop();
    void swap(stack&&);
};
```

`top` 接口会返回对栈顶元素的引用，为了保护数据，需要将改成返回栈顶元素的副本。但是，这仍然会有竞争条件出现的风险。这是接口本身的设计导致的固有的竞争条件。

#### 存在的问题
**(1) `top()` 和 `empty()` 之间的竞争**
```cpp
stack<int> s;
if (!s.empty()) { // 1
    int const value = s.top(); // 2
    s.pop(); // 3
    do_something(value);
}
```

| 线程 t1      | 线程 t2   |
|   :-:       |  :-:      |
| `s.empty()` |           |
|             | `s.pop()` |
| `s.top()`   |           |

在一个线程 t1 中，1 处的 `s.empty()` 返回 false；然后，在另一个线程 t2 中，由于调用 `s.pop()` 让栈空了；此时 t1 中调用 `s.top()` 就会出现问题。

**(2) `top()` 和 `pop()` 之间的竞争**
假设栈中至少有两个元素，且只有下面这两个线程，考虑如下执行序列：

| 线程 A      | 线程 B   |
|   :-:       |  :-:      |
| `if(!s.empty())` |                  |
|                  | `if(!s.empty())` |
| `int const value=s.top();` |        |
|        | `int const value=s.top();` |
| `s.pop();` |                        |
| `do_something(value);` | `s.pop();` |
|  | `do_something(value);`           |

线程 A 和线程 B 获取到的元素是同一个元素，本来应该是线程 A 和线程 B 各自处理不同的元素，但是现在两个线程处理的是用一个元素，而另一个需要被处理的元素漏掉了，而且永远从栈中消失了。

**(3) 将 `pop` 和 `top` 操作合并**
`pop()` 接口被设计成将栈顶元素从栈中弹出，并返回给调用者。问题在于将元素返回给调用者时，复制构造可能会抛出异常，这就导致调用方没有接受到元素，元素也从栈中消失了。

#### 解决方法
##### Option 1: 传入引用
传递一个变量的引用给 `pop`，在调用 `pop()` 时将弹出的值作为参数接收在该变量中。
```cpp
// iterface
void pop(T &ele);

stack s;
T e; // 创建一个对象来接受从栈中弹出的值
s.pop(e);
```
这种方法有一个不足：调用方需要在调用 `pop()` 之前创建一个栈中元素类型的实例。对于某些类型来说，这是不切实际的，因为构造实例在时间或资源方面非常昂贵。对于其他类型来说，这并不总是可行的，因为构造函数需要的参数在代码中此时不一定可用。最后，这种方法要求栈中所存储的类型是可赋值的，这是一个重要的限制：许多用户定义的类型不支持赋值。

##### Option 2: 要求不抛出异常的复制构造函数或移动构造函数
让栈只接受存储那些复制构造函数或移动构造函数不抛异常的类型。这样是安全的，但是局限性太大。

##### Option 3: 返回指向弹出元素的指针
返回指向弹出元素的指针，因为复制指针不会抛出异常，但是使用指针就意味着要管理分配给元素的内存，增加了额外的负担。可以使用智能指针 `shared_ptr` 来减轻管理内存的负担。

##### Option 5: 提供 Option 1 和 Option2/Option3

##### 一个线程安全栈的例子
```cpp
// Listing 3.4 An outline class definition for a thread-safe stack
#include <exception>
#include <memory>

struct empty_stack: std::exception {
    const char * what() const noexcept;
};

template<typename T>
class threadsafe_stack {
public:
    threadsafe_stack();
    threadsafe_stack(const threadsafe_stack&);
    threadsafe_stack &operator=(const threadsafe_stack &) = delete;
    void push(T new_value);
    std::shared_ptr<T> pop(); // option 3
    void pop(T& value); // option 1
    bool empty() const;
};
```

具体实现：请见[listing 3.5](../../src/ch03_sharing_data_between_threads/listing_3_5.cc)。

### 死锁：问题和解决方案
死锁描述的是两个或多个线程由于相互等待而永远被阻塞的情况。例如：一对线程中的每个线程都需要锁定一对互斥锁才能执行某些操作，并且每个线程都有一个互斥锁并正在等待另一个互斥锁；此时，两个线程都无法继续，因为它们都在等待对方释放其互斥锁。

通用的避免死锁的建议是：永远以相同的次序来锁住两个互斥量。但是，有些时候也会出问题。例如，当交换两个实例中的数据时，为了保证两个实例的数据正确交换，需要锁住分别保护这两个实例的互斥量。以一定的次序锁住这两个互斥量，当两个被交换的实例不同时，没什么问题发生，但是当交换的两个实例是同一个实例，则会造成死锁（因为同一个实例的互斥量不能被锁住两次，c++ 中的 `std::mutex` 是非递归的）。

**C++ 中提供函数 `std::lock`，它可以一次性锁住多个互斥量且没有死锁的风险。**

下面这个例子展示的是一个交换操作（具体代码请见[listing 3.6](../../src/ch03_sharing_data_between_threads/listing_3_6.cc)）：
```cpp
class some_big_object {};
void swap(some_big_object &lhs, some_big_object &rhs) {};

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
```

在交换函数中，首先会判断两个对象是不是同一个对象，如果不是同一个才交换。1 处会同时锁住 `lhs` 的互斥量和 `rhs` 的互斥量，如果 `std::lock` 已经锁住了一个互斥量，然后尝试锁住另一个互斥量时抛出了异常，已经被锁住的那个互斥量就会释放锁，然后将异常传出 `std::lock`。`std::lock` 提供了 *all-or-nothing* 的语义，要么锁住所有的互斥量，要么一个都不锁住。2 和 3 处初始化 `std::lock_guard` 对象时，多了一个 `std::adopt_lock` 参数，这个参数的意思是告诉 `std::lock_guard`，传入的互斥量已经上锁了，你不用在尝试上锁了，你只需要取得现有锁的所有权就行。

C++ 17 为这样的场景提供了额外的支持，我们可以使用 `std::scoped_lock` 来代替 `std::lock` 和 `std::lock_guard`。`std::scoped_lock` 是一个*可变参数类模板*，接受一系列的互斥量类型作为模板参数，以及一系列的互斥量作为构造函数的实参。在构造函数中，会用类似 `std::lock` 的方式对传入的这些互斥量进行锁定；在析构函数中，会将所有互斥量解锁。上面的 `swap` 可以改写成：
```cpp
void swap(X &lhs, X &rhs) {
    if (&lhs == &rhs) {
        return;
    }
    std::scoped_lock guard(lhs.mtx, rhs.mtx); // 1
    swap(lhs.some_detail, rhs.some_detail);
}
```

上面的 1 处并没有显式地给出类模板的参数，因为用到了 C++ 17 的新特性：**类模板参数自动推导**。其实，这一行等价于：`std::scoped_lock<std::mutex, std::mutex> guard(lhs.mtx, rhs.mtx)`。

虽然 `std::lock`（和 `std::scoped_lock<>`）可以在需要同时获取两个或更多锁的情况下帮助编码者避免死锁，但如果单独获取它们则无济于事。在这种情况下，则必须依靠开发人员的纪律(准则)来确保不会出现死锁。


### 避免死锁的进一步指导
死锁不一定只出现在有锁的地方，没有锁也会出现死锁。比如，两个线程互相等待对方结束时，也会发生死锁 ([demo_3_1](../../src/ch03_sharing_data_between_threads/demo_3_1.cc))：
```cpp
void func(std::thread &t) {
    t.join();  // 等待另一个线程结束
}

int main() {
    std::thread t1, t2;
    t1 = std::thread(func, std::ref(t2));
    t2 = std::thread(func, std::ref(t1));

    t1.join();
    t2.join();
}
```

避免死锁的指导原则可以归结为一个想法：如果有机会等待另一个线程，就不要等待它。下面这些单独的指导方针提供了识别和消除其他线程正在等待您的可能性的方法：

#### 避免嵌套锁
已经持有一个锁就不要再获取其他锁。如果需要获取多个锁，请使用 `std::lock` 来避免死锁。

#### 避免在持有锁时调用用户提供的代码
用户提供的代码会做什么事情是不确定的，它可能会获取锁，这就导致了嵌套锁，有可能会发生死锁。但是，有时候调用用户提供的代码是不可避免的，则需要新的指导方针。

#### 以固定的次序获取锁
如果必须获取多个锁，并且不能使用 `std::lock` 来一次性获取，最好的做法就是在每个线程中以固定的次序来获取锁。

考虑给每一个节点分配一个互斥量来保护双向链表的情形。当删除一个节点时，需要获取3个锁：当前节点的锁、前一个节点和后一个节点的锁。当遍历链表时，一个线程必须在持有当前节点的锁的同时，去获取下一个节点（往前遍历或往后遍历）的锁，当获取到下一个节点的锁后，才释放当前节点的锁。这样做是为了防止当前节点的指向下一个节点的指针（next 或 prev 指针）被其他线程修改。这种手牵手的加锁方式可以让多个线程访问链表，但为了防止死锁，加锁的顺序必须是固定的。如果一个线程 t1 从前往后遍历链表，另一个线程 t2 从后往前遍历链表。在某个时刻，两个线程遍历到链表的中间位置，有 2 个节点：`... <-> A <-> B <-> ...`。线程 t1 持有节点 A 的锁并尝试去获取节点 B 的锁，线程 t2 持有节点 B 的锁并尝试去获取节点 A 的锁，死锁就发生了。同样的，当某个线程想删除一个节点 B，已经获取了 B 的锁，尝试去获取前一个节点 A 的锁；此时，另一个遍历链表的线程获取了 A 的锁并尝试去获取 B 的锁，死锁也发生了。解决死锁的一个方法是规定只能从前往后获取节点的锁，这样做避免了死锁，但是牺牲了反向遍历链表的功能。

#### 使用层级锁
层级锁的理念是将应用程序划分为多个层，并确定可能在任何给定层中锁定的所有互斥锁。当代码尝试锁定互斥锁时，如果该互斥锁已持有来自较低层的锁，则不允许锁定该互斥锁。您可以在运行时检查这一点，方法是为每个互斥锁分配层号，并记录每个线程锁定了哪些互斥锁。

C++ 标准库不提供层级锁，可以自行实现。一个简单的层级锁的实现如下 (具体代码请见 [listing 3.8](../../src/ch03_sharing_data_between_threads/listing_3_8.h))：
```cpp
class hierarchical_mutex {
private:
    const unsigned long hierarchy_value;
    unsigned long previous_hierarchy_value;
    std::mutex internal_mutex;
    thread_local static unsigned long this_thread_hierarchy_value;

    void check_for_hierarchy_violation() {
        if (this_thread_hierarchy_value <= hierarchy_value) {
            throw std::logic_error("mutex hierarchy violated");
        }
    }

    void update_hierarchy_value() {
        previous_hierarchy_value = this_thread_hierarchy_value;
        this_thread_hierarchy_value = hierarchy_value;
    }

public:
    explicit hierarchical_mutex(unsigned long value)
        : hierarchy_value(value), previous_hierarchy_value(0) {}

    void lock() {
        check_for_hierarchy_violation();
        internal_mutex.lock();
        update_hierarchy_value();
    }

    bool try_lock() {
        check_for_hierarchy_violation();
        if (!internal_mutex.try_lock()) {
            return false;
        }
        update_hierarchy_value();
        return true;
    }

    void unlock() {
        if (this_thread_hierarchy_value != hierarchy_value) {
            throw std::logic_error("mutex hierarchy violated");
        }
        this_thread_hierarchy_value = previous_hierarchy_value;
        internal_mutex.unlock();
    }
};

thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value{
    std::numeric_limits<unsigned long>::max()};
```

上面的实现使用 静态的 `thread_local` 变量来存储当前线程的层级值。首先，这个变量是静态的 (specified with `static`)，它可以被所有的互斥量访问。另外，这个变量是线程局部的 (specified with `thread_local`)，它在每一个线程中都有一个副本，不同线程中的值可能不同。这样，每个线程可以独立地使用这个变量来记录当前线程到达的层级值。

使用自定义的层级锁（具体代码请见 [listing 3.7](../../src/ch03_sharing_data_between_threads/listing_3_7.cc)）：
```cpp
hierarchical_mutex high_level_mutex(10000);
hierarchical_mutex low_level_mutex(5000);
hierarchical_mutex other_mutex(6000);

int do_low_level_stuff() { return 1; }

int low_level_func() {
    std::lock_guard<hierarchical_mutex> lk(low_level_mutex);
    return do_low_level_stuff();
}

void do_high_level_stuff(int some_param) {}

void high_level_func() {
    std::lock_guard<hierarchical_mutex> lk(high_level_mutex);
    do_high_level_stuff(low_level_func());
}

void do_other_stuff() {}

void other_func() {
    std::lock_guard<hierarchical_mutex> lk(other_mutex);
    high_level_func();
    do_other_stuff();
}

std::thread t1([] { high_level_func(); }); // 不会抛出异常
std::thread t2([] { other_func(); });  // 会抛出异常
```

在上面的例子中，可以将自定义的类型 `hierarchical_mutex` 作为模板实参传递给类模板 `std::lock_guard`。因为 C++ 规定 `template<class Mutex> lock_guard;` 的模板参数 `Mutex` 只需要满足 `BasicLockable` 要求即可。这个要求规定，对于一个类型 `L` 的对象 `m`，需要满足以下几点：
- `m.lock()`：阻塞，直到可以为当前执行代理（线程、进程、任务）获取锁为止。如果引发异常，则不会获取锁。
- `m.unlock()`：在当前执行代理对 `m` 持有非共享锁的前提条件下，释放执行代理持有的非共享锁。不会引发任何异常。

### 灵活的锁：`std::unique_lock`
C++ 标准库还提供 `std::unique_lock`，它在使用上比 `std::lock_guard` 更加灵活 (通过放宽不变量)，而且它并不总是拥有一个相关的互斥量。

```cpp
template< class Mutex >
class unique_lock;
```

- `std::unique_lock` 有默认构造函数：`unique_lock() noexcept;`，使用默认构造函数创建的 unique_lock 对象并不拥有相关的互斥量。
- 可以给 `std::unique_lock` 的构造函数传递类型为 `std::adopt_lock_t` 的第二个参数（如 `std::adopt_lock`），让其接受一个已经获得锁的互斥量。对应构造函数：`unique_lock( mutex_type& m, std::adopt_lock_t t );`。
- 可以给构造函数传递类型为 `std::defer_lock_t` 的第二个参数（如 `std::defer_lock`），从而在构造函数中不上锁。对应构造函数：`unique_lock( mutex_type& m, std::defer_lock_t t ) noexcept;`。
- 可以给构造函数传递类型为 `std::chrono::duration` 的第二个参数，作为尝试获取锁的一段最长阻塞时间，超过这段时间后返回。对应构造函数：`template< class Rep, class Period >unique_lock( mutex_type& m, const std::chrono::duration<Rep, Period>&  timeout_duration );`。
- 可以给构造函数传递类型为 `std::chrono::time_point` 的第二个参数，阻塞直到到达指定的时间点或者获得锁。对应构造函数：`template< class Clock, class Duration > unique_lock( mutex_type& m, const std::chrono::time_point<Clock, Duration>& timeout_time );`。
- `std::unique_lock` 是 *可移动构造的 (MoveConstructible)* 和 *移动赋值的 (MoveAssignable)* 。
- `std::unique_lock` 是满足 *BasicLockable* 要求的。如果 `Mutex` 满足 *Lockable* 要求，那么 `unique_lock` 也满足 *BasicLockable* 要求（比如，可以把这样的 `unique_lock` 对象作为参数传递给 `std::lock`）；如果 `Mutex` 是满足 *TimedLockable* 要求，`unique_lock` 也满足 *TimedLockable* 要求。

根据以上几点，我们可以将 [listing 3.6](../../src/ch03_sharing_data_between_threads/listing_3_6.cc) 中的 `swap` 函数改用 `unique_lock` 来实现：
```cpp
void swap(X &lhs, X &rhs) {
    if (&lhs == &rhs) {
        return;
    }
    // 传入 `std::defer_lock` 让 mutex 处于解锁状态
    std::unique_lock<std::mutex> lock_a(lhs.mtx, std::defer_lock);
    std::unique_lock<std::mutex> lock_b(rhs.mtx, std::defer_lock);
    // 再使用 `std::lock` 一起上锁
    std::lock(lock_a, lock_b);
    swap(lhs.some_detail, rhs.some_detail);
}
```

`unqiue_lock` 定义了成员函数：`lock` 和 `unlock`，甚至是 `try_lock`、`try_lock_for`、`try_lock_until`。这些函数会调用底层的 mutex 的同名函数，并更新 `unique_lock` 实例中的一个标志，这个标志表明 mutex 是否被实例所拥有。正所谓“鱼与熊掌不可兼得”，由于这个标志  `unique_lock` 更灵活，但是存储标志需要额外的空间、更新和维护标志的状态需要额外的时间，所以 `unque_lock` 通常比 `lock_guard` 占用更多的空间且性能上有稍微的损失。

有的时候，我们需要就是灵活性，`unique_lock` 有一些情形下比 `lock_guard` 更合适：
- 当需要推迟上锁时
- 当需要将锁从一个作用域传递到另一个作用域时

### 在作用域之间传递互斥量的所有权
允许函数锁定互斥锁并将该锁的所有权转移给调用者，以便调用者可以在同一锁的保护下执行其他操作：
```cpp
std::unique_lock<std::mutex> get_lock() {
    extern std::mutex some_mutex;
    // lk 是自动变量，返回时会被 move 到返回值中
    std::unique_lock<std::mutex> lk(some_mutex);
    prepare_data();
    return lk;
}

void process_data() {
    // 锁的所有权从 get_lock 转移到 process_data 中
    std::unique_lock<std::mutex> lk(get_lock());
    do_something();
}
```

`unique_lock` 还可以主动释放锁。在 `std::unique_lock` 实例被销毁之前释放锁的能力意味着，如果显然不再需要该锁，可以选择在特定代码分支中释放它。这对于应用程序的性能非常重要；持有锁的时间超过所需时间可能会导致性能下降，因为等待锁的其他线程无法继续执行，白白浪费了 CPU 时间。

### 以适当的粒度进行锁定
多个线程正在等待同一资源，如果任何线程持有锁的时间超过必要时间，就会增加等待的总时间。在可能的情况下，仅在访问共享数据时锁定互斥锁；尽可能在锁之外进行任何数据处理。另外，在持有锁的时候，不要做一些耗时的活动，如文件 I/O 等。因为文件 I/O 需要访问磁盘，其速度远远低于在内存中操作数据，这样会影响其他线程的执行效率，从而抵消了多线程带来的性能提升。

这时候，`std::unique_lock` 就能派上用场了。当代码不再需要访问共享数据时，可以调用 `unlock()`，然后如果代码中稍后需要访问，则再次调用 `lock()`：
```cpp
void get_and_process_data() {
    std::unique_lock<std::mutex> lk(the_mutex);
    some_class data_to_process = get_next_data_chunk();
    lk.unlock();  // 处理数据时不需要访问共享数据，不需要上锁
    result_type result = process(data_to_process);
    lk.lock(); // 写入结果时重新上锁
    write_result(data_to_process, result);
}
```

*一般来说，锁应该只保持执行所需操作所需的最短时间*。

```cpp
class Y {
private:
    mutable std::mutex mtx;
    int some_detail;

    int get_detail() const {
        std::lock_guard<std::mutex> lk(mtx);
        return some_detail;
    }
public:
    explicit Y(int sd) : some_detail(sd) {}

    // 实现 1
    friend bool operator=(const Y &lhs, const Y &rhs) {
        if (&lhs == &rhs) { return true; }
        const int lhs_value = lhs.get_detail();
        const int rhs_value = rhs.get_detail();
        return lhs_value == rhs_value;
    }

    // 实现 2
    friend bool operator=(const Y &lhs, const Y &rhs) {
        std::unique_lock<std::mutex> lk_lhs(lhs.mtx, std::defer_lock);
        std::unique_lock<std::mutex> lk_rhs(rhs.mtx, std::defer_lock);
        std::lock(lk_lhs, lk_rhs);
        return lhs.some_detail == rhs.some_detail;
    }
};
```

实现 1 减小了锁的粒度，但微妙地改变了比较的语义。在某一个时刻获取了 `lhs` 的值，此时 `lhs` 的值还不等于 `rhs` 的值，但接着去获取 `rhs` 的值时，`rhs` 的值已经被改为与 `lhs` 相等了。在这个实现中，比较的是 某一时刻 `lhs` 的值 是否与 另一时刻 `rhs` 的值 相等。而在实现 2 中，对 `lhs` 和 `rhs` 同时上锁，比较的是 同一时刻 `lhs` 与 `rhs` 是否相等。

## 保护共享数据的替代设施
互斥量并不是保护共享数据的唯一法宝，在特定场景下，有更合适的替代。一个具有代表性的场景是，共享数据只需要在初始化的时候被保护。用互斥量来保护已经初始化的数据，单纯地是为了保护数据地初始化，其实是不必要的，而且影响性能。

### 保护共享数据的初始化
假设有一个共享资源，它的初始化开销比较大，所以只在有必要的情况下才初始化（惰性初始化, *Lazy initialization*）。单线程场景的代码如下：
```cpp
std::shared_ptr<some_resource> resource_ptr;

void foo() {
    // 资源还没被初始化，那就初始化
    if (!resource_ptr) {
        resource_ptr.reset(new some_resource); // 1
    }
    resource_ptr->do_something();
}
```

在多线程场景下，如果对共享资源本身的并发访问是安全的，那么唯一需要保护的就是 1 处的代码。多个线程可能会重复的对共享资源进行初始化。将上面的单线程代码改为多线程代码，比较单纯的实现如下：
```cpp
std::shared_ptr<some_resource> resource_ptr;
std::mutex resource_mutex;

void foo() {
    // 所有的线程都串行在这里（在这里排队等候，拿到锁的线程才去尝试初始化）
    std::unique_lock<std::mutex> lk(resource_mutex);
    if (!resource_ptr) {
        resource_ptr.reset(new some_resource); // 只有初始化资源才需要保护
    }
    lk.unlock();
    resource_ptr->do_something();
}
```

上面的代码存在不必要的串行化问题，当共享资源已经初始化后，线程还需要去获取锁，再去检查资源是否被初始化，而没拿到锁的其他线程都得等着。

为了避免这个问题，有个臭名昭著的解法 (双重检查锁定机制, *Double-checked locking pattern*)：
```cpp
std::shared_ptr<some_resource> resource_ptr;
std::mutex resource_mutex;

void foo() {
    if (!resource_ptr) {  // 1
        std::unique_lock<std::mutex> lk(resource_mutex);
        if (!resource_ptr) {  // 2
            resource_ptr.reset(new some_resource);  // 3
        }
    }
    resource_ptr->do_something();  // 4
}
```

但是，这种机制有不好的竞争条件。1 处读取 `resource_ptr` 的操作在锁的保护之外，它与其他线程对 `resource_ptr` 的写入是不同步的。即使一个线程看到了写入 `resource_ptr` 的值，它可能还没看到 `resource_ptr` 所指向的资源，接着在 4 处就会出现未定义的行为。

好在，C++ 标准为这个场景提供了专门的解决方案：`std::once_flag` 和 `std::call_once`。

```cpp
template< class Callable, class... Args >
void call_once( std::once_flag& flag, Callable&& f, Args&&... args );

class once_flag;
```

`call_once` 接受一个 `std::once_flag` 类型的标志 `flag`，一个可调用的对象 `f`，以及传递给 `f` 的参数。`call_once` 对 `f` 只会执行一次：
- 当调用 `call_once` 时 `flag` 标志着 `f` 已经被执行了，`call_once` 会直接返回。(这种情况下对 `call_once` 的调用称为 *passive*)。
- 当 `flag` 没有标志 `f` 已经被执行，`call_once` 会去调用 `f` (`INVOKE(std::forward<Callable>(f), std::forward<Args>(args)...)`)。（这种情况下对 `call_once` 的调用称为 *active*）。
    - 如果对 `f` 的调用抛出了异常，异常会传播给调用 `call_once` 的调用者，`flag` 也不会翻转。（这种情况下对 `call_once` 的调用称为 *exceptional*）。
    - 如果对 `f` 的调用正常，`flag` 被翻转，所有其他的带有相同 `flag` 的对 `call_once` 的调用都会是 *passive* 的。

同一标志上的所有活动调用 (active calls) 形成一个由零个或多个异常调用 (exceptional calls) 组成的单个全序，后跟一个返回调用 (returning calls)。每个活动调用的结束与该顺序中的下一个活动调用同步。

因此，上面的案例代码可以改写成：
```cpp
std::shared_ptr<some_resource> resource_ptr;
std::once_flag resource_flag;

void init_resource() {
    resource_ptr.reset(new some_resource);
}

void foo() {
    // 这里使用 `std::call_once` 的开销比使用 `std::mutex` 小
    std::call_once(resource_flag, init_resource);
    resource_ptr->do_something();
}
```

`std::call_once()` 也可轻松用于类成员的惰性初始化，示例代码请见 [listing 3.12](../../src/ch03_sharing_data_between_threads/listing_3_12.cc)。

初始化过程中可能存在竞争条件的一种情况是使用 `static` 声明的局部变量：
```cpp
class my_class;

my_class &get_instance() {
    static my_class instance;
    return instance;
}
```

在许多 C++11 之前的编译器中，这种竞争条件在实践中是有问题的，因为多个线程可能认为它们是第一个并尝试初始化变量，或者线程可能在另一个线程启动初始化之后但在完成之前尝试使用它。在 C++11 中，这个问题得到了解决：初始化被定义为只在一个线程上发生，并且在初始化完成之前没有其他线程会继续进行，因此竞争条件是关于哪个线程来进行初始化，而不是任何有问题的事情。总之，上面所示的初始化代码在 C++ 11 及以后是天然线程安全的。

### 保护很少更新的数据结构
有一种场景：大多数时候，该数据结构是只读的，因此可以被多个线程同时读取，但有时该数据结构可能需要更新。比如，考虑一个用于存储 DNS 条目缓存的表，该缓存用于将域名解析为其对应的 IP 地址。通常，给定的 DNS 条目将在很长一段时间内保持不变。尽管随着用户访问不同的网站，可能会不时将新条目添加到表中，但这些已经添加上去的条目在其整个生命周期内将基本保持不变。尽管更新很少见，但仍有可能发生，并且如果要从多个线程访问此缓存，则需要在更新期间对其进行适当保护，以确保读取缓存的任何线程都不会看到损坏的数据结构。

但是，使用 `std::mutex` 来保护数据结构过于悲观，因为它会消除在数据结构未被修改时读取数据结构的并发性（换句话说，使用互斥锁，多个线程不能同时去读数据）。所以，需要一种新的锁来适应这种场景。这种锁叫做读写锁（reader-writer lock/mutex）：它可以由单个“写者”线程进行独占访问，或者由多个“读者”线程进行的并发访问。

C++ 17 标准库提供两个这样的 mutex：`std::shared_mutex` 和 `std::shared_timed_mutex`，其中 `std::shared_timed_mutex` 从 C++ 14 就提供了。这两个 mutex 提供了两种级别的访问：
- 独占式访问（exclusive）：只有一个线程能拥有这个 mutex
- 共享式访问（shared）：多个线程能共享同一个 mutex 的所有权

C++ 14 标准库还新增了一个 RAII 类模板：`std::shared_lock`，它对共享式的 mutex 进行包装，提供共享式访问。`std::shared_lock` 的模板参数 `Mutex` 需要满足 *SharedLockable* 要求。

为了使类型 `L` 成为 *SharedLockable*，类型 `L` 的对象 `m` 必须满足以下条件：


| 表达式 | 前置条件 | 效果 | 返回值 |
| --- | --- | --- | --- |
| `m.lock_shared()` |  | 阻塞，直到可以为当前执行代理（线程、进程、任务）获取锁为止。如果引发异常，则不会获取锁。| |
| `m.try_lock_shared()` | 尝试以非阻塞方式获取当前执行代理（线程、进程、任务）的锁。如果引发异常，则不会获取锁。| | 如果获得了锁，则返回 `true`，否则返回 `false` |
| `m.unlock_shared()` | 当前执行代理对 `m` 持有共享锁。| 释放执行代理持有的共享锁。不会引发任何异常。 | |

下面看一个具体示例：
```cpp
class dns_entry;

class dns_cache {
private:
    std::map<std::string, dns_entry> entries;
    mutable std::shared_mutex entry_mutex;
public:
    dns_entry find_entry(const std::string &domain) const {
        // 此处共享式访问
        std::shared_lock<std::shared_mutex> lk(entry_mutex); // 1
        const std::map<std::string, dns_entry>::const_iterator it = entries
            .find(domain);
        return (it == entries.end() ? dns_entry() : it->second);
    }

    void update_or_add_entry(const std::string &domain,
    const dns_entry &dns_details) {
        // 此处独占式访问
        std::lock_guard<std::shared_mutex> lk(entry_mutex); // 2
        entries[domain] = dns_details;
    }
};
```

### 递归锁
使用 `std::mutex` 时，线程尝试锁定已拥有的 mutex 是错误的，尝试这样做会导致未定义的行为。但在某些情况下，线程需要多次获取同一个 mutex，而无需先释放它。为此，C++ 标准库提供了 `std::recursive_mutex`。它的工作原理与 `std::mutex` 类似，不同之处在于可以从同一个线程获取单个实例上的多个锁。

**大多数时候，如果您认为您想要一个递归互斥锁，那么您可能需要改变您的设计，而不是把递归锁请出来。** 例如，在一个类中，每个公共成员函数都会锁定互斥锁，执行工作，然后解锁互斥锁。一个公共成员函数会在其操作中调用另一个公共成员函数。在这种情况下，第二个成员函数也会尝试锁定互斥锁，从而导致未定义的行为。快速而粗略的解决方案是将互斥锁更改为递归互斥锁。但不建议使用这种用法，因为它会导致思维不清晰和设计不良。特别是，类不变量通常在持有锁时被破坏，这意味着第二个成员函数即使在不变量被破坏的情况下也需要工作。通常，最好是提取一个新的私有成员函数，该函数被两个成员函数调用，不会锁定互斥锁（它认为互斥锁已经被锁定）。