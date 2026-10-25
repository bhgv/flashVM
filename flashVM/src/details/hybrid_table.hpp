/* FlashVM: C++98 containers and implementation utilities: hybrid table.
 * File: src/details/hybrid_table.hpp.
 */

#ifndef FVM_DETAILS_HYBRID_TABLE_HPP
/* Define FVM DETAILS HYBRID TABLE HPP constant or compilation switch. */
#define FVM_DETAILS_HYBRID_TABLE_HPP
/* C++98, no STL. Dense nonnegative indices use the array part; sparse indices
 * and strings use separate hash tables. Queue order is independent of keys.
 * No implicit metatable dispatch: that belongs to the VM's value semantics. */
#include "vector.hpp"
#include "hash_table.hpp"

namespace fvm_detail
{ /* Define hybrid_table class and its representation. */

template <class V> class hybrid_table
{ /* Define slot struct and its representation. */


    struct slot
    {
        bool present; /* Whether this sparse slot contains a value. */
        V value;      /* Stored ActionScript value. */


        /* Initialize slot and its owned state. */
        slot() : present(false), value()
        {
        }
    };

    vector<slot> array_;           /* Dense indexed storage. */
    map<unsigned long, V> sparse_; /* Sparse indexed storage. */
    map<string, V> named_;         /* Storage for named keys. */
    vector<V> queue_;              /* Queued table values. */
    size_t count_;                 /* Number of active entries. */

    /* Avoid allocating enormous dense arrays for sparse numeric keys. */


    /* Compute and return dense candidate. */
    bool dense_candidate(unsigned long index) const
    {
        return index < 64UL || (index < 1048576UL && index <= array_.size() * 2UL + 16UL);
    }

  public:


    /* Initialize hybrid table and its owned state. */
    hybrid_table() : count_(0)
    {
    }


    /* Return the number of stored elements. */
    size_t size() const
    {
        return count_;
    }


    /* Compute and return array capacity. */
    size_t array_capacity() const
    {
        return array_.size();
    }


    /* Compute and return queue size. */
    size_t queue_size() const
    {
        return queue_.size();
    }


    /* Push queue. */
    void queue_push(const V &v)
    {
        queue_.push_back(v);
    }


    /* Pop queue. */
    bool queue_pop(V *out)
    {
        if (queue_.empty())
        {
            return false;
        }
        if (out)
        {
            *out = queue_.front();
        }
        queue_.erase(queue_.begin());
        return true;
    }


    /* Read the current value. */
    bool get(unsigned long i, V *out) const
    {
        if (i < array_.size() && array_[i].present)
        {
            if (out)
            {
                *out = array_[i].value;
            }
            return true;
        }
        typename map<unsigned long, V>::iterator it = sparse_.find(i);
        if (it == sparse_.end())
        {
            return false;
        }
        if (out)
        {
            *out = it->second;
        }
        return true;
    }


    /* Update the current value. */
    void set(unsigned long i, const V &v)
    {
        if (dense_candidate(i) && sparse_.find(i) == sparse_.end())
        {
            if (i >= array_.size())
            {
                array_.resize((size_t)i + 1);
            }
            if (!array_[i].present)
            {
                array_[i].present = true;
                ++count_;
            }
            array_[i].value = v;
        }
        else
        {
            if (sparse_.find(i) == sparse_.end())
            {
                ++count_;
            }
            sparse_[i] = v;
        }
    }


    /* Read the current value. */
    bool get(const string &key, V *out) const
    {
        typename map<string, V>::iterator it = named_.find(key);
        if (it == named_.end())
        {
            return false;
        }
        if (out)
        {
            *out = it->second;
        }
        return true;
    }


    /* Update the current value. */
    void set(const string &key, const V &v)
    {
        if (named_.find(key) == named_.end())
        {
            ++count_;
        }
        named_[key] = v;
    }
};
} // namespace fvm_detail
#endif
