/* FlashVM: C++98 containers and implementation utilities: hash table.
 * File: src/details/hash_table.hpp.
 */

#ifndef FVM_DETAILS_HASH_TABLE_HPP
/* Define FVM DETAILS HASH TABLE HPP constant or compilation switch. */
#define FVM_DETAILS_HASH_TABLE_HPP
#include "string.hpp"
#include <cstddef>
#include <new>
namespace fvm_detail
{
/* Compute and return hash key. */
inline unsigned long hash_key(const fvm_detail::string &s)
{
    unsigned long h = 2166136261UL;
    for (size_t i = 0; i < s.size(); ++i)
        h = (h ^ (unsigned char)s[i]) * 16777619UL;
    return h;
}

/* Compute and return hash key. */
inline unsigned long hash_key(unsigned long k)
{

    /* Computation / Вычисление и обновление состояния. */
    k ^= k >> 16;
    k *= 2654435761UL;
    return k;
}
/* Stable linked entries preserve deterministic enumeration order. A bucket array
   provides average O(1) lookup; resizing never invalidates entry addresses. */
/* Define map class and its representation. */
template <class K, class V> class map
{
  public:
    /* Define entry struct and its representation. */
    struct entry
    {
        const K first;      /* Key associated with this map entry. */
        V second;           /* Value associated with this map entry. */
        entry *next;        /* Link to the next entry. */
        entry *bucket_next; /* Next entry in the same hash bucket. */
        /* Initialize entry and its owned state. */
        entry(const K &k) : first(k), second(), next(0), bucket_next(0)
        {
        }
    };
    /* Define iterator class and its representation. */
    class iterator
    {
        entry *p_; /* Entry referenced by this iterator. */

      public:
        /* Initialize iterator and its owned state. */
        iterator(entry *p = 0) : p_(p)
        {
        }
        /* Compute and return &operator*() const. */
        entry &operator*() const
        {
            return *p_;
        }
        /* Apply operator-> to the supplied operands. */
        entry *operator->() const
        {
            return p_;
        }
        /* Compute and return &operator++(). */
        iterator &operator++()
        {
            if (p_)
                p_ = p_->next;
            return *this;
        }
        /* Compare both operands for equality. */
        bool operator==(const iterator &o) const
        {
            return p_ == o.p_;
        }
        /* Compare both operands for inequality. */
        bool operator!=(const iterator &o) const
        {
            return p_ != o.p_;
        }
    };
    typedef iterator const_iterator;

  private:
    entry **buckets_; /* Hash bucket storage owned by the table. */
    size_t capacity_ /* Allocated storage limit; zero may denote an unbounded queue. */,
        count_;                                              /* Number of active entries. */
    entry *head_ /* First entry in the sequence. */, *tail_; /* Last entry in the sequence. */
    /* Rebuild hash buckets for the current value. */
    void rehash(size_t n)
    {
        entry **b = new entry *[n];
        for (size_t i = 0; i < n; ++i)
            b[i] = 0;
        for (entry *e = head_; e; e = e->next)
        {
            size_t k = hash_key(e->first) % n;
            e->bucket_next = b[k];
            b[k] = e;
        }
        delete[] buckets_;

        /* Computation / Вычисление и обновление состояния. */
        buckets_ = b;
        capacity_ = n;
    }

  public:
    /* Initialize map and its owned state. */
    map() : buckets_(0), capacity_(0), count_(0), head_(0), tail_(0)
    {
    }
    /* Initialize map and its owned state. */
    map(const map &m) : buckets_(0), capacity_(0), count_(0), head_(0), tail_(0)
    {
        try
        {
            for (entry *e = m.head_; e; e = e->next)
                (*this)[e->first] = e->second;
        }
        catch (...)
        {
            clear();
            delete[] buckets_;
            throw;
        }
    }
    /* Compute and return &operator=(const map &m). */
    map &operator=(const map &m)
    {
        if (this != &m)
        {
            map tmp(m);
            swap(tmp);
        }
        return *this;
    }
    /* Release the resources owned by this map instance. */
    ~map()
    {
        clear();
        delete[] buckets_;
    }
    /* Exchange the current value. */
    void swap(map &m)
    {
        entry **b = buckets_;

        /* Computation / Вычисление и обновление состояния. */
        buckets_ = m.buckets_;
        m.buckets_ = b;
        size_t n = capacity_;

        /* Computation / Вычисление и обновление состояния. */
        capacity_ = m.capacity_;
        m.capacity_ = n;
        n = count_;
        count_ = m.count_;
        m.count_ = n;
        entry *p = head_;

        /* Computation / Вычисление и обновление состояния. */
        head_ = m.head_;
        m.head_ = p;
        p = tail_;
        tail_ = m.tail_;
        m.tail_ = p;
    }
    /* Clear the current value. */
    void clear()
    {
        entry *e = head_;
        while (e)
        {
            entry *n = e->next;
            delete e;
            e = n;
        }

        /* Computation / Вычисление и обновление состояния. */
        head_ = tail_ = 0;
        count_ = 0;
        for (size_t i = 0; i < capacity_; ++i)
            buckets_[i] = 0;
    }
    /* Removing a catch binding must also remove it from enumeration. */
    /* Compute and return erase. */
    bool erase(const K &key)
    {
        if (!capacity_)
            return false;

        /* Initialization / Инициализация локального состояния. */
        size_t bucket = hash_key(key) % capacity_;
        entry **link = &buckets_[bucket];
        while (*link && !((*link)->first == key))
            link = &(*link)->bucket_next;
        if (!*link)
            return false;
        entry *victim = *link;
        *link = victim->bucket_next;
        entry *previous = 0;
        for (entry *e = head_; e != victim; e = e->next)
            previous = e;
        if (previous)
            previous->next = victim->next;
        else
            head_ = victim->next;
        if (tail_ == victim)
            tail_ = previous;
        delete victim;
        --count_;
        return true;
    }
    /* Return the number of stored elements. */
    size_t size() const
    {
        return count_;
    }
    /* Report whether the container has no elements. */
    bool empty() const
    {
        return count_ == 0;
    }
    /* Return an iterator to the first element. */
    iterator begin() const
    {
        return iterator(head_);
    }
    /* Return the iterator past the last element. */
    iterator end() const
    {
        return iterator(0);
    }
    /* Find the current value. */
    iterator find(const K &key) const
    {
        if (!capacity_)
            return end();
        for (entry *e = buckets_[hash_key(key) % capacity_]; e; e = e->bucket_next)
            if (e->first == key)
                return iterator(e);
        return end();
    }
    /* Compute and return &operator[](const K &key). */
    V &operator[](const K &key)
    {
        iterator it = find(key);
        if (it != end())
            return it->second;
        if (!capacity_ || (count_ + 1) * 4 >= capacity_ * 3)
            rehash(capacity_ ? capacity_ * 2 : 16);

        /* Initialization / Инициализация локального состояния. */
        entry *e = new entry(key);
        size_t k = hash_key(key) % capacity_;

        /* Computation / Вычисление и обновление состояния. */
        e->bucket_next = buckets_[k];
        buckets_[k] = e;
        if (tail_)
            tail_->next = e;
        else
            head_ = e;

        /* Computation / Вычисление и обновление состояния. */
        tail_ = e;
        ++count_;
        return e->second;
    }
};
} // namespace fvm_detail
#endif
