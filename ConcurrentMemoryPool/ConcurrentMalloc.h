#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

void* ConcurrentAlloc(size_t size){ //分配内存的策略

        void *ptr = nullptr;

        size_t roundsize = SizeClass::_RoundUp(size,1<<PAGE_SHIFT);
        size_t npage = roundsize >> PAGE_SHIFT;

        if(size > MAXPAGES * PAGEBITES){

            ptr = CreatePage(npage);

        }else if(size > MAXBITES) {

            Span *span = PageCache::GetInstance()->NewSpan(npage);
            ptr = (void *)(span -> _pageid << PAGE_SHIFT);
            span ->_usecount = 1;
        }else {

            if(tls_threadcache == nullptr) {
                tls_threadcache = new ThreadCache();
            }
            ptr = tls_threadcache->Allocate(size);
        }
        return ptr;



}


void ConcurrentFree(void *ptr){ //删除内存的策略
        size_t size = (PageID)ptr >> PAGE_SHIFT;
        Span *span = PageCache::GetInstance()->GetIdToSpan(size);

        if(span == nullptr) {
            DeletePage(ptr);
            return ;
        }

        size = span->_objsize;

        if(size > MAXBITES) {
            span->_usecount = 0;
            PageCache::GetInstance()->RealeaseSpanToPageCache(span);
        }else {
            tls_threadcache->Deallocte(ptr, size);
        }

}
