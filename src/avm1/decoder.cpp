#include "flashvm/avm1.h"
extern "C" FVM_Result fvm_avm1_inspect(const void *data, size_t size, FVM_AVM1Info *info) {
    const unsigned char *p=(const unsigned char *)data;
    size_t pos=0;
    unsigned long count=0,push=0,branch=0;
    int ended=0;
    if (!info || info->struct_size!=sizeof(*info) || (!data && size)) return FVM_INVALID_ARGUMENT;
    while (pos<size) {
        unsigned int op=p[pos++];
        ++count;
        if (op==0) { ended=1; break; }
        if (op>=0x80) {
            size_t len;
            if (size-pos<2) return FVM_INVALID_ARGUMENT;
            len=(size_t)p[pos] | ((size_t)p[pos+1]<<8);
            pos+=2;
            if (len>size-pos) return FVM_INVALID_ARGUMENT;
            if (op==0x96) ++push;
            if (op==0x99 || op==0x9d) { if (len!=2) return FVM_INVALID_ARGUMENT; ++branch; }
            pos+=len;
        }
    }
    if (!ended || pos!=size) return FVM_INVALID_ARGUMENT;
    info->instruction_count=count;
    info->push_count=push;
    info->branch_count=branch;
    return FVM_OK;
}
