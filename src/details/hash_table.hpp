#ifndef FVM_DETAILS_HASH_TABLE_HPP
#define FVM_DETAILS_HASH_TABLE_HPP
#include "string.hpp"
#include <cstddef>
#include <new>
namespace fvm_detail {
inline unsigned long hash_key(const fvm_detail::string &s) { unsigned long h=2166136261UL;for(size_t i=0;i<s.size();++i)h=(h^(unsigned char)s[i])*16777619UL;return h; }
inline unsigned long hash_key(unsigned long k) { k ^= k>>16; k*=2654435761UL;return k; }
/* Stable linked entries preserve deterministic enumeration order. A bucket array
   provides average O(1) lookup; resizing never invalidates entry addresses. */
template<class K,class V> class map {
public:
    struct entry { const K first; V second; entry *next; entry *bucket_next; entry(const K &k):first(k),second(),next(0),bucket_next(0) {} };
    class iterator {
        entry *p_;
    public:
        iterator(entry *p=0):p_(p){}
        entry &operator*() const {return *p_;}
        entry *operator->() const {return p_;}
        iterator &operator++(){if(p_)p_=p_->next;return *this;}
        bool operator==(const iterator &o) const{return p_==o.p_;}
        bool operator!=(const iterator &o) const{return p_!=o.p_;}
    };
    typedef iterator const_iterator;
private:
    entry **buckets_;size_t capacity_,count_;entry *head_,*tail_;
    void rehash(size_t n) { entry **b=new entry*[n];for(size_t i=0;i<n;++i)b[i]=0;for(entry *e=head_;e;e=e->next){size_t k=hash_key(e->first)%n;e->bucket_next=b[k];b[k]=e;}delete [] buckets_;buckets_=b;capacity_=n; }
public:
    map():buckets_(0),capacity_(0),count_(0),head_(0),tail_(0){}
    map(const map &m):buckets_(0),capacity_(0),count_(0),head_(0),tail_(0){for(entry *e=m.head_;e;e=e->next)(*this)[e->first]=e->second;}
    map &operator=(const map &m){if(this!=&m){map tmp(m);swap(tmp);}return *this;}
    ~map(){clear();delete [] buckets_;}
    void swap(map &m){entry **b=buckets_;buckets_=m.buckets_;m.buckets_=b;size_t n=capacity_;capacity_=m.capacity_;m.capacity_=n;n=count_;count_=m.count_;m.count_=n;entry *p=head_;head_=m.head_;m.head_=p;p=tail_;tail_=m.tail_;m.tail_=p;}
    void clear(){entry *e=head_;while(e){entry *n=e->next;delete e;e=n;}head_=tail_=0;count_=0;for(size_t i=0;i<capacity_;++i)buckets_[i]=0;}
    size_t size() const{return count_;}
    bool empty() const{return count_==0;}
    iterator begin() const{return iterator(head_);}
    iterator end() const{return iterator(0);}
    iterator find(const K &key) const{if(!capacity_)return end();for(entry *e=buckets_[hash_key(key)%capacity_];e;e=e->bucket_next)if(e->first==key)return iterator(e);return end();}
    V &operator[](const K &key){iterator it=find(key);if(it!=end())return it->second;if(!capacity_ || (count_+1)*4>=capacity_*3)rehash(capacity_ ? capacity_*2 : 16);entry *e=new entry(key);size_t k=hash_key(key)%capacity_;e->bucket_next=buckets_[k];buckets_[k]=e;if(tail_)tail_->next=e;else head_=e;tail_=e;++count_;return e->second;}
};
}
#endif
