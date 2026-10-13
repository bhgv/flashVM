/* FlashVM: bounded, allocation-free validation of neutral SWF document tags. */
#ifndef FVM_SWF_DOCUMENT_HPP
#define FVM_SWF_DOCUMENT_HPP /* Guard for shared document-tag validation. */
#include "flashvm/flashvm.h"
#include <cstring>
/* Borrowed root-document data remains valid while its canonical decoded bytes remain alive. */
struct FVM_SWFDocument
{
    bool attributes_present; /* Whether one first-position FileAttributes tag was read. */
    unsigned long attributes; /* Supported neutral flags; network/AVM2/hardware policies are gated. */
    bool metadata_present; /* Distinguish an empty metadata string from no Metadata tag. */
    const char *metadata; /* Borrowed UTF-8 bytes excluding the final terminator. */
    size_t metadata_size; /* Complete metadata byte count. */
    /* Start without any document tags or borrowed spans. */
    FVM_SWFDocument() : attributes_present(false), attributes(0), metadata_present(false),
                        metadata(0), metadata_size(0) {}
};
/* Validate UTF-8 scalar encoding without allocating or parsing XML. */
inline bool fvm_swf_metadata_utf8(const unsigned char *p, size_t size)
{
    for (size_t i = 0; i < size; )
    {
        unsigned int first = p[i++], count = 0;
        unsigned long code = 0, minimum = 0;
        if (first < 0x80)
            continue;
        if (first >= 0xc2 && first <= 0xdf) { count = 1; code = first & 31; minimum = 0x80; }
        else if (first >= 0xe0 && first <= 0xef) { count = 2; code = first & 15; minimum = 0x800; }
        else if (first >= 0xf0 && first <= 0xf4) { count = 3; code = first & 7; minimum = 0x10000; }
        else return false;
        if (count > size - i)
            return false;
        while (count--)
        {
            unsigned int next = p[i++];
            if ((next & 0xc0) != 0x80)
                return false;
            code = (code << 6) | (next & 63);
        }
        if (code < minimum || code > 0x10ffffUL || (code >= 0xd800 && code <= 0xdfff))
            return false;
    }
    return true;
}
/* Admit only neutral attributes and exactly one terminated, scalar UTF-8 metadata payload. */
inline FVM_Result fvm_swf_document_tag(unsigned int tag, const unsigned char *p, size_t size,
                                       bool first, FVM_SWFDocument &document)
{
    if (tag == 69)
    {
        if (!first || document.attributes_present || size != 4)
            return FVM_INVALID_ARGUMENT;
        unsigned long flags = (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
                              ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
        if (flags & ~0x79UL)
            return FVM_INVALID_ARGUMENT;
        if (flags & ~0x10UL)
            return FVM_UNSUPPORTED;
        document.attributes_present = true;
        document.attributes = flags;
    }
    else if (tag == 77)
    {
        if (!document.attributes_present || !(document.attributes & 0x10) ||
            document.metadata_present || !size ||
            std::memchr(p, 0, size) != p + size - 1 || !fvm_swf_metadata_utf8(p, size - 1))
            return FVM_INVALID_ARGUMENT;
        document.metadata_present = true;
        document.metadata = (const char *)p;
        document.metadata_size = size - 1;
    }
    return FVM_OK;
}
/* Require the declared metadata tag before successful completion of the root stream. */
inline FVM_Result fvm_swf_document_finish(const FVM_SWFDocument &document)
{
    return ((document.attributes & 0x10) != 0) == document.metadata_present
               ? FVM_OK : FVM_INVALID_ARGUMENT;
}
#endif
