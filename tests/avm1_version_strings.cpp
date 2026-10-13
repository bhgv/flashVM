/* Legacy ordinary undefined string conversion is distinct from Trace spelling. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "../src/details/string.hpp"
static fvm_detail::string traced;
static FVM_Result sink(FVM_Context *,FVM_Object *,unsigned n,const FVM_Value *a,FVM_Value *,void *)
{ assert(n==1 && a[0].type==FVM_VALUE_STRING);traced=fvm_detail::string(a[0].string,fvm_value_string_size(a));return FVM_OK; }
static FVM_Value nullish_call(FVM_Context *c, const FVM_Value &owner)
{
    assert(fvm_set_global(c,"owner",&owner)==FVM_OK);
    Bytes b;integer(b,9);integer(b,1);global(b,"owner");text(b,"toString");b.push_back(0x52);
    return run(c,b);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=5;version<=8;++version)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',0,16,0,0,0,0x08,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        FVM_Value value;fvm_value_undefined(&value);assert(fvm_set_global(c,"input",&value)==FVM_OK);
        Bytes b;global(b,"input");b.push_back(0x4b);FVM_Value result=run(c,b);
        assert(result.type==FVM_VALUE_STRING && !std::strcmp(result.string,version<7?"":"undefined"));
        b.clear();text(b,"prefix");global(b,"input");b.push_back(0x47);result=run(c,b);
        assert(result.type==FVM_VALUE_STRING && !std::strcmp(result.string,version<7?"prefix":"prefixundefined"));
        b.clear();global(b,"input");text(b,"suffix");b.push_back(0x47);result=run(c,b);
        assert(result.type==FVM_VALUE_STRING && !std::strcmp(result.string,version<7?"suffix":"undefinedsuffix"));
        assert(fvm_avm1_set_trace_rooted(c,sink,0,0)==FVM_OK);
        b.clear();global(b,"input");b.push_back(0x26);b.push_back(0);
        assert(fvm_avm1_execute(c,b.begin(),b.size())==FVM_OK && traced==fvm_detail::string("undefined"));
        assert(nullish_call(c,value).type==FVM_VALUE_UNDEFINED);value.type=FVM_VALUE_NULL;
        assert(nullish_call(c,value).type==FVM_VALUE_UNDEFINED);
        assert(fvm_set_global(c,"input",&value)==FVM_OK);b.clear();global(b,"input");b.push_back(0x4b);
        assert(!std::strcmp(run(c,b).string,"null"));
        /* Nullish receivers still validate their argument count and available operands. */
        b.clear();integer(b,1);global(b,"owner");text(b,"method");b.push_back(0x52);b.push_back(0);
        assert(fvm_avm1_execute(c,b.begin(),b.size())==FVM_ERROR);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
