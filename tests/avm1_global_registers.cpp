/* Shared loaded register values survive activations and GC; private Function2 banks stay isolated. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
struct Probe { bool loaded; };
static FVM_Result native_body(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
    FVM_Value value;fvm_value_number(&value,99);
    assert(fvm_exec_global_register(c,1,1,&value,out)==FVM_OK);
    assert(frame->register_count>1);
    if(((Probe *)raw)->loaded)assert(frame->registers[1].type==FVM_VALUE_NUMBER && frame->registers[1].number==99);
    else assert(frame->registers[1].type==FVM_VALUE_UNDEFINED);
    return FVM_OK;
}
static bool present(FVM_Context *c,FVM_Object *object)
{ for(size_t i=0;i<c->objects.size();++i)if(c->objects[i]==object)return true;return false; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=9;++version) {
        FVM_Context *c=0,*other=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_context_create(rt,&other)==FVM_OK);
        if(version<=8) { unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK); }
        assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        Bytes store;integer(store,42);store.push_back(0x87);u16(store,1);store.push_back(1);store.push_back(0x17);store.push_back(0);
        assert(fvm_avm1_execute(c,store.begin(),store.size())==FVM_OK);
        Bytes read;reg(read,1);end(read);FVM_Object *reader=0;assert(fvm_function_create_avm1(c,read.begin(),read.size(),0,0,0,&reader)==FVM_OK && fvm_gc_pin(c,reader)==FVM_OK);
        FVM_Value out;assert(fvm_function_invoke(c,reader,0,0,0,&out)==FVM_OK);
        if(version<=8)assert(out.type==FVM_VALUE_NUMBER && out.number==42);else assert(out.type==FVM_VALUE_UNDEFINED);
        Probe probe={version<=8};assert(fvm_function_set_body(c,reader,native_body,&probe,0)==FVM_OK);
        assert(fvm_function_invoke(c,reader,0,0,0,&out)==FVM_OK && out.number==99);
        assert(fvm_function_set_body(c,reader,0,0,0)==FVM_OK);
        if(version<=8) {
            Bytes local;integer(local,88);local.push_back(0x87);u16(local,1);local.push_back(1);local.push_back(0x17);reg(local,1);end(local);
            FVM_Object *fn=define(c,2,0,0,0,0,local);assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==88);
            fn=define(c,0,0,0,0,0,read);assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==99);
        }
        FVM_Object *object=0,*meta=0;assert(fvm_construct(c,"Object",0,0,&object)==FVM_OK && fvm_object_create(c,&meta)==FVM_OK);
        FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=object;
        assert(fvm_exec_global_register(c,1,0,&value,&out)==FVM_OK);
        char bytes[]={'a',0,'b'};fvm_value_string_bytes(&value,bytes,sizeof(bytes));value.metatable=meta;
        assert(fvm_exec_global_register(c,1,2,&value,&value)==FVM_OK);bytes[0]='x';bytes[2]='y';
        assert(fvm_gc_collect(c,0)==FVM_OK && present(c,object) && present(c,meta));
        assert(fvm_exec_global_register(c,0,2,0,&out)==FVM_OK && out.type==FVM_VALUE_STRING && out.string_size==3 && !std::memcmp(out.string,"a\0b",3) && out.metatable==meta);
        assert(fvm_exec_global_register(other,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        fvm_value_undefined(&value);assert(fvm_exec_global_register(c,1,0,&value,&out)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK && !present(c,object));
        assert(fvm_exec_global_register(c,0,FVM_EXEC_GLOBAL_REGISTER_COUNT,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        assert(!c->gc_frames && !c->script_depth && !c->avm1_target_active);
        fvm_context_destroy(other);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
