#include "flashvm/gc.h"
#include "flashvm/table.h"
#include <assert.h>
int main(void) {
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *root,*last,*next;
    FVM_Value v;unsigned long i,n;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_object_create(c,&root)==FVM_OK);
    last=root;
    for(i=1;i<12000UL;++i) {
        assert(fvm_object_create(c,&next)==FVM_OK);
        fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=next;
        assert(fvm_table_set_index(c,last,0,&v)==FVM_OK);last=next;
    }
    v.object=root;assert(fvm_object_set(c,last,"cycle",&v)==FVM_OK);
    assert(fvm_gc_pin(c,root)==FVM_OK);assert(fvm_gc_collect(c,&n)==FVM_OK && n==0);
    assert(fvm_gc_object_count(c,&n)==FVM_OK && n==12001UL);
    assert(fvm_gc_unpin(c,root)==FVM_OK);assert(fvm_gc_collect(c,&n)==FVM_OK && n==12000UL);
    assert(fvm_gc_object_count(c,&n)==FVM_OK && n==1);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
