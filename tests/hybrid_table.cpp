#include "../src/details/hybrid_table.hpp"
#include <cassert>
int main() {
    fvm_detail::hybrid_table<int> t;
    int v=0;
    t.set(0UL,11);t.set(2UL,33);t.set(100000000UL,99);
    t.set(fvm_detail::string("key"),44);
    assert(t.size()==4);
    assert(t.array_capacity()<1000000UL);
    assert(t.get(2UL,&v)&&v==33);
    assert(t.get(100000000UL,&v)&&v==99);
    assert(!t.get(1UL,&v));
    assert(t.get(fvm_detail::string("key"),&v)&&v==44);
    t.set(2UL,55);assert(t.size()==4);
    t.queue_push(7);t.queue_push(8);
    assert(t.queue_pop(&v)&&v==7);
    assert(t.queue_pop(&v)&&v==8);
    assert(!t.queue_pop(&v));
    return 0;
}
