#include "swf_shape4_stroke_fixture.hpp"
int main()
{
    size_t style=0,flags=0,lines=0; Bytes source=shape4_stroke(0x13,2,1,-128,80,2,8,4,false,&style);
    Bytes ramp=shape4(0x13,2,1,-128,2,0,&flags,&lines),bytes;
    for(size_t i=0;i<style;++i) bytes.push_back(source[i]); bytes[bytes.size()-1]=0;
    Bits records; records.put(0,4); records.put(0,4); records.put(0,1); records.put(16,5); records.bit=0;
    records.b.push_back(0); records.b.push_back(1);
    for(size_t i=style;i<style+4+lines-flags-2;++i) records.b.push_back(source[i]);
    records.put(0,4); records.put(1,4); records.put(0,1); records.put(9,5); records.put(16,5);
    records.put(40,16); records.put(160,16); records.put(1,1); edge(records,240,0); records.put(0,6);
    append(bytes,records.b); FVM_SWFShape out;
    assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,out)==FVM_OK);
    assert(out.paints.size()==1 && out.paints[0].stroke && out.paints[0].no_close && out.paints[0].gradient_focal==-.5);
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *root=load(c,shape4_tags(bytes),1); tick(c,root); assert(fvm_gc_collect(c,0)==FVM_OK);
    stroke4_pixels(c,root,0x13,2,1,-.5);
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
