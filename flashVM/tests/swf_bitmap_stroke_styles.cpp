#include "swf_bitmap_stroke_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned kind=0x40;kind<=0x43;++kind) {
        size_t style=0,end=0; Bytes source=bs_shape(kind,80,true,false,10,7,&style,&end),bytes;
        for(size_t i=0;i<style;++i) bytes.push_back(source[i]); bytes[bytes.size()-1]=0;
        Bits records; records.put(0,4); records.put(0,4); records.put(0,1); records.put(16,5); records.bit=0;
        records.b.push_back(0); records.b.push_back(1); for(size_t i=style;i<end;++i) records.b.push_back(source[i]);
        records.put(0,4); records.put(1,4); records.put(0,1); records.put(9,5); records.put(16,5);
        records.put(40,16); records.put(160,16); records.put(1,1); edge(records,240,0); records.put(0,6); append(bytes,records.b);
        FVM_Object *root=load(c,bs_tags(bytes),1); tick(c,root); assert(fvm_gc_collect(c,0)==FVM_OK); bs_pixels(c,root,kind,80,true,false,10);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
