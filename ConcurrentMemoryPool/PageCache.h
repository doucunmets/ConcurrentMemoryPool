#pragma once
#include "Common.h"
#include "Radix.h"

class PageCache{

private :

std::mutex _mtx;
SpanList _spanlist[MAXPAGES + 1];
static PageCache _Inst;
Radix<Span *> _page_id_span;
//std::unordered_map<PageID, Span*> _page_id_span;

public:

PageCache() = default;
PageCache(const PageCache&) = delete;
PageCache operator = (const PageCache&) = delete;

static PageCache *GetInstance(){
	return &_Inst;
}

Span *_NewSpan(size_t npage);

Span *NewSpan(size_t npage);

void RealeaseSpanToPageCache(Span *span);

Span *GetIdToSpan(size_t id);


};

PageCache PageCache::_Inst;

Span *PageCache::NewSpan(size_t npage){ //获取span 对象

	    _mtx.lock();
        Span *span = _NewSpan(npage);
        span->_objsize = span->_pages * PAGEBITES;
        _mtx.unlock();
        return span;


}

Span *PageCache::_NewSpan(size_t npage){

        if(!_spanlist[npage].Empty()){
            Span *span = _spanlist[npage].Front();
            return span;
        }
        for(size_t i = npage + 1; i <= MAXPAGES; i++){

            if(!_spanlist[i].Empty() ){
                Span *span = _spanlist[i].Front();
                Span *surplus  = new Span();

                surplus -> _pages = npage;
                surplus -> _pageid = span -> _pageid + span -> _pages - npage;

                span -> _pages -= npage;

                _spanlist[span -> _pages].Push(span);

                for(size_t i = 0;i < surplus -> _pages;i++){
                    _page_id_span.InsertRadix(surplus -> _pageid + i,surplus);
                    //_page_id_span[surplus -> _pageid + i] = surplus;
                }
                return surplus;
            }
        }
        Span *span = new Span;
        void *ptr = CreatePage(MAXPAGES);
        span->_pages = MAXPAGES;
        span -> _pageid = (PageID)ptr >> PAGE_SHIFT;

        _spanlist[span -> _pages].Push(span);

        for(size_t i = 0;i < span -> _pages;i++){
            _page_id_span.InsertRadix(span -> _pageid + i,span);
            //_page_id_span[span -> _pageid + i] = span;
        }
        return _NewSpan(npage);

}

Span *PageCache::GetIdToSpan(size_t id){ //查找页号对应的span对象

//        auto it = _page_id_span.find(id);
//        if(it == _page_id_span.end()) {
//            return nullptr;
//        }
//        return it->second;
        auto it = _page_id_span.QueryRadix(id);
        return it;
}

void PageCache::RealeaseSpanToPageCache(Span *span){ //合并
        _mtx.lock();
        while(1){
            size_t id = span -> _pageid - 1;
            Span *surplus = GetIdToSpan(id);
            if(surplus == nullptr) break;
            if(surplus -> _usecount != 0) break;
            if(span->_pages + surplus->_pages > MAXPAGES) break;

            if(surplus -> _usecount == 0){
                _spanlist[surplus -> _pages].Erase(surplus);

                surplus -> _pages += span -> _pages;
                for(size_t i = 0;i < span -> _pages;i++){
                    _page_id_span.InsertRadix(span -> _pageid + i,surplus);
                    //_page_id_span[span -> _pageid + i] =  surplus;
                }
                delete span;
                span = surplus;
            }
        }

        while(1){
            size_t id = span -> _pageid + span -> _pages;
            Span *surplus = GetIdToSpan(id);

            if(surplus == nullptr) break;
            if(surplus -> _usecount != 0) break;
            if(span->_pages + surplus->_pages > MAXPAGES) break;

            if(surplus -> _usecount == 0){
                _spanlist[surplus -> _pages].Erase(surplus);
                surplus -> _pages += span -> _pages;
                surplus -> _pageid = span -> _pageid;
                for(size_t i = 0;i < span -> _pages;i++){
                    _page_id_span.InsertRadix(span -> _pageid + i,surplus);
                    //_page_id_span[span -> _pageid + i] = surplus;
                }

                delete span;
                span = surplus;
            }
        }
        _spanlist[span -> _pages].Push(span);
        _mtx.unlock();
}

