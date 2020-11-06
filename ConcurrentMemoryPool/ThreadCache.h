#pragma once
#include "Common.h"
#include "CentralCache.h"


class ThreadCache{

private:
FreeList _freelist[MAXBLOCKS + 1];

public:

void *Allocate(size_t sizes);

void *FetchFromCentralCache(size_t index,size_t sizes);

void Deallocte(void *ptr,size_t sizes);
void ListTooLong(FreeList &freelists,size_t num,size_t sizes);

};
static _declspec(thread) ThreadCache* tls_threadcache = nullptr;

void *ThreadCache::FetchFromCentralCache(size_t index,size_t sizes){

    FreeList &freelists = _freelist[index];

    size_t num_to_move = std::min(SizeClass::NumMoveSize(sizes),freelists.MaxSize());

    void *start ,*endd;
    size_t fetchnum = CentralCache::GetCentralInstance()->FetchRangeObj(start,endd,num_to_move,sizes);


    if(fetchnum > 1){
        freelists.PushRange(NextObj(start),endd,fetchnum - 1);
    }
    if(num_to_move == fetchnum){
        freelists.SetMaxSize(num_to_move + 1);
    }
    return start;

}

void *ThreadCache::Allocate(size_t sizes){

        size_t index = SizeClass::ListIndex(sizes);

        if(!_freelist[index].Empty()){
            return _freelist[index].Front();
        }

        return FetchFromCentralCache(index,SizeClass::RoundUp(sizes));

}


void ThreadCache::Deallocte(void *ptr , size_t sizes){

        size_t index = SizeClass::ListIndex(sizes);

        FreeList &freelists = _freelist[index];

        freelists.Push(ptr);

        size_t num = SizeClass::NumMoveSize(sizes);

        if(freelists.Num() >= num){
            ListTooLong(freelists, num, sizes);
        }

}

void ThreadCache::ListTooLong(FreeList &freelists,size_t num,size_t sizes){

    void *start = nullptr ,*endd = nullptr;
    freelists.PopRange(start,endd,num);
    NextObj(endd) = nullptr;
    CentralCache::GetCentralInstance()->ReleaseListToSpans(start,sizes);

}

