#pragma once
#include <iostream>
#include <assert.h>
#include <memory>
#include <queue>

const int child = 1<<4; //ÿ���ڵ��ж��ٸ����ӽڵ�
const int multiple = 4; //ÿ��Ψһλ�ƶ���λ

/***********
����ǻ���������Ϊɾ����ʱ����Ҫ�ϲ������Կ��ٵĻ�ȡҳ�Ŷ�Ӧ��
Span����ʹ�û�������unordered_map�졣

************/

template<typename T>
struct RadixNode{
T val;
bool _flag;
RadixNode<T> *_next[child];

};

template<typename T>
class Radix{

private:
RadixNode<T> *root;

public:


Radix():root(new RadixNode<T>()) {
    for(int i=0;i<child;i++) root -> _next[i] = nullptr;
	root -> _flag = false;
}

RadixNode<T>  *CreateRadixNode(){
	RadixNode<T>  *cur (new RadixNode<T>());
	for(int i=0;i<child;i++) cur -> _next[i] = nullptr;
	cur -> _flag = false;
	return cur;
}


void InsertRadix(size_t  key, T val ){
	RadixNode<T>  *cur = root;
	while(key){
        if(cur -> _next[key &(child - 1)] == nullptr)
            cur -> _next[key &(child - 1)]  = CreateRadixNode();
        cur = cur -> _next[key &(child - 1)] ;
        key = key >> multiple;
	}
	cur -> val = val;
}


T QueryRadix(size_t key ){
	RadixNode<T>  *cur = root;
	while(key){
        cur = cur -> _next[key &(child - 1)];
        if(cur == nullptr) return nullptr;
        key = key >> multiple;
	}
	return cur -> val;
}

~Radix(){
    std::queue<RadixNode<T> *> vx;
    vx.push(root);
    while(!vx.empty()){
        RadixNode<T> *u = vx.front(); vx.pop();
        for(int i=0;i<child;i++){
            if(u->_next[i]!=nullptr) {
                vx.push(u->_next[i]);
            }
        }
        delete u;
    }
}

};


