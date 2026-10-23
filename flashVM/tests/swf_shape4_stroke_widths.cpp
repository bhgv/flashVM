#include "swf_shape4_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    const unsigned widths[]={0,1,20,80,65535};
    for(unsigned k=0;k<3;++k) for(unsigned i=0;i<5;++i) {
        unsigned kind=k==0?0:k==1?0x10:0x13; Bytes bytes=shape4_stroke(kind,2,1,128,widths[i]); FVM_SWFShape out;
        assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,out)==FVM_OK);
        assert(out.paints[0].width==widths[i]/20.0);
        FVM_Object *root=load(c,shape4_tags(bytes),1); tick(c,root);
        stroke4_pixels(c,root,kind,2,1,kind==0x13?.5:0,widths[i]);
    }
    /* Direct color and solid FillType are equivalent, including alpha. */
    FVM_Object *root=load(c,shape4_tags(shape4_stroke(0,0,0,0,80,2,0)),1); tick(c,root);
    stroke4_pixels(c,root,0,0,0,0);
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
