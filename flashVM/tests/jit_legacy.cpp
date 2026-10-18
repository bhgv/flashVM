/* Legacy equality/less and logical AND/OR preserve versioned primitive semantics in native code. */
#define FVM_STRING_FIXTURE_ONLY
#include "jit_strings.cpp"
#include <limits>
#ifndef FVM_LEGACY_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    const unsigned char operations[]={0x0e,0x0f,0x10,0x11};
    for(unsigned version=0;version<=8;++version)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);
        FVM_Value values[9];fvm_value_number(values,0);fvm_value_number(values+1,1);
        fvm_value_number(values+2,std::numeric_limits<double>::quiet_NaN());fvm_value_string(values+3,"");
        fvm_value_string(values+4,"0");fvm_value_string(values+5,"2");fvm_value_undefined(values+6);
        fvm_value_undefined(values+7);values[7].type=FVM_VALUE_NULL;fvm_value_boolean(values+8,1);
        for(unsigned op=0;op<4;++op)
        {
            Program p=string_program(operations[op],2);
            for(unsigned a=0;a<9;++a)for(unsigned b=0;b<9;++b)
            {
                FVM_Value args[2]={values[a],values[b]};string_check(c,p,2,args);
            }
            Program missing;missing.op(operations[op]);missing.finish();string_check(c,missing,0,0);
        }
        FVM_Value args[2],expected;fvm_value_string(args,"1");fvm_value_string(args+1,"01");fvm_value_boolean(&expected,1);
        string_check(c,string_program(0x0e,2),2,args,&expected);
        fvm_value_string(args,"10");fvm_value_string(args+1,"2");fvm_value_boolean(&expected,0);
        string_check(c,string_program(0x0f,2),2,args,&expected);
        fvm_value_string(args,"0");fvm_value_boolean(args+1,1);fvm_value_boolean(&expected,!version || version>=7);
        string_check(c,string_program(0x10,2),2,args,&expected);
        fvm_value_boolean(args+1,0);string_check(c,string_program(0x11,2),2,args,&expected);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
#endif
