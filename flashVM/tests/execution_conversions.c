/* C89 conversion callers share loaded policies and allow output aliases. */
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;unsigned version;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(version=4;version<=8;++version){
        FVM_Context *c;FVM_Value value,out;FVM_Object *clip;
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        fvm_value_undefined(&value);assert(fvm_exec_to_number(c,0,&value,&out)==FVM_OK && out.type==FVM_VALUE_NUMBER);
        assert(version<7?out.number==0:out.number!=out.number);
        fvm_value_string(&value,"0");assert(fvm_exec_to_boolean(c,&value,&value)==FVM_OK && value.type==FVM_VALUE_BOOLEAN && value.boolean==(version>=7));
        fvm_value_string(&value,"12");assert(fvm_exec_to_number(c,0,&value,&value)==FVM_OK && value.number==12);
        fvm_value_number(&value,4294967295.9);assert(fvm_exec_to_number(c,1,&value,&value)==FVM_OK && value.number==-1);
        fvm_value_number(&value,-4294967297.9);assert(fvm_exec_to_number(c,1,&value,&value)==FVM_OK && value.number==-1);
        fvm_value_number(&value,17);assert(fvm_exec_type_of(c,&value,&value)==FVM_OK && !strcmp(value.string,"number"));
        fvm_value_undefined(&value);value.type=FVM_VALUE_NULL;assert(fvm_exec_type_of(c,&value,&out)==FVM_OK && !strcmp(out.string,"null"));
        assert(fvm_construct(c,"MovieClip",0,0,&clip)==FVM_OK);value.type=FVM_VALUE_OBJECT;value.object=clip;
        assert(fvm_exec_type_of(c,&value,&out)==FVM_OK && !strcmp(out.string,"movieclip"));
        fvm_value_number(&out,99);assert(fvm_exec_to_number(c,0,0,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        assert(fvm_exec_to_boolean(c,0,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        assert(fvm_exec_type_of(c,0,&out)==FVM_INVALID_ARGUMENT && out.number==99);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
