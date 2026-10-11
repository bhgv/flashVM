#ifndef FVM_DETAILS_STRING_HPP
#define FVM_DETAILS_STRING_HPP
/* Owned byte string for internal identifiers. C++98, no STL dependencies.
 * Embedded NUL bytes are retained; c_str() provides a terminating NUL.
 * This is NOT a Unicode/UTF-16 ActionScript string implementation. */
#include <cstddef>
#include <cstring>
#include <new>
namespace fvm_detail {
class string {
    char *data_;
    size_t length_;
    static char *copy_bytes(const char *p, size_t n) {
        if (n==(size_t)-1) throw std::bad_alloc();
        char *s=new char[n+1];
        if(n) std::memcpy(s,p,n);
        s[n]=0;
        return s;
    }
public:
    string():data_(copy_bytes("",0)),length_(0) {}
    string(const char *p):data_(0),length_(0) {
        if(!p)p="";
        length_=std::strlen(p);
        data_=copy_bytes(p,length_);
    }
    string(const char *p,size_t n):data_(0),length_(n) { data_=copy_bytes(p,n); }
    string(const string &s):data_(copy_bytes(s.data_,s.length_)),length_(s.length_) {}
    ~string(){delete [] data_;}
    string &operator=(const string &s){if(this!=&s){string tmp(s);swap(tmp);}return *this;}
    string &operator=(const char *s){string tmp(s);swap(tmp);return *this;}
    void swap(string &s){char *p=data_;data_=s.data_;s.data_=p;size_t n=length_;length_=s.length_;s.length_=n;}
    size_t size() const{return length_;}
    bool empty() const{return length_==0;}
    const char *c_str() const{return data_;}
    const char *data() const{return data_;}
    char operator[](size_t i) const{return data_[i];}
    bool operator==(const string &s) const{return length_==s.length_ && (!length_ || std::memcmp(data_,s.data_,length_)==0);}
    bool operator!=(const string &s) const{return !(*this==s);}
};
}
#endif
