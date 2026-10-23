#include "swf_shape4_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned kind=0;kind<4;++kind) for(unsigned spread=0;spread<3;++spread) for(unsigned interpolation=0;interpolation<2;++interpolation) {
        unsigned type=kind==0?0:kind==1?0x10:kind==2?0x12:0x13;
        Bytes bytes=shape4_stroke(type,spread,interpolation,-128); FVM_SWFShape out;
        assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,out)==FVM_OK);
        assert(out.paints.size()==1 && out.paints[0].stroke && out.paints[0].width==4);
        FVM_Object *root=load(c,shape4_tags(bytes),1); tick(c,root);
        stroke4_pixels(c,root,type,spread,interpolation,type==0x13?-.5:0);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
