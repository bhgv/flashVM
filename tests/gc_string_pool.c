#include "flashvm/gc.h"
#include "flashvm/stack.h"
#include "flashvm/auxlib.h"
#include "flashvm/exception.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Value value,borrowed;
    FVM_GCStringStats empty,first,last;FVM_Stack *stack;
    const char counted[]={'A',0,'B'};const char surrogate[]={(char)0xed,(char)0xa0,(char)0x80};
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_gc_string_stats(c,&empty)==FVM_OK && !empty.buffers);
    assert(fvm_value_string_bytes(&value,counted,sizeof(counted))==FVM_OK);
    assert(fvm_set_global(c,"counted",&value)==FVM_OK);
    assert(fvm_gc_string_stats(c,&first)==FVM_OK && first.buffers==1 && first.payload_bytes==3 && first.allocated_bytes==4);
    assert(fvm_get_global(c,"counted",&borrowed)==FVM_OK && borrowed.string_size==3);
    fvm_value_undefined(&value);assert(fvm_set_global(c,"counted",&value)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK);
    assert(fvm_gc_string_stats(c,&last)==FVM_OK && last.buffers==first.buffers && last.payload_bytes==first.payload_bytes && last.allocated_bytes==first.allocated_bytes);
    assert(!memcmp(borrowed.string,counted,sizeof(counted)));
    assert(fvm_value_string_bytes(&value,surrogate,sizeof(surrogate))==FVM_OK);
    assert(fvm_exception_raise(c,&value)==FVM_EXCEPTION);assert(fvm_gc_string_stats(c,&last)==FVM_OK && last.buffers==2 && last.payload_bytes==6 && last.allocated_bytes==8);
    assert(fvm_exception_clear(c)==FVM_OK);
    assert(fvm_stack_create(c,&stack)==FVM_OK);assert(fvm_stack_pushstring(stack,"abcd")==FVM_OK);
    assert(fvm_gc_string_stats(c,&last)==FVM_OK && last.buffers==3 && last.payload_bytes==10 && last.allocated_bytes==13);
    fvm_stack_destroy(stack);assert(fvm_gc_collect(c,0)==FVM_OK);
    assert(fvm_gc_string_stats(c,&last)==FVM_OK && last.buffers==3);
    last.buffers=987;assert(fvm_gc_string_stats(0,&last)==FVM_INVALID_ARGUMENT && last.buffers==987);
    assert(fvm_gc_string_stats(c,0)==FVM_INVALID_ARGUMENT);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
