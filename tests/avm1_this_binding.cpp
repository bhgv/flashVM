/* Implicit this precedes local/formal names in loaded SWF5+ calls. */
#define main named_scope_fixture_main
#include "avm1_function2_named_scope.cpp"
#undef main
static FVM_Object *function(FVM_Context *c,bool function2,bool formal,bool local)
{
    Bytes body;
    if(local) { text(body,"this");text(body,"local-value");body.push_back(0x3c); }
    variable(body,"this");body.push_back(0x3e);body.push_back(0);
    FVM_Object *fn=0;
    const char *parameters[]={"this"};
    if(!function2) {
        assert(fvm_function_create_avm1(c,body.begin(),body.size(),formal?1:0,parameters,0,&fn)==FVM_OK);
        return fn;
    }
    Bytes header;name(header,"");u16(header,formal?1:0);header.push_back(0);u16(header,0);
    if(formal) { header.push_back(0);name(header,"this"); }
    u16(header,(unsigned)body.size());
    Bytes outer;outer.push_back(0x8e);u16(outer,(unsigned)header.size());
    for(size_t i=0;i<header.size();++i)outer.push_back(header[i]);
    for(size_t i=0;i<body.size();++i)outer.push_back(body[i]);
    outer.push_back(0x3e);outer.push_back(0);
    FVM_Object *factory=0;assert(fvm_function_create_avm1(c,outer.begin(),outer.size(),0,0,0,&factory)==FVM_OK);
    FVM_Value out;assert(fvm_function_invoke(c,factory,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);
    return out.object;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=9;++version) {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        if(version<=8) {
            unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
            assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        }
        assert(fvm_standard_install(c)==FVM_OK);
        FVM_Object *self=0;assert(fvm_construct(c,"Object",0,0,&self)==FVM_OK);assert(fvm_gc_pin(c,self)==FVM_OK);
        for(unsigned kind=0;kind<2;++kind)for(unsigned formal=0;formal<2;++formal)for(unsigned local=0;local<2;++local) {
            FVM_Object *fn=function(c,kind!=0,formal!=0,local!=0);assert(fvm_gc_pin(c,fn)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);
            FVM_Value arg,out;fvm_value_string(&arg,"formal-value");assert(fvm_function_invoke(c,fn,self,1,&arg,&out)==FVM_OK);
            if((version>=5 && version<=8) || (!formal && !local))assert(out.type==FVM_VALUE_OBJECT && out.object==self);
            else assert(out.type==FVM_VALUE_STRING && !std::strcmp(out.string,local?"local-value":"formal-value"));
            assert(fvm_gc_unpin(c,fn)==FVM_OK);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
