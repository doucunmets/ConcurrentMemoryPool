#pragma once
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <windows.h>
#include <thread>
#include <unordered_map>
#include <mutex>

using std::cout;
using std::endl;


const size_t MAXPAGES = 128; //PageCache 中最大的页数
const size_t PAGEBITES = 4 * 1024; //PageCache 一页的字节数
const size_t MAXBITES = 64 * 1024; //ThreadCache 一次性分配的最大的字节数
const size_t MAXBLOCKS = 240; //ThreadCache 和CentralCache 中有多少块
const size_t PAGE_SHIFT = 12; //1<<PAGE_SHIFT 表明一页的字节数


static inline void*& NextObj(void* obj) //取当前指针指向的对象的前4/8个字节
{
	return *((void**)obj);
}

void *CreatePage(size_t npage){ //创建内存 windows下的

    void* ptr = VirtualAlloc(0, npage * (1 << PAGE_SHIFT),MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if(ptr == nullptr){
		throw std::bad_alloc();
	}
	return ptr;
}

void DeletePage(void *ptr){ //删除内存
    VirtualFree(ptr, 0, MEM_RELEASE);
}


//记录页的内容
typedef size_t  PageID;
struct Span{

void *_objlist = nullptr;//分配的地址的头指针   因为分配内存返回的是void*头指针。
Span *_prev = nullptr;
Span *_next = nullptr;

size_t _pages = 0; //记录有几页
PageID _pageid = 0; //记录开始页号
size_t _objsize = 0; //记录centralCache中每一块的大小
size_t _usecount = 0;//记录被使用了多少块


};


class SpanList{ //维护span的链表

private:
Span *_head = nullptr;
std::mutex _mtx;

public:
SpanList(){ //初始化
	_head = new Span();
	_head->_next = _head;
	_head->_prev = _head;
}

void Lock(){
    _mtx.lock();
}

void Unlock(){
    _mtx.unlock();
}

Span *Begin(){
    return _head->_next;
}

Span *End(){
    return _head;
}

bool Empty(){ //判断链表是否为空

	if(_head->_next == _head) return true;
	return false;
}

Span *Front(){ // 取出一个内存
	Span *span = _head -> _next;
	_head -> _next = span -> _next;
	span -> _next -> _prev = _head;
	span->_next = span->_prev=nullptr;
	return span;
}

void Push(Span *span){ //放入一块内存

	span->_next = _head->_next;
	_head->_next->_prev = span;
	_head->_next = span;
	span->_prev = _head;
}

void PushBack(Span *span){
	Span *prev = _head->_prev;

	span ->_next = prev->_next;
	prev->_next->_prev = span;
	span ->_prev = prev;
	prev ->_next = span;
}

void Erase(Span *span){
    Span *prev = span -> _prev;
    Span *nex = span -> _next;
    prev -> _next = nex;
    nex -> _prev = prev;
    span->_next = span->_prev = nullptr;
}

};


class SizeClass{ //内存字节对齐，获取字节对应的下标等

public:

static size_t _RoundUp(size_t size,size_t align){
	return ((size + align -1) & (~(align - 1)));
}

static size_t RoundUp(size_t size){
	if(size <= 128) {
		return _RoundUp(size,8);
	}else if(size <= 1024) {
		return _RoundUp(size,16);
	}else if(size <= 8*1024){
		return _RoundUp(size,128);
	}else if(size <= 64*1024){
		return _RoundUp(size,1024);
	}
	return -1;
}

static  size_t _ListIndex(size_t size,size_t align_shift){
	return ((size + (1<<align_shift) - 1) >> align_shift);
}

static  size_t ListIndex(size_t size){
	if(size <= 128) {
		return _ListIndex(size,3);
	}else if(size <= 1024 ){
		return _ListIndex(size - 128,4) + 16;
	}else if(size <= 8 * 1024){
		return _ListIndex(size - 1024,7) + 56 + 16;
	}else if(size <= 64 * 1024){
		return _ListIndex(size - (8 * 1024),10) +16 + 56 + 56;
	}
	return -1;
}


static  size_t NumMoveSize(size_t size){
	size_t num = MAXBITES / size;
	if( num <2 ) num = 2;
	if( num > 512 ) num = 512;
	return num;
}

static  size_t NumMovePage(size_t size){
	size_t num = NumMoveSize(size);
	size_t pages = (num * size) >> PAGE_SHIFT;
	if(pages == 0) pages = 1;
	return pages;
}

};


class FreeList{ //管理void*链表

private:
void *_freelist = nullptr;
size_t _num = 0;
size_t _maxsize = 1;

public:

void SetMaxSize(size_t num){
	_maxsize = num;
}

size_t MaxSize(){
	return _maxsize;
}

void Push(void *obj){
	 NextObj(obj) = _freelist;
	_freelist = obj;
	++ _num;
}

void* Front(){
	void *obj = _freelist;
	_freelist = NextObj(_freelist);
	--_num;
	return obj;
}

void PushRange(void *start ,void *endd,size_t num){
	NextObj(endd) = _freelist;
	_freelist = start;
	_num += num;
}

size_t PopRange(void *&start,void *&endd,size_t num){
	start = _freelist;
	void *prev = _freelist;
	void *cur = NextObj(_freelist);
	size_t nums = 1;
	for(size_t i=0;i<num -1 && cur != nullptr;i++){
		prev = cur;
		cur = NextObj(cur);
		++ nums;
	}
	NextObj(prev) = nullptr;
	endd = prev;
	_freelist = cur;
	_num -= nums;
	return nums;
}

bool Empty(){
	if(_freelist == nullptr) return true;
	return false;
}

size_t Num(){
    return _num;
}

};

