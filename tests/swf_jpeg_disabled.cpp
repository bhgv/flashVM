#include "../src/capi/swf_jpeg_internal.h"
#include <cassert>
int main()
{
    const unsigned char jpeg[]={255,216,255,217};unsigned width=99,height=99;unsigned char *pixels=(unsigned char *)1;
    assert(fvm_swf_decode_jpeg(0,0,1024,&width,&height,&pixels)==FVM_INVALID_ARGUMENT);assert(!width && !height && !pixels);
#ifndef FVM_HAS_IMAGE_CODECS
    assert(fvm_swf_decode_jpeg(jpeg,sizeof(jpeg),1024,&width,&height,&pixels)==FVM_UNSUPPORTED);assert(!width && !height && !pixels);
#else
    (void)jpeg;
#endif
    return 0;
}
