#include "swf_shape4_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned int kind=0;kind<3;++kind) for(unsigned int spread=0;spread<3;++spread)
    for(unsigned int interpolation=0;interpolation<2;++interpolation) {
        unsigned int type=kind==0?0x10:kind==1?0x12:0x13;
        Bytes source=shape4(type,spread,interpolation,-128); FVM_SWFShape decoded;
        assert(fvm_swf_read_shape(source.begin(),source.size(),83,decoded)==FVM_OK);
        assert(decoded.id==9 && decoded.paints.size()==1);
        assert(decoded.paints[0].gradient_spread==spread);
        assert(decoded.paints[0].gradient_interpolation==interpolation);
        FVM_Object *root=load(c,shape4_tags(source),1); tick(c,root);
        shape4_pixels(c,root,type,spread,interpolation,kind==2?-.5:0);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
