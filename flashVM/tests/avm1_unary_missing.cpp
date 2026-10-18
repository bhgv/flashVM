/* Missing unary operands become undefined only for loaded SWFs; raw oracle values stay independent. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
static void check(FVM_Context *c,unsigned opcode,unsigned version)
{
    Bytes code;code.push_back((unsigned char)opcode);text(code,"result");
    code.push_back(0x4d);code.push_back(0x1d);code.push_back(0);
    assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK);
    FVM_Value out;assert(fvm_get_global(c,"result",&out)==FVM_OK);
    if(opcode==0x12)assert(out.type==FVM_VALUE_BOOLEAN && out.boolean);
    else if(opcode==0x44)assert(out.type==FVM_VALUE_STRING && std::strcmp(out.string,"undefined")==0);
    else if(opcode==0x4b)assert(out.type==FVM_VALUE_STRING && std::strcmp(out.string,version<7?"":"undefined")==0);
    else if(opcode==0x33 || opcode==0x37)assert(out.type==FVM_VALUE_STRING && std::strcmp(out.string,"")==0);
    else
    {
        assert(out.type==FVM_VALUE_NUMBER);
        if((opcode==0x4a || opcode==0x50 || opcode==0x51) && version>=7)assert(out.number!=out.number);
        else if(opcode==0x50)assert(out.number==1);
        else if(opcode==0x51)assert(out.number==-1);
        else if(opcode==0x14 || opcode==0x31)assert(out.number==(version<7?0:9));
        else if(opcode==0x32 || opcode==0x36)assert(out.number==(version<7?0:117));
        else assert(out.number==0);
    }
}
int main()
{
    const unsigned ops[]={0x12,0x14,0x18,0x31,0x32,0x33,0x36,0x37,0x44,0x4a,0x4b,0x50,0x51};
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=0;mode<6;++mode)
    {
        FVM_Context *c=0;unsigned version=mode+3;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);
        if(mode)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);
        for(unsigned i=0;i<sizeof(ops)/sizeof(ops[0]);++i)
        {
            if(mode)check(c,ops[i],version);
            else {unsigned char code[2]={(unsigned char)ops[i],0};assert(fvm_avm1_execute(c,code,sizeof(code))==FVM_ERROR);}
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
