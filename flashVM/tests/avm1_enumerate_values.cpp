/* Loaded enumeration accepts primitives, stringifies names once and uses an undefined sentinel. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static unsigned names;
static bool name_failure;
static FVM_Result name_value(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    ++names;assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Value v;
    assert(fvm_object_get(c,frame->receiver,"alive",&v)==FVM_OK && v.number==17);
    if(name_failure){fvm_value_number(out,77);return fvm_exception_raise(c,out);}
    return fvm_value_string(out,"target");
}
static FVM_Result forbidden_box(FVM_Context *,const FVM_ScriptFrame *,FVM_Value *,void *)
{assert(0);return FVM_ERROR;}
static void run(FVM_Context *c,Bytes &code){code.push_back(0);assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK);}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    {
        FVM_Context *c=0;FVM_Value value,out;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        attach(c,c->global,"Number",forbidden_box,0);attach(c,c->global,"Boolean",forbidden_box,0);attach(c,c->global,"String",forbidden_box,0);
        for(unsigned kind=0;kind<5;++kind)
        {
            fvm_value_undefined(&value);
            if(kind==1)value.type=FVM_VALUE_NULL;
            if(kind==2)fvm_value_number(&value,17);
            if(kind==3)fvm_value_boolean(&value,1);
            if(kind==4)assert(fvm_value_string(&value,"abc")==FVM_OK);
            assert(fvm_set_global(c,"input",&value)==FVM_OK);
            Bytes code;text(code,"sentinel");variable(code,"input");code.push_back(0x55);code.push_back(0x1d);run(c,code);
            assert(fvm_get_global(c,"sentinel",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        }
        Bytes empty;empty.push_back(0x55);text(empty,"empty_sentinel");empty.push_back(0x4d);empty.push_back(0x1d);run(c,empty);
        assert(fvm_get_global(c,"empty_sentinel",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        FVM_Object *target=0,*name_object=0;
        assert(fvm_object_create(c,&target)==FVM_OK);fvm_value_number(&value,1);
        assert(fvm_object_set(c,target,"a",&value)==FVM_OK);value.type=FVM_VALUE_OBJECT;value.object=target;
        assert(fvm_set_global(c,"target",&value)==FVM_OK && fvm_set_global(c,"17",&value)==FVM_OK);
        assert(fvm_object_create(c,&name_object)==FVM_OK);fvm_value_number(&value,17);
        assert(fvm_object_set(c,name_object,"alive",&value)==FVM_OK);attach(c,name_object,"toString",name_value,0);
        fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=name_object;
        assert(fvm_set_global(c,"input",&value)==FVM_OK);
        Bytes failed;variable(failed,"input");failed.push_back(0x46);failed.push_back(0);
        name_failure=true;
        assert(fvm_avm1_execute(c,failed.begin(),failed.size())==FVM_EXCEPTION);
        assert(fvm_exception_clear(c)==FVM_OK);name_failure=false;
        for(unsigned mode=0;mode<2;++mode)
        {
            if(mode){fvm_value_number(&value,17);assert(fvm_set_global(c,"input",&value)==FVM_OK);}
            Bytes code;text(code,"sentinel");variable(code,"input");code.push_back(0x46);
            text(code,"key");code.push_back(0x4d);code.push_back(0x1d);code.push_back(0x1d);run(c,code);
            assert(fvm_get_global(c,"key",&out)==FVM_OK && out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"a"));
            assert(fvm_get_global(c,"sentinel",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        }
        fvm_context_destroy(c);
    }
    assert(names==10);fvm_runtime_destroy(rt);return 0;
}
