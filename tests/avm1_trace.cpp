/* ActionTrace uses a retained synchronous host sink rather than a global function. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/gc.h"
#include <new>
static unsigned delivered,converted;
static FVM_Object *capture;
static FVM_Context *other;
static FVM_Value last;
static FVM_Result sink(FVM_Context *c,FVM_Object *receiver,unsigned count,const FVM_Value *a,FVM_Value *out,void *userdata)
{
    assert(receiver && count==1 && a[0].type==FVM_VALUE_STRING);
    assert(userdata==capture);FVM_Value value;
    assert(fvm_object_get(c,capture,"alive",&value)==FVM_OK && value.number==42);
    assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_set_trace_rooted(other, sink,0,capture)==FVM_INVALID_ARGUMENT);
    last=a[0];++delivered;fvm_value_number(out,99);return FVM_OK;
}
static FVM_Result stringify(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
    ++converted;assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);
    return fvm_value_string_bytes(out,"a\0b",3);
}
static FVM_Result error_sink(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ return FVM_ERROR; }
static FVM_Result throw_sink(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ throw std::bad_alloc(); }
static FVM_Result replacement(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ ++delivered;return fvm_avm1_set_trace_rooted(c,0,0,0); }
static FVM_Result trace(FVM_Context *c,const FVM_Value &value)
{
    assert(fvm_set_global(c,"input",&value)==FVM_OK);Bytes b;global(b,"input");
    b.push_back(0x26);b.push_back(0);return fvm_avm1_execute(c,b.begin(),b.size());
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *o=0;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);
    assert(fvm_standard_install(c)==FVM_OK);assert(fvm_object_create(c,&capture)==FVM_OK);
    FVM_Value v;fvm_value_number(&v,42);assert(fvm_object_set(c,capture,"alive",&v)==FVM_OK);
    assert(fvm_avm1_set_trace_rooted(c,sink,capture,capture)==FVM_OK);
    unsigned long freed=0;assert(fvm_gc_collect(c,&freed)==FVM_OK);
    /* A global named trace cannot intercept ActionTrace. */
    fvm_value_number(&v,7);assert(fvm_set_global(c,"trace",&v)==FVM_OK);
    assert(trace(c,v)==FVM_OK && delivered==1 && !std::strcmp(last.string,"7"));
    fvm_value_undefined(&v);assert(trace(c,v)==FVM_OK && !std::strcmp(last.string,"undefined"));
    assert(fvm_construct(c,"Object",0,0,&o)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,o,"toString",stringify,0,0)==FVM_OK);
    v.type=FVM_VALUE_OBJECT;v.object=o;converted=0;
    assert(trace(c,v)==FVM_OK && converted==1 && fvm_value_string_size(&last)==3 && !std::memcmp(last.string,"a\0b",3));
    assert(fvm_avm1_set_trace_rooted(c,0,0,0)==FVM_OK);
    assert(trace(c,v)==FVM_OK && converted==2);
    assert(fvm_avm1_set_trace_rooted(c,0,capture,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_avm1_set_trace_rooted(c,error_sink,0,0)==FVM_OK);assert(trace(c,v)==FVM_ERROR);
    assert(fvm_avm1_set_trace_rooted(c,throw_sink,0,0)==FVM_OK);assert(trace(c,v)==FVM_OUT_OF_MEMORY);
    assert(fvm_avm1_set_trace_rooted(c,replacement,0,capture)==FVM_OK);
    unsigned prior=delivered;assert(trace(c,v)==FVM_OK && delivered==prior+1);
    assert(trace(c,v)==FVM_OK && delivered==prior+1);
    assert(fvm_gc_collect(c,&freed)==FVM_OK && freed>=1);
    const unsigned char empty[]={0x26,0};assert(fvm_avm1_execute(c,empty,sizeof(empty))==FVM_ERROR);
    fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
