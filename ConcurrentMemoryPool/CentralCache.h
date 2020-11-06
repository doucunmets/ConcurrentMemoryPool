#pragma once
#include "Common.h"
#include "PageCache.h"

class CentralCache{

private:

static CentralCache _inst;
SpanList _spanlists[MAXBLOCKS + 1];

public:

CentralCache() = default; //Ĭ�Ϲ��캯��
CentralCache(CentralCache &) = delete; //��ֹ����
CentralCache operator = (CentralCache &) = delete; //��ֹ��ֵ

static CentralCache* GetCentralInstance(){
	return &_inst;
}

Span *GetOneSpan(size_t size);

size_t FetchRangeObj(void *&start,void *&endd,size_t num,size_t size);

void ReleaseListToSpans(void *start,size_t size);

};
CentralCache CentralCache::_inst;

//��PagCache�л�ȡspan������ֿ�
Span *CentralCache::GetOneSpan(size_t size){

        size_t index = SizeClass::ListIndex(size);
        SpanList &spanlists = _spanlists[index];
        Span *first = spanlists.Begin();
        Span *second = spanlists.End();
        while(first != second){
            if(first->_objlist != nullptr){
                spanlists.Erase(first);
                return first;
            }
            first = first->_next;
        }
        size_t numpage = SizeClass::NumMovePage(size);

        Span *span = PageCache::GetInstance()->NewSpan(numpage);
        char *start = (char *)(span -> _pageid << PAGE_SHIFT);
        char *endd = start + ( span -> _pages  << PAGE_SHIFT);
        size_t num = 0;

        char *obj = start;
        while(obj + size <endd){

            NextObj(obj) = obj + size;
            obj += size;
            ++ num;
        }

        NextObj(obj) = nullptr;
        span -> _objlist = start;
        span -> _objsize = size;
        span -> _usecount = 0;

        return span;

}


//�����PageCache�л�ȡ��span�����ҷ����ThreadCache

size_t CentralCache::FetchRangeObj(void *&start,void *&endd,size_t num,size_t size){


        size_t indexs = SizeClass::ListIndex(size);
        SpanList& spanlist = _spanlists[indexs];
        spanlist.Lock();

        Span *span = GetOneSpan(size);
        start = span -> _objlist;

        void *prev = start;

        void *cur = NextObj(start);

        size_t fetchnum = 1;

        while(cur != nullptr &&fetchnum < num ){
            prev = cur;
            cur = NextObj(cur);
            fetchnum ++;
        }

        NextObj(prev) = nullptr;
        endd = prev;

        span -> _objlist = cur;
        span -> _usecount += fetchnum;

        size_t index = SizeClass::ListIndex(size);

        if(span -> _objlist == nullptr){
            _spanlists[index].PushBack(span);
        }else _spanlists[index].Push(span);
        spanlist.Unlock();
        return fetchnum;

}

//�ϲ���ThreadCache�л�ȡ���ڴ�

void CentralCache::ReleaseListToSpans(void *start,size_t size){

        size_t index = SizeClass::ListIndex(size);
        SpanList& spanlist = _spanlists[index];
        spanlist.Lock();
        while(start){

            void *next = NextObj(start);
            PageID id = (PageID) start >> PAGE_SHIFT;
            Span *span = PageCache::GetInstance()->GetIdToSpan(id);
            NextObj(start) = span -> _objlist;
            span -> _objlist = start;
            span -> _usecount --;
            if(span -> _usecount == 0){
                size_t index = SizeClass::ListIndex(size);
                _spanlists[index].Erase(span);
                span -> _objlist = nullptr;
                span -> _objsize = 0;
                PageCache::GetInstance()->RealeaseSpanToPageCache(span);
            }
            start = next;
        }
        spanlist.Unlock();
}



