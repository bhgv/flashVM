#include "flashvm/gc.h"
#include "flashvm/stack.h"
#include "flashvm/callable.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/gc_roots.h"
#include <assert.h>
static FVM_Result stackfn(FVM_Context *c,FVM_Stack *s,void *u) {(void)c;(void)u;return fvm_stack_clear(s);}
static FVM_Result native(FVM_Context *c,FVM_Object *s,unsigned int n,const FVM_Value *a,FVM_Value *o,void *u) {
    (void)c;(void)s;(void)n;(void)a;(void)u;fvm_value_undefined(o);return FVM_OK;
}
int main(void) {
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *root,*fn,*m,*o;
    FVM_Value v;unsigned long n;FVM_GCRootScope *scope;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_object_create(c,&root)==FVM_OK);
    assert(fvm_stack_register_rooted(c,"rooted",stackfn,0,root)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==0);
    assert(fvm_stack_register(c,"rooted",stackfn,0)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==1);
    assert(fvm_object_create(c,&root)==FVM_OK);
    assert(fvm_callable_create_rooted(c,stackfn,0,root,&fn)==FVM_OK);
    assert(fvm_gc_pin(c,fn)==FVM_OK);assert(fvm_gc_collect(c,&n)==FVM_OK && n==0);
    assert(fvm_gc_unpin(c,fn)==FVM_OK);assert(fvm_gc_collect(c,&n)==FVM_OK && n==2);
    assert(fvm_object_create(c,&root)==FVM_OK);assert(fvm_object_create(c,&m)==FVM_OK);
    assert(fvm_object_create(c,&o)==FVM_OK);
    assert(fvm_meta_register_rooted(c,m,"__gc",native,0,root)==FVM_OK);
    assert(fvm_table_set_metatable(c,o,m)==FVM_OK);
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;
    assert(fvm_set_global(c,"owner",&v)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==0);
    fvm_value_undefined(&v);assert(fvm_set_global(c,"owner",&v)==FVM_OK);
    assert(fvm_gc_collect(c,&n)==FVM_OK && n==3);
    assert(fvm_gc_roots_begin(c,&scope)==FVM_OK);
    fvm_value_undefined(&v);v.type=(FVM_ValueType)99;
    assert(fvm_gc_roots_add(scope,&v)==FVM_INVALID_ARGUMENT);
    v.type=FVM_VALUE_STRING;v.string=0;
    assert(fvm_gc_roots_add(scope,&v)==FVM_INVALID_ARGUMENT);
    assert(fvm_gc_roots_end(scope)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
