/* FlashVM: C++98 containers and implementation utilities: vector.
 * File: src/details/vector.hpp.
 */

#ifndef FVM_DETAILS_VECTOR_HPP
/* Define FVM DETAILS VECTOR HPP constant or compilation switch. */
#define FVM_DETAILS_VECTOR_HPP
#include <cstddef>
#include <new>

namespace fvm_detail
{ /* Define vector class and its representation. */

template <class T> class vector
{
    T *items_;     /* Contiguous storage owned by this container. */
    size_t count_ /* Number of active entries. */,
        capacity_; /* Allocated storage limit; zero may denote an unbounded queue. */

  public:

    typedef T *iterator;
    typedef const T *const_iterator;


    /* Initialize vector and its owned state. */
    vector() : items_(0), count_(0), capacity_(0)
    {
    }


    /* Initialize vector and its owned state. */
    explicit vector(size_t n) : items_(0), count_(0), capacity_(0)
    {
        try
        {
            resize(n);
        }
        catch (...)
        {
            delete[] items_;
            throw;
        }
    }


    /* Initialize vector and its owned state. */
    vector(const vector &v) : items_(0), count_(0), capacity_(0)
    {
        assign(v.begin(), v.end());
    }


    /* Release the resources owned by this vector instance. */
    ~vector()
    {
        delete[] items_;
    }

    /* Compute and return &operator=(const vector &v). */
    vector &operator=(const vector &v)
    {
        if (this != &v)
        {


            vector tmp(v);
            swap(tmp);
        }
        return *this;
    }


    /* Exchange the current value. */
    void swap(vector &v)
    {
        T *p = items_;

        /* Computation / Вычисление и обновление состояния. */
        items_ = v.items_;
        v.items_ = p;
        size_t n = count_;

        /* Computation / Вычисление и обновление состояния. */
        count_ = v.count_;
        v.count_ = n;
        n = capacity_;
        capacity_ = v.capacity_;
        v.capacity_ = n;
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
    iterator begin()
    {
        return items_;
    }


    /* Return an iterator to the first element. */
    const_iterator begin() const
    {
        return items_;
    }


    /* Return the iterator past the last element. */
    iterator end()
    {
        return items_ ? items_ + count_ : 0;
    }


    /* Return the iterator past the last element. */
    const_iterator end() const
    {
        return items_ ? items_ + count_ : 0;
    }

    /* Compute and return &operator[](size t i). */
    T &operator[](size_t i)
    {
        return items_[i];
    }

    /* Compute and return &operator[](size t i) const. */
    const T &operator[](size_t i) const
    {
        return items_[i];
    }


    /* Compute and return &front(). */
    T &front()
    {
        return items_[0];
    }


    /* Compute and return &front() const. */
    const T &front() const
    {
        return items_[0];
    }


    /* Compute and return &back(). */
    T &back()
    {
        return items_[count_ - 1];
    }


    /* Compute and return &back() const. */
    const T &back() const
    {
        return items_[count_ - 1];
    }


    /* Reserve the current value. */
    void reserve(size_t n)
    {
        if (n <= capacity_)
        {
            return;
        }
        T *p = new T[n];
        try
        {
            for (size_t i = 0; i < count_; ++i)
            {
                p[i] = items_[i];
            }
        }
        catch (...)
        {
            delete[] p;
            throw;
        }
        delete[] items_;

        /* Computation / Вычисление и обновление состояния. */
        items_ = p;
        capacity_ = n;
    }


    /* Resize the current value. */
    void resize(size_t n)
    {
        if (n > capacity_)
        {
            reserve(n);
        }
        if (n < count_)
        {
            for (size_t i = n; i < count_; ++i)
            {
                items_[i] = T();
            }
        }
        else
        {
            for (size_t i = count_; i < n; ++i)
            {
                items_[i] = T();
            }
        }
        count_ = n;
    }


    /* Push back. */
    void push_back(const T &v)
    {
        T copy = v;
        if (count_ == capacity_)
        {
            if (capacity_ > (size_t)-1 / 2)
            {
                throw std::bad_alloc();
            }
            reserve(capacity_ ? capacity_ * 2 : 8);
        }

        /* Computation / Вычисление и обновление состояния. */
        items_[count_] = copy;
        ++count_;
    }


    /* Pop back. */
    void pop_back()
    {
        if (count_)
        {
            --count_;
            items_[count_] = T();
        }
    }


    /* Clear the current value. */
    void clear()
    {
        resize(0);
    }


    /* Compute and return erase. */
    iterator erase(iterator it)
    {
        size_t i = (size_t)(it - items_);
        if (i >= count_)
        {
            return end();
        }
        for (size_t j = i + 1; j < count_; ++j)
        {
            items_[j - 1] = items_[j];
        }
        pop_back();
        return items_ + i;
    }


    /* Assign the current value. */
    template <class It> void assign(It first, It last)
    {
        vector tmp;
        for (; first != last; ++first)
        {
            tmp.push_back(*first);
        }
        swap(tmp);
    }
};
} // namespace fvm_detail
#endif
