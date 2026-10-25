/* FlashVM: bounded, allocation-free validation of neutral SWF document tags. */
#ifndef FVM_SWF_DOCUMENT_HPP
#define FVM_SWF_DOCUMENT_HPP /* Guard for shared document-tag validation. */
#include "flashvm/flashvm.h"
#include "format_constants.h"
#include "../core/unicode_constants.h"
#include <cstring>


/* Borrowed root-document data remains valid while its canonical decoded bytes remain alive. */
struct FVM_SWFDocument
{
    bool attributes_present;       /* Whether one first-position FileAttributes tag was read. */
    unsigned long attributes;      /* Supported neutral flags; network/AVM2/hardware policies are gated. */
    bool metadata_present;         /* Distinguish an empty metadata string from no Metadata tag. */
    const char *metadata;          /* Borrowed UTF-8 bytes excluding the final terminator. */
    size_t metadata_size;          /* Complete metadata byte count. */
    unsigned int debugger_tag;     /* Last authoring debugger tag; zero means absent. */
    const char *debugger_password; /* Borrowed password/hash bytes, never an external debugger command. */
    size_t debugger_password_size; /* Count excluding its final NUL. */
    const unsigned char *debug_id; /* Borrowed opaque DebugID, or null when absent. */


    /* Start without any document tags or borrowed spans. */
    FVM_SWFDocument()
        : attributes_present(false), attributes(0), metadata_present(false), metadata(0), metadata_size(0),
          debugger_tag(0), debugger_password(0), debugger_password_size(0), debug_id(0)
    {
    }
};


/* Validate UTF-8 scalar encoding without allocating or parsing XML. */
inline bool fvm_swf_metadata_utf8(const unsigned char *p, size_t size)
{
    for (size_t i = 0; i < size;)
    {
        unsigned int first = p[i++], count = 0;
        unsigned long code = 0, minimum = 0;
        if (first < FVM_UTF8_ASCII_LIMIT)
        {
            continue;
        }
        if (first >= FVM_UTF8_TWO_BYTE_LEAD_MIN && first <= FVM_UTF8_TWO_BYTE_LEAD_MAX)
        {
            count = 1;
            code = first & FVM_UTF8_TWO_BYTE_PAYLOAD_MASK;
            minimum = FVM_UTF8_TWO_BYTE_SCALAR_MIN;
        }
        else if (first >= FVM_UTF8_THREE_BYTE_LEAD_MIN && first <= FVM_UTF8_THREE_BYTE_LEAD_MAX)
        {
            count = 2;
            code = first & FVM_UTF8_THREE_BYTE_PAYLOAD_MASK;
            minimum = FVM_UTF8_THREE_BYTE_SCALAR_MIN;
        }
        else if (first >= FVM_UTF8_FOUR_BYTE_LEAD_MIN && first <= FVM_UTF8_FOUR_BYTE_LEAD_MAX)
        {
            count = 3;
            code = first & FVM_UTF8_FOUR_BYTE_PAYLOAD_MASK;
            minimum = FVM_UTF8_FOUR_BYTE_SCALAR_MIN;
        }
        else
        {
            return false;
        }
        if (count > size - i)
        {
            return false;
        }
        while (count--)
        {
            unsigned int next = p[i++];
            if ((next & FVM_UTF8_CONTINUATION_CLASS_MASK) != FVM_UTF8_CONTINUATION_MARKER)
            {
                return false;
            }
            code = (code << FVM_UTF8_CONTINUATION_PAYLOAD_BITS) | (next & FVM_UTF8_CONTINUATION_PAYLOAD_MASK);
        }
        if (code < minimum || code > FVM_UNICODE_SCALAR_MAX ||
            (code >= FVM_UNICODE_SURROGATE_MIN && code <= FVM_UNICODE_SURROGATE_MAX))
        {
            return false;
        }
    }
    return true;
}


/* Admit only neutral attributes and exactly one terminated, scalar UTF-8 metadata payload. */
inline FVM_Result
fvm_swf_document_tag(unsigned int tag, const unsigned char *p, size_t size, bool first, FVM_SWFDocument &document)
{
    if (tag == FVM_SWF_TAG_FILE_ATTRIBUTES)
    {
        if (!first || document.attributes_present || size != FVM_SWF_FILE_ATTRIBUTES_BYTES)
        {
            return FVM_INVALID_ARGUMENT;
        }
        unsigned long flags = (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) |
                              ((unsigned long)p[3] << 24);
        if (flags & ~FVM_SWF_FILE_ATTRIBUTES_VALID_FLAGS)
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (flags & ~FVM_SWF_FILE_ATTRIBUTES_METADATA)
        {
            return FVM_UNSUPPORTED;
        }
        document.attributes_present = true;
        document.attributes = flags;
    }
    else if (tag == FVM_SWF_TAG_METADATA)
    {
        if (!document.attributes_present || !(document.attributes & FVM_SWF_FILE_ATTRIBUTES_METADATA) ||
            document.metadata_present || !size || std::memchr(p, 0, size) != p + size - 1 ||
            !fvm_swf_metadata_utf8(p, size - 1))
        {
            return FVM_INVALID_ARGUMENT;
        }
        document.metadata_present = true;
        document.metadata = (const char *)p;
        document.metadata_size = size - 1;
    }
    else if (tag == FVM_SWF_TAG_ENABLE_DEBUGGER || tag == FVM_SWF_TAG_ENABLE_DEBUGGER2)
    {
        size_t prefix = tag == FVM_SWF_TAG_ENABLE_DEBUGGER2 ? FVM_SWF_DEBUGGER2_PREFIX_BYTES : 0;
        if (size <= prefix || std::memchr(p + prefix, 0, size - prefix) != p + size - 1)
        {
            return FVM_INVALID_ARGUMENT;
        }
        document.debugger_tag = tag;
        document.debugger_password = (const char *)(p + prefix);
        document.debugger_password_size = size - prefix - 1;
    }
    else if (tag == FVM_SWF_TAG_DEBUG_ID)
    {
        if (size != FVM_SWF_DEBUG_ID_BYTES)
        {
            return FVM_INVALID_ARGUMENT;
        }
        document.debug_id = p;
    }
    return FVM_OK;
}


/* Require the declared metadata tag before successful completion of the root stream. */
inline FVM_Result fvm_swf_document_finish(const FVM_SWFDocument &document)
{
    return ((document.attributes & FVM_SWF_FILE_ATTRIBUTES_METADATA) != 0) == document.metadata_present
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}
#endif
