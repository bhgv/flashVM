#include "swf_shape4_fixture.hpp"
int main()
{
    /* Append a second RGBA style bank at an aligned StateNewStyles boundary. */
    size_t line=0; Bytes prefix=shape4(0x10,0,0,0,2,0,0,&line); Bytes bytes;
    for(size_t i=0;i<=line;++i) bytes.push_back(prefix[i]);
    Bits records; records.put(1,4); records.put(0,4);
    records.put(0,1); records.put(16,5); records.bit=0;
    records.b.push_back(1); records.b.push_back(0); records.b.push_back(0);
    records.b.push_back(255); records.b.push_back(0); records.b.push_back(128); records.b.push_back(0);
    records.put(1,4); records.put(0,4); records.put(0,1); records.put(3,5); records.put(16,5);
    records.put(0,16); records.put(0,16); records.put(1,1);
    edge(records,320,0); edge(records,0,320); edge(records,-320,0); edge(records,0,-320);
    records.put(0,6); append(bytes,records.b); FVM_SWFShape decoded;
    assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,decoded)==FVM_OK);
    assert(decoded.paints.size()==1 && decoded.paints[0].rgb==0x00ff00 && decoded.paints[0].alpha>50);
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *root=load(c,shape4_tags(bytes),1); tick(c,root);
    unsigned char pixels[1024]; std::memset(pixels,0,sizeof(pixels)); FVM_RasterSurface surface={pixels,16,16,64};
    FVM_Result r=fvm_movieclip_render(c,root,&surface);
    if(r==FVM_OK) { const unsigned char *p=pixels+4*(8*16+8); assert(p[0]==0 && p[1]==255 && p[2]==0 && p[3]==128); }
    else assert(r==FVM_UNSUPPORTED);
    /* A malformed newly admitted line record must still fail before publication. */
    bytes[line+9]=1;
    assert(fvm_swf_read_shape(bytes.begin(),bytes.size(),83,decoded)==FVM_INVALID_ARGUMENT);
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
