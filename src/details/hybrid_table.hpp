#ifndef FVM_DETAILS_HYBRID_TABLE_HPP
#define FVM_DETAILS_HYBRID_TABLE_HPP
/* C++98, no STL. Dense nonnegative indices use the array part; sparse indices
 * and strings use separate hash tables. Queue order is independent of keys.
 * No implicit metatable dispatch: that belongs to the VM's value semantics. */
#include "vector.hpp"
#include "hash_table.hpp"
namespace fvm_detail {
template<class V> class hybrid_table {
    struct slot { bool present; V value; slot():present(false),value(){} };
    vector<slot> array_;
    map<unsigned long,V> sparse_;
    map<string,V> named_;
    vector<V> queue_;
    size_t count_;
    /* Avoid allocating enormous dense arrays for sparse numeric keys. */
    bool dense_candidate(unsigned long index) const {
        return index < 64UL || (index < 1048576UL && index <= array_.size()*2UL+16UL);
    }
public:
    hybrid_table():count_(0){}
    size_t size() const {return count_;}
    size_t array_capacity() const {return array_.size();}
    size_t queue_size() const {return queue_.size();}
    void queue_push(const V &v) {queue_.push_back(v);}
    bool queue_pop(V *out) {if(queue_.empty())return false;if(out)*out=queue_.front();queue_.erase(queue_.begin());return true;}
    bool get(unsigned long i,V *out) const {
        if(i<array_.size() && array_[i].present){if(out)*out=array_[i].value;return true;}
        typename map<unsigned long,V>::iterator it=sparse_.find(i);
        if(it==sparse_.end())return false;if(out)*out=it->second;return true;
    }
    void set(unsigned long i,const V &v) {
        if(dense_candidate(i) && sparse_.find(i)==sparse_.end()) {
            if(i>=array_.size())array_.resize((size_t)i+1);
            if(!array_[i].present){array_[i].present=true;++count_;}
            array_[i].value=v;
        } else {
            if(sparse_.find(i)==sparse_.end())++count_;
            sparse_[i]=v;
        }
    }
    bool get(const string &key,V *out) const {
        typename map<string,V>::iterator it=named_.find(key);
        if(it==named_.end())return false;if(out)*out=it->second;return true;
    }
    void set(const string &key,const V &v) {
        if(named_.find(key)==named_.end())++count_;
        named_[key]=v;
    }
};
}
#endif
