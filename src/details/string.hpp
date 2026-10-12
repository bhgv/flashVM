/* FlashVM: C++98 containers and implementation utilities: string.
 * File: src/details/string.hpp.
 */

#ifndef FVM_DETAILS_STRING_HPP
/* Define FVM DETAILS STRING HPP constant or compilation switch. */
#define FVM_DETAILS_STRING_HPP
/* Owned byte string for internal identifiers. C++98, no STL dependencies.
 * Embedded NUL bytes are retained; c_str() provides a terminating NUL.
 * This is NOT a Unicode/UTF-16 ActionScript string implementation. */
#include <cstddef>
#include <cstring>
#include <new>
namespace fvm_detail
{
/* Define string class and its representation. */
class string
{
    char *data_;    /* Payload storage used by this operation. */
    size_t length_; /* Length of the associated sequence. */
    /* Copy bytes. */
    static char *copy_bytes(const char *p, size_t n)
    {
        if (n == (size_t)-1)
            throw std::bad_alloc();
        char *s = new char[n + 1];
        if (n)
            std::memcpy(s, p, n);
        s[n] = 0;
        return s;
    }

  public:
    /* Initialize string and its owned state. */
    string() : data_(copy_bytes("", 0)), length_(0)
    {
    }
    /* Initialize string and its owned state. */
    string(const char *p) : data_(0), length_(0)
    {
        if (!p)
            p = "";

        /* Computation / Вычисление и обновление состояния. */
        length_ = std::strlen(p);
        data_ = copy_bytes(p, length_);
    }
    /* Initialize string and its owned state. */
    string(const char *p, size_t n) : data_(0), length_(n)
    {
        data_ = copy_bytes(p, n);
    }
    /* Initialize string and its owned state. */
    string(const string &s) : data_(copy_bytes(s.data_, s.length_)), length_(s.length_)
    {
    }
    /* Release the resources owned by this string instance. */
    ~string()
    {
        delete[] data_;
    }
    /* Compute and return &operator=(const string &s). */
    string &operator=(const string &s)
    {
        if (this != &s)
        {
            string tmp(s);
            swap(tmp);
        }
        return *this;
    }
    /* Compute and return &operator=(const char *s). */
    string &operator=(const char *s)
    {
        string tmp(s);
        swap(tmp);
        return *this;
    }
    /* Exchange the current value. */
    void swap(string &s)
    {
        char *p = data_;

        /* Computation / Вычисление и обновление состояния. */
        data_ = s.data_;
        s.data_ = p;
        size_t n = length_;

        /* Computation / Вычисление и обновление состояния. */
        length_ = s.length_;
        s.length_ = n;
    }
    /* Return the number of stored elements. */
    size_t size() const
    {
        return length_;
    }
    /* Report whether the container has no elements. */
    bool empty() const
    {
        return length_ == 0;
    }
    /* Compute and return c str. */
    const char *c_str() const
    {
        return data_;
    }
    /* Compute and return data. */
    const char *data() const
    {
        return data_;
    }
    /* Access the indexed element without a bounds check. */
    char operator[](size_t i) const
    {
        return data_[i];
    }
    /* Compare both operands for equality. */
    bool operator==(const string &s) const
    {
        return length_ == s.length_ && (!length_ || std::memcmp(data_, s.data_, length_) == 0);
    }
    /* Compare both operands for inequality. */
    bool operator!=(const string &s) const
    {
        return !(*this == s);
    }
};
} // namespace fvm_detail
#endif
