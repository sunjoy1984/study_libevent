#ifndef _COMM_LRU_H
#define _COMM_LUR_H

#include <iostream>
#include <vector>
#include <ext/hash_map>
#include <pthread.h>
#include "base/mutex.h"


namespace __gnu_cxx {

static inline unsigned long binary_hash(const std::string& s) {
        unsigned long h = 0;
        const char* p = &s[0];
        int N = s.length();
        for (int i = 0; i < N; i++) {
            h = 31 * h + *p;
            p++;
        }
        return h;
}

template<> struct hash<const std::string> {
    size_t operator()(const std::string& s) const {
        unsigned long h = binary_hash(s);
        return size_t(h);
    }
};

template<> struct hash<std::string> {
    size_t operator()(const std::string& s) const {
        unsigned long h = binary_hash(s);
        return size_t(h);
    }
};

} //__gnu_cxx

namespace comm {

using namespace std;
using namespace __gnu_cxx;

template <class K, class T>
struct Node{
    K key;
    T data;
    Node *prev, *next;
};

template <class K, class T>
class LRUCache{
public:
    LRUCache(size_t size){
        entries_ = new Node<K,T>[size];
        for(int i=0; i<size; ++i) {
            free_entries_.push_back(entries_+i);
        }
        head_ = new Node<K,T>;
        tail_ = new Node<K,T>;
        head_->prev = NULL;
        head_->next = tail_;
        tail_->prev = head_;
        tail_->next = NULL;
    }
    ~LRUCache(){
        delete head_;
        delete tail_;
        delete[] entries_;
    }
    void Put(K key, T data){
        comm::SafeMutex mutex_guard(mutex_);
        Node<K,T> *node = hashmap_[key];
        if(node){ // node exists
            detach(node);
            node->data = data;
            attach(node);
        }
        else{
            if(free_entries_.empty()){
                node = tail_->prev;
                detach(node);
                hashmap_.erase(node->key);
            }
            else{
                node = free_entries_.back();
                free_entries_.pop_back();
            }
            node->key = key;
            node->data = data;
            hashmap_[key] = node;
            attach(node);
        }
    }
    bool Get(K key, T* value){
        comm::SafeMutex mutex_guard(mutex_);
        Node<K,T> *node = hashmap_[key];
        if(node){
            detach(node);
            attach(node);
            *value = node->data;
            return true;
        }
        else{
            *value = T();
            return false;
        }
    }
private:
    void detach(Node<K,T>* node){
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void attach(Node<K,T>* node){
        node->prev = head_;
        node->next = head_->next;
        head_->next = node;
        node->next->prev = node;
    }
private:
    hash_map<K, Node<K,T>* > hashmap_;
    vector<Node<K,T>* > free_entries_;
    Node<K,T> *head_, *tail_;
    Node<K,T> *entries_;
    comm::Mutex mutex_;
};

} //end of comm

#endif
