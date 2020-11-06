# ConcurrentMemoryPool

语言：C++

系统：windows

开发工具：codeblocks


描述：
此内存池实现了内存的复用。适用于多线程的情况，优化了内存碎片。释放和删除整体时间大部分比malloc/free快。

模块：
Common.h:字节对齐、对象的操作等常用的类和函数。

PageCache.h：以页为单位（一页4K），从操作系统获取内存，不同页对象放在不同链表上，根据页号和标记合并对象。

CentralCache.h：从PageCache获取对象，拆分对象中的页为内存块，不同大小的块的对象放在不同链表上，根据页号减少对象内的标记。

ThreadCache.h：从CentralCache获取内存块，直接分配，不同大小内存块放在不同的链表上，放进ThreadCache维护的内存链表，根据链表大小决定是否删除。

ConcurrentMalloc.h：分配从ThreadCache获取的内存给用户，直接传递删除的内存块指针。

Radix.h：基数树，加快查找对象的速度。
