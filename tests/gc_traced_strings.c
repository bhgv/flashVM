#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "flashvm/exception.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include <assert.h>
#include <string.h>
static unsigned finals;
static FVM_Value newborn;
static FVM_Result finalize(FVM_Context *c,FVM_Object *self,unsigned int n,const FVM_Value *a,FVM_Value *out,void *raw)
{
    FVM_Value v;unsigned short text[3]={'N',0,'B'};
    (void)n;(void)a;(void)raw;
    assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_ARENA)==FVM_INVALID_ARGUMENT);
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=self;
    assert(fvm_set_global(c,"resurrected",&v)==FVM_OK);
    assert(fvm_value_string_utf16(c,text,3,&newborn)==FVM_OK);
    ++finals;fvm_value_undefined(out);return FVM_OK;
}
static void counts(FVM_Context *c,size_t buffers,size_t bytes)
{
    FVM_GCStringStats stats;
    assert(fvm_gc_string_stats(c,&stats)==FVM_OK);
    assert(stats.buffers==buffers && stats.payload_bytes==bytes && stats.allocated_bytes==bytes+buffers);
}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Value v,view,out;
    FVM_GCRootScope *scope;FVM_Object *o,*meta;unsigned short text[3]={'A',0,'B'};
    int mode=-1;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_gc_string_state(c,&mode)==FVM_OK && mode==FVM_GC_STRINGS_ARENA);
    assert(fvm_gc_string_mode(c,2)==FVM_INVALID_ARGUMENT);
    assert(fvm_gc_string_mode(0,FVM_GC_STRINGS_TRACED)==FVM_INVALID_ARGUMENT);
    assert(fvm_gc_string_state(c,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_value_string_utf16(c,text,3,&v)==FVM_OK);counts(c,1,3);
    assert(fvm_gc_collect(c,0)==FVM_OK);counts(c,1,3);
    assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
    assert(fvm_gc_string_state(c,&mode)==FVM_OK && mode==FVM_GC_STRINGS_TRACED);
    assert(fvm_gc_roots_begin(c,&scope)==FVM_OK);
    assert(fvm_value_string_bytes(&view,v.string+1,2)==FVM_OK);
    assert(fvm_gc_roots_add(scope,&view)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK);counts(c,1,3);
    assert(!memcmp(view.string,"\0B",2));
    assert(fvm_gc_roots_end(scope)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK);counts(c,0,0);
    assert(fvm_value_string_utf16(c,text,3,&v)==FVM_OK);
    assert(fvm_exception_raise(c,&v)==FVM_EXCEPTION);counts(c,2,6);
    assert(fvm_gc_collect(c,0)==FVM_OK);counts(c,1,3);
    assert(fvm_exception_get(c,&out)==FVM_OK && !memcmp(out.string,"A\0B",3));
    assert(fvm_exception_clear(c)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK);counts(c,0,0);
    assert(fvm_object_create(c,&o)==FVM_OK);assert(fvm_object_create(c,&meta)==FVM_OK);
    assert(fvm_value_string(&v,"retained")==FVM_OK);
    assert(fvm_object_set(c,o,"text",&v)==FVM_OK);
    assert(fvm_meta_register(c,meta,"__gc",finalize,0)==FVM_OK);
    assert(fvm_table_set_metatable(c,o,meta)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK && finals==1);counts(c,2,11);
    assert(fvm_get_global(c,"resurrected",&out)==FVM_OK && out.object==o);
    assert(fvm_object_get(c,o,"text",&out)==FVM_OK && !strcmp(out.string,"retained"));
    assert(!memcmp(newborn.string,"N\0B",3));
    assert(fvm_gc_collect(c,0)==FVM_OK && finals==1);counts(c,1,8);
    fvm_value_undefined(&v);assert(fvm_set_global(c,"resurrected",&v)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK && finals==1);counts(c,0,0);
    assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_ARENA)==FVM_OK);
    assert(fvm_value_string_utf16(c,text,3,&v)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK);counts(c,1,3);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
