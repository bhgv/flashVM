#ifndef FVM_DETAILS_VECTOR_HPP
#define FVM_DETAILS_VECTOR_HPP
#include <cstddef>
#include <new>
namespace fvm_detail {
template<class T> class vector {
    T *items_; size_t count_, capacity_;
public:
    typedef T* iterator;
    typedef const T* const_iterator;
    vector():items_(0),count_(0),capacity_(0) {}
    explicit vector(size_t n):items_(0),count_(0),capacity_(0) { try {resize(n);}catch(...){delete [] items_;throw;} }
    vector(const vector &v):items_(0),count_(0),capacity_(0) { assign(v.begin(),v.end()); }
    ~vector() { delete [] items_; }
    vector &operator=(const vector &v) { if(this!=&v) { vector tmp(v); swap(tmp); } return *this; }
    void swap(vector &v) { T *p=items_;items_=v.items_;v.items_=p;size_t n=count_;count_=v.count_;v.count_=n;n=capacity_;capacity_=v.capacity_;v.capacity_=n; }
    size_t size() const { return count_; }
    bool empty() const { return count_==0; }
    iterator begin() { return items_; }
    const_iterator begin() const { return items_; }
    iterator end() { return items_ ? items_+count_ : 0; }
    const_iterator end() const { return items_ ? items_+count_ : 0; }
    T &operator[](size_t i) { return items_[i]; }
    const T &operator[](size_t i) const { return items_[i]; }
    T &front() { return items_[0]; }
    const T &front() const { return items_[0]; }
    T &back() { return items_[count_-1]; }
    const T &back() const { return items_[count_-1]; }
    void reserve(size_t n) { if(n<=capacity_)return; T *p=new T[n]; try { for(size_t i=0;i<count_;++i)p[i]=items_[i]; } catch(...) {delete [] p;throw;} delete [] items_;items_=p;capacity_=n; }
    void resize(size_t n) { if(n>capacity_)reserve(n); if(n<count_)for(size_t i=n;i<count_;++i)items_[i]=T(); else for(size_t i=count_;i<n;++i)items_[i]=T();count_=n; }
    void push_back(const T &v) { T copy=v; if(count_==capacity_) { if(capacity_>(size_t)-1/2)throw std::bad_alloc();reserve(capacity_ ? capacity_*2 : 8); } items_[count_]=copy;++count_; }
    void pop_back() { if(count_) { --count_;items_[count_]=T(); } }
    void clear() { resize(0); }
    iterator erase(iterator it) { size_t i=(size_t)(it-items_); if(i>=count_)return end();for(size_t j=i+1;j<count_;++j)items_[j-1]=items_[j];pop_back();return items_+i; }
    template<class It> void assign(It first,It last) { vector tmp;for(;first!=last;++first)tmp.push_back(*first);swap(tmp); }
};
}
#endif
