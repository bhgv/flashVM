#include "flashvm/gc.h"
#include "flashvm/stack.h"
#include "flashvm/userdata.h"
#include "flashvm/metamethod.h"
#include "flashvm/table.h"
#include "flashvm/exception.h"
#include <assert.h>
#include <string.h>
typedef struct Payload
{
    FVM_Context *context;FVM_Object *object,*child,*keeper;FVM_Stack *stack;
    unsigned released,finalized,keeper_released;
} Payload;
static void release_keeper(void *raw){++((Payload *)raw)->keeper_released;}
static FVM_Result finalize(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *a,FVM_Value *out,void *raw)
{
    Payload *p=(Payload *)raw;(void)c;(void)self;(void)n;(void)a;
    ++p->finalized;fvm_value_undefined(out);return FVM_OK;
}
static void release(void *raw)
{
    Payload *p=(Payload *)raw;FVM_Value v;++p->released;
    fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=p->object;
    assert(fvm_set_global(p->context,"saved",&v)==FVM_OK);
    assert(fvm_exception_raise(p->context,&v)==FVM_EXCEPTION);
    v.object=p->child;
    assert(fvm_object_set(p->context,p->keeper,"late",&v)==FVM_OK);
    assert(fvm_stack_push(p->stack,&v)==FVM_OK);
    fvm_value_undefined(&v);assert(fvm_set_global(p->context,"keeper",&v)==FVM_OK);
}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c;FVM_Object *meta;
    FVM_Value v,out,edge;FVM_GCStringStats strings;Payload p;unsigned long freed;int traced;void *data;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(traced=0;traced<2;++traced)
    {
        memset(&p,0,sizeof(p));assert(fvm_context_create(rt,&c)==FVM_OK);p.context=c;
        assert(fvm_gc_string_mode(c,traced?FVM_GC_STRINGS_TRACED:FVM_GC_STRINGS_ARENA)==FVM_OK);
        assert(fvm_stack_create(c,&p.stack)==FVM_OK);
        assert(fvm_object_create(c,&p.object)==FVM_OK);assert(fvm_object_create(c,&p.child)==FVM_OK);
        assert(fvm_object_create(c,&p.keeper)==FVM_OK);assert(fvm_object_create(c,&meta)==FVM_OK);
        assert(fvm_userdata_attach(c,p.object,&p,release)==FVM_OK);
        assert(fvm_userdata_attach(c,p.keeper,&p,release_keeper)==FVM_OK);
        assert(fvm_meta_register(c,meta,"__gc",finalize,&p)==FVM_OK);
        assert(fvm_table_set_metatable(c,p.object,meta)==FVM_OK);
        fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=p.child;
        assert(fvm_object_set(c,p.object,"child",&v)==FVM_OK);
        assert(fvm_object_set(c,p.child,"self",&v)==FVM_OK);
        v.object=p.keeper;assert(fvm_set_global(c,"keeper",&v)==FVM_OK);
        assert(fvm_value_string_bytes(&v,"A\0B",3)==FVM_OK);
        assert(fvm_object_set(c,p.child,"text",&v)==FVM_OK);
        assert(fvm_gc_collect(c,&freed)==FVM_OK && !freed);
        assert(p.released==1 && p.finalized==1 && !p.keeper_released);
        assert(fvm_get_global(c,"saved",&out)==FVM_OK && out.object==p.object);
        assert(fvm_exception_get(c,&out)==FVM_OK && out.object==p.object);
        assert(fvm_userdata_get(c,p.object,&data)==FVM_OK && !data);
        assert(fvm_object_get(c,p.keeper,"late",&edge)==FVM_OK && edge.object==p.child);
        assert(fvm_object_get(c,p.child,"self",&edge)==FVM_OK && edge.object==p.child);
        assert(fvm_object_get(c,p.child,"text",&edge)==FVM_OK && !memcmp(edge.string,"A\0B",3));
        assert(fvm_gc_collect(c,&freed)==FVM_OK && freed==1 && p.keeper_released==1);
        fvm_value_undefined(&v);assert(fvm_set_global(c,"saved",&v)==FVM_OK);
        assert(fvm_exception_clear(c)==FVM_OK);assert(fvm_stack_clear(p.stack)==FVM_OK);
        assert(fvm_gc_collect(c,&freed)==FVM_OK && freed==3);
        assert(p.released==1 && p.finalized==1 && p.keeper_released==1);
        assert(fvm_gc_string_stats(c,&strings)==FVM_OK && strings.buffers==(traced?0:1));
        fvm_stack_destroy(p.stack);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
