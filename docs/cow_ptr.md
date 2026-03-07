# `Yc::locally_cow_ptr`

在文件 `"cow_ptr.h"` 定义。

```C++
template<
    class T,
    class Allocator = std::allocator<T>>
class locally_cow_ptr;
```

`locally_cow_ptr` 是实现写时拷贝（copy-on-write）语义的一种智能指针。它是用于单线程场景的，名字里面的 locally 暗示这一点。

每个 `locally_cow_ptr` 指向一个控制块，控制块内有引用计数和一个 `T` 类型的对象。如果一个 `locally_cow_ptr` 指向的控制块不被其他 `locally_cow_ptr` 指向，则称其独占该控制块或者对象，否则它和其他指针共享该控制块或者对象。只有独占对象的 `locally_cow_ptr` 可以修改其内容。而共享状态的 `locally_cow_ptr` 如果请求写入内容，则会先分配一个新的控制块并在其中创建一个原先 `T` 对象的副本，然后再由用户进行修改。`const` 的 `locally_cow_ptr` 不能修所管理对象。

复制构造或者复制赋值 `locally_cow_ptr` 对象可以创造共享状态的 `locally_cow_ptr` ，而独占的 `locally_cow_ptr` 析构或者被赋值以指向其他对象给时会销毁 `T` 对象并解分配控制块。`locally_cow_ptr` 没有空状态。

请注意，`locally_cow_ptr` 所管理的对象持有指向自己的 `locally_cow_ptr` 会因为引用计数无法归零引发内存泄露。另一方面，对 `locally_cow_ptr` 对象本身的操作以及内部对控制块的操作都不是线程安全的，所以请不要跨线程传递 `locally_cow_ptr` 。

## 模板参数

| | |
|-:|:-|
|`T`|要为之管理值的类型。该类型必须满足 [_可复制插入(CopyInsertable)_](https://zh.cppreference.com/w/cpp/named_req/CopyInsertable) 到 `X` 和 从 `X` [_可擦除(Erasable)_](https://zh.cppreference.com/w/cpp/named_req/Erasable)，其中 `X` 是虚设的一个具有元素类型 `T` 和分配器 `Allocator` 的 [_知分配器容器(AllocatorAwareContainer)_](https://zh.cppreference.com/w/cpp/named_req/AllocatorAwareContainer) （`locally_cow_ptr` 本身不是容器）|
|`Allocator`|用于管理控制块和 `T` 类型对象的 [_分配器(Allocator)_](https://zh.cppreference.com/w/cpp/named_req/Allocator)|

## 成员类型

|成员类型|定义|
|-:|:-|
|`element_type`|`T`|
|`allocator_type`|`Allocator`|

## 成员函数

| | |
|:-|:-|
|（构造函数）|构造 `locally_cow_ptr` <br>（公开成员函数）|
|（析构函数）|析构 `locally_cow_ptr` <br>（公开成员函数）|
|`operator=`|对指针复制赋值，使得它放弃现有对象转而和其他指针共享对象<br>(公开成员函数)|

### 修改器

| | |
|:-|:-|
|`swap`|交换所管理的对象<br>(公开成员函数)|

### 观察器

| | |
|:-|:-|
|`operator->`<br>`operator*`|访问所管理的对象，当前指针非 `const` 情况下可能会发生复制以实现COW语义<br>（公开成员函数）|
|`use_count`|返回当前对象管理的控制块有多少个持有者，永远不会返回 `0`<br>（公开成员函数）|
|`unique`|返回该指针是否独占当前对象<br>（公开成员函数）|
|`get_allocator`|返回使用的分配器<br>（公开成员函数）|
|`value`|返回到所含值的引用，如果当前指针非 `const` 那么可能会发生复制来实现COW的语义<br>（公开成员函数）|
|`const_value`|返回到所含值的不可变引用，不会出发复制（公开成员函数）|

## 非成员函数

| | |
|:-|:-|
|`make_locally_cow_ptr`|使用传入参数创建一个带有新的 `T` 类型对象的控制块，被返回的 `locally_cow_ptr` 管理，使用默认的分配器|
|`make_locally_cow_ptr_with_allocator`|使用传入参数创建一个带有新的 `T` 类型对象的控制块，被返回的 `locally_cow_ptr` 管理，使用指定的分配器|
|`swap`|交换所管理的对象<br>(公开成员函数)|
