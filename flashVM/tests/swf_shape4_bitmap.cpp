#define FVM_BITMAP_SWF_FIXTURE_ONLY
#include "swf_bitmap_fill.cpp"
static Bytes extend_shape(const Bytes &legacy)
{
    Bytes b; for(size_t i=0;i<11;++i) b.push_back(legacy[i]);
    for(size_t i=2;i<11;++i) b.push_back(legacy[i]); b.push_back(0);
    for(size_t i=11;i<legacy.size();++i) b.push_back(legacy[i]); return b;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned int kind=0x40;kind<=0x43;++kind) {
        Bytes tags,empty,payload=extend_shape(bitmap_shape(kind));
        tag(tags,20,lossless(5,2,1,bitmap_raw())); tag(tags,83,payload); tag(tags,26,place(6,1,9,0,0));
        tag(tags,1,empty); tag(tags,0,empty); FVM_Object *root=load(c,tags,1); tick(c,root);
        assert(fvm_gc_collect(c,0)==FVM_OK); bitmap_pixels(c,root,kind);
        tags.clear(); tag(tags,83,payload); tag(tags,1,empty); tag(tags,0,empty);
        Bytes bytes=movie(tags,1); root=0;
        assert(fvm_movieclip_load_swf(c,bytes.begin(),bytes.size(),"root",&root)==FVM_INVALID_ARGUMENT && !root);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
