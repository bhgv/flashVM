#include "swf_bitmap_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned kind=0x40;kind<=0x43;++kind) for(unsigned closed=0;closed<2;++closed) for(unsigned no_close=0;no_close<2;++no_close) {
        Bytes bytes=bs_shape(kind,80,no_close!=0,closed!=0); FVM_SWFShape out;
        assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,out)==FVM_OK);
        bool has_close=false; for(size_t i=0;i<out.paints[0].commands.size();++i) if(out.paints[0].commands[i].verb==FVM_PATH_CLOSE) has_close=true;
        assert(has_close==(closed && !no_close)); FVM_Object *root=load(c,bs_tags(bytes),1); tick(c,root);
        bs_pixels(c,root,kind,80,no_close!=0,closed!=0);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
