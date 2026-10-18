/* Loaded SWF 5/6 use numeric string truth; SWF 7/8 use nonempty string truth. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
static FVM_Value logic(FVM_Context *c, unsigned op, const FVM_Value &value)
{
    assert(fvm_set_global(c,"input",&value)==FVM_OK);
    Bytes b;global(b,"input");
    if(op!=0x12) integer(b,1);
    b.push_back((unsigned char)op);
    return run(c,b);
}
static bool branch(FVM_Context *c, const FVM_Value &value)
{
    assert(fvm_set_global(c,"input",&value)==FVM_OK);
    Bytes b;integer(b,0);global(b,"input");
    b.push_back(0x9d);b.push_back(2);b.push_back(0);b.push_back(5);b.push_back(0);
    b.push_back(0x99);b.push_back(2);b.push_back(0);b.push_back(9);b.push_back(0);
    b.push_back(0x17);integer(b,1);
    return run(c,b).number==1;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    const char *samples[]={"", "0", "-0", "1", "-2.5", "word", "NaN", " 0 ", "Infinity"};
    for(unsigned version=5;version<=8;++version)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',0,16,0,0,0,0x08,0,0,12,1,0,0,0};
        swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);
        for(unsigned i=0;i<sizeof(samples)/sizeof(*samples);++i)
        {
            FVM_Value value;fvm_value_string(&value,samples[i]);
            bool expected=version>=7 ? i!=0 : i==3 || i==4;
            assert(logic(c,0x12,value).boolean==!expected);
            assert(logic(c,0x10,value).boolean==expected);
            assert(logic(c,0x11,value).boolean);
            assert(branch(c,value)==expected);
        }
        FVM_Value value;assert(fvm_value_string_bytes(&value,"1\0x",3)==FVM_OK);
        assert(logic(c,0x12,value).boolean==(version<7));
        assert(branch(c,value)==(version>=7));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
