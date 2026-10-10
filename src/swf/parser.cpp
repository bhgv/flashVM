#include "flashvm/swf.h"
#include <cstring>
static unsigned int read16(const unsigned char *p) { return (unsigned int)p[0] | ((unsigned int)p[1]<<8); }
static unsigned long read32(const unsigned char *p) { return (unsigned long)p[0] | ((unsigned long)p[1]<<8) | ((unsigned long)p[2]<<16) | ((unsigned long)p[3]<<24); }
extern "C" FVM_Result fvm_swf_inspect(const void *data, size_t size, FVM_SWFInfo *info) {
    const unsigned char *p=(const unsigned char *)data;
    size_t pos, rect_size;
    unsigned long count=0, actions=0;
    int ended=0;
    if (!info || info->struct_size!=sizeof(*info) || !p || size<14) return FVM_INVALID_ARGUMENT;
    if (p[0]!='F' || p[1]!='W' || p[2]!='S' || p[3]<1 || p[3]>8) return FVM_UNSUPPORTED;
    if (read32(p+4)!=size) return FVM_INVALID_ARGUMENT;
    rect_size=(5+4*(p[8]>>3)+7)/8;
    if (rect_size>size-8 || size-8-rect_size<4) return FVM_INVALID_ARGUMENT;
    pos=8+rect_size;
    info->version=p[3]; info->declared_length=read32(p+4);
    info->frame_rate_fixed8=read16(p+pos); info->frame_count=read16(p+pos+2);
    pos+=4;
    while (pos<size) {
        unsigned int code; unsigned long len;
        if (size-pos<2) return FVM_INVALID_ARGUMENT;
        code=read16(p+pos)>>6; len=read16(p+pos)&63; pos+=2;
        if (len==63) { if (size-pos<4) return FVM_INVALID_ARGUMENT; len=read32(p+pos); pos+=4; }
        if (len>size-pos) return FVM_INVALID_ARGUMENT;
        ++count; if (code==12) ++actions;
        pos+=(size_t)len;
        if (code==0) { ended=1; break; }
    }
    if (!ended || pos!=size) return FVM_INVALID_ARGUMENT;
    info->tag_count=count; info->action_tag_count=actions;
    return FVM_OK;
}
