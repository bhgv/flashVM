#include "../src/details/vector.hpp"
#include "../src/details/hash_table.hpp"
#include <cassert>
struct Value {
    static int live,until_throw;
    int n;
    Value():n(0) {++live;}
    Value(const Value &v):n(v.n) {++live;}
    ~Value(){--live;}
    Value &operator=(const Value &v) {
        if(until_throw==0)throw 1;
        if(until_throw>0)--until_throw;
        n=v.n;return *this;
    }
};
int Value::live=0;int Value::until_throw=-1;
int main() {
    Value::until_throw=0;
    try {fvm_detail::vector<Value> failed(10);assert(false);}catch(int){}
    assert(Value::live==0);Value::until_throw=-1;
    {
        fvm_detail::vector<Value> v;Value x;x.n=42;v.push_back(x);
        int count=Value::live;Value::until_throw=0;
        try {v.reserve(20);assert(false);}catch(int){}
        assert(Value::live==count && v.size()==1 && v[0].n==42);
        try {v.push_back(x);assert(false);}catch(int){}
        assert(Value::live==count && v.size()==1);
        Value::until_throw=-1;
    }
    assert(Value::live==0);
    {
        fvm_detail::map<unsigned long,Value> m;
        for(unsigned long i=0;i<5;++i)m[i].n=(int)i;
        int count=Value::live;Value::until_throw=2;
        try {fvm_detail::map<unsigned long,Value> copy(m);assert(false);}catch(int){}
        assert(Value::live==count && m.size()==5);
        Value::until_throw=-1;
    }
    assert(Value::live==0);return 0;
}
