#include "flashvm/gc.h"
#include "flashvm/stack.h"
#include "flashvm/callable.h"
#include <assert.h>
static FVM_Result native(FVM_Context *c,FVM_Object *self,unsigned int argc,
    const FVM_Value *args,FVM_Value *out,void *ud) {
    (void)c;(void)self;(void)argc;(void)args;(void)ud;
    fvm_value_undefined(out);return FVM_OK;
}
static FVM_Result callable(FVM_Context *c,FVM_Stack *s,void *ud) {
    (void)c;(void)ud;return fvm_stack_clear(s);
}
int main(void) {
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *o,*fn;
    FVM_Value v;FVM_Stack *a,*b;unsigned long n;int is;
    assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_object_create(c,&o)==FVM_OK);
    assert(fvm_register_function_rooted(c,"capture",native,0,o)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==0);
    assert(fvm_register_function(c,"capture",native,0)==FVM_OK);
    assert(fvm_stack_create(c,&a)==FVM_OK);
    assert(fvm_stack_create(c,&b)==FVM_OK);
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;
    assert(fvm_stack_push(a,&v)==FVM_OK);
    fvm_stack_destroy(b); /* Independent stacks need not close in LIFO order. */
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==0);
    assert(fvm_stack_clear(a)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==1);
    fvm_stack_destroy(a);
    assert(fvm_callable_create(c,callable,0,&fn)==FVM_OK);
    assert(fvm_callable_is(c,fn,&is)==FVM_OK && is);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==1);
    assert(fvm_object_create(c,&o)==FVM_OK);
    assert(fvm_callable_is(c,o,&is)==FVM_OK && !is);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
