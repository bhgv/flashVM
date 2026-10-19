/* Every allocation failure during URL conversion leaves no partial string or live scope. */
#include <cstdlib>
#include <new>
static long fail_after=-1;
void *operator new(size_t n) { if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;void *p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p; }
void *operator new[](size_t n){return ::operator new(n);}
void operator delete(void *p) throw(){std::free(p);}
void operator delete[](void *p) throw(){std::free(p);}
void *operator new(size_t n,const std::nothrow_t &) throw(){try{return ::operator new(n);}catch(...){return 0;}}
void *operator new[](size_t n,const std::nothrow_t &) throw(){try{return ::operator new(n);}catch(...){return 0;}}
void operator delete(void *p,const std::nothrow_t &) throw(){std::free(p);}
void operator delete[](void *p,const std::nothrow_t &) throw(){std::free(p);}
#include "../src/core/runtime.hpp"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstring>
int main()
{
    for(unsigned decode=0;decode<2;++decode) {
        unsigned failures=0;bool completed=false;
        for(long allocation=0;allocation<256 && !completed;++allocation) {
            FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
            assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg,&rt) == FVM_OK);
            assert(fvm_context_create(rt,&c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
            const char *name=decode?"unescape":"escape";
            FVM_Value arg,out;assert(fvm_value_string(&arg,decode?"a%40b%2Ec+%00%E4%B8%AD":"a@b.c \xe4\xb8\xad") == FVM_OK);
            size_t before=c->strings.size();fail_after=allocation;FVM_Result status=fvm_call(c,name,1,&arg,&out);fail_after=-1;
            assert(status == FVM_OK || status == FVM_OUT_OF_MEMORY);
            assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->string_depth);
            if(status == FVM_OUT_OF_MEMORY) {++failures;assert(out.type == FVM_VALUE_UNDEFINED);assert(c->strings.size()==before);}
            else {completed=true;assert(out.type == FVM_VALUE_STRING);}
            assert(fvm_gc_collect(c,0) == FVM_OK && fvm_call(c,name,1,&arg,&out) == FVM_OK);
            /* Public dispatch preserves aliased input before clearing callback output. */
            assert(fvm_call(c,name,1,&arg,&arg) == FVM_OK && arg.type == FVM_VALUE_STRING);
            fvm_context_destroy(c);fvm_runtime_destroy(rt);
        }
        assert(completed && failures>0);
    }
}
