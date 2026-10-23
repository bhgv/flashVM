#include "swf_stroke_bounds_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    Bytes tags,empty,a=caps_shape(3,1,1,80,false,false),b=caps_shape(2,2,1,80,false,false); b[0]=10;
    tag(tags,36,lossless(5,2,2,bs_raw()));
    tag(tags,83,a); tag(tags,83,b); tag(tags,26,place(6,1,9,0,0)); tag(tags,1,empty);
    tag(tags,26,place(3,1,10,0,0)); tag(tags,1,empty); tag(tags,0,empty);
    FVM_Object *root=load(c,tags,2); tick(c,root); FVM_Object *node=display_node(root);
    selected_bounds(c,root,1,1); tick(c,root); assert(display_node(root)==node);
    selected_bounds(c,root,2,1);
    for(unsigned int i=0;i<16;++i) {
        unsigned int frame=i%2+1; seek(c,root,frame); assert(fvm_gc_collect(c,0)==FVM_OK);
        selected_bounds(c,root,frame==1?1:2,1);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
