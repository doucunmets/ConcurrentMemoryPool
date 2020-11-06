# ConcurrentMemoryPool

语言：C++

系统：windows

开发工具：codeblocks


描述：
此内存池实现了内存的复用。适用于多线程的情况，优化了内存碎片。释放和删除整体时间大部分比malloc/free快。

模块：
Common.h:字节对齐、对象的操作等常用的类和函数。

PageCache.h：以页为单位（一页4K），从操作系统获取内存，不同页数的对象放在不同链表上。

CentralCache.h：从PageCache获取对象，拆分对象中的连续页为相同大小内存块，不同大小的块的对象放在不同链表上。

ThreadCache.h：从CentralCache获取内存块，直接分配，不同大小内存块放在不同的链表上。

ConcurrentMalloc.h：分配从ThreadCache获取的内存给用户。

Radix.h：基数树，加快查找对象的速度。
