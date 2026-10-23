#include "swf_shape4_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned kind=0;kind<2;++kind) for(unsigned closed=0;closed<2;++closed) for(unsigned no_close=0;no_close<2;++no_close) {
        unsigned type=kind?0x13:0; Bytes bytes=shape4_stroke(type,1,1,-128,80,2,8,no_close?4:0,closed!=0); FVM_SWFShape out;
        assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,out)==FVM_OK);
        const FVM_SWFShapePaint &paint=out.paints[0]; assert(paint.no_close==(no_close!=0));
        bool has_close=false; for(size_t i=0;i<paint.commands.size();++i) if(paint.commands[i].verb==FVM_PATH_CLOSE) has_close=true;
        assert(has_close==(closed && !no_close));
        FVM_Object *root=load(c,shape4_tags(bytes),1); tick(c,root);
        stroke4_pixels(c,root,type,1,1,kind?-.5:0,80,2,closed!=0,no_close!=0);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
