/* FlashVM: SWF file parsing and model access: parser.
 * File: src/swf/parser.cpp.
 */

#include "format_constants.h"
#include "decode.hpp"
#include "document.hpp"
#include <cstring>
#include <climits>
#include "zlib.h"

namespace
{


/* Compute and return read16. */
unsigned int read16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}


/* Compute and return read32. */
unsigned long read32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) | ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}


/* Compute and return inspect fws. */
FVM_Result inspect_fws(const unsigned char *p, size_t size, FVM_SWFInfo *info)
{
    size_t rect_size = (5 + 4 * (p[8] >> 3) + 7) / 8;
    if ((p[8] >> 3) == 0 || rect_size > size - 8 || size - 8 - rect_size < 4)
    {
        return FVM_INVALID_ARGUMENT;
    }

    FVM_SWFInfo parsed;

    /* Computation / Вычисление и обновление состояния. */
    std::memset(&parsed, 0, sizeof(parsed));
    parsed.struct_size = sizeof(parsed);
    parsed.version = p[3];
    parsed.declared_length = read32(p + 4);
    size_t pos = 8 + rect_size;

    /* Computation / Вычисление и обновление состояния. */
    parsed.frame_rate_fixed8 = read16(p + pos);
    parsed.frame_count = read16(p + pos + 2);
    pos += 4;

    FVM_SWFDocument document;
    while (pos < size)
    {
        if (size - pos < 2)
        {
            return FVM_INVALID_ARGUMENT;
        }
        unsigned int header = read16(p + pos);
        unsigned int code = header >> FVM_SWF_TAG_CODE_SHIFT;
        unsigned long length = header & FVM_SWF_TAG_LENGTH_MASK;
        pos += 2;
        if (length == FVM_SWF_TAG_LENGTH_MASK)
        {
            if (size - pos < 4)
            {
                return FVM_INVALID_ARGUMENT;
            }
            length = read32(p + pos);
            pos += 4;
        }
        if (length > size - pos)
        {
            return FVM_INVALID_ARGUMENT;
        }
        FVM_Result document_status =
            fvm_swf_document_tag(code, p + pos, (size_t)length, parsed.tag_count == 0, document);
        if (document_status != FVM_OK)
        {
            return document_status;
        }
        ++parsed.tag_count;
        if (code == FVM_SWF_TAG_DO_ACTION)
        {
            ++parsed.action_tag_count;
        }
        pos += (size_t)length;
        if (code == FVM_SWF_TAG_END)
        { /* End is a zero-length tag and must terminate the file. */
            if (length != 0 || pos != size)
            {
                return FVM_INVALID_ARGUMENT;
            }
            FVM_Result document_status = fvm_swf_document_finish(document);
            if (document_status != FVM_OK)
            {
                return document_status;
            }
            *info = parsed;
            return FVM_OK;
        }
        if (code == FVM_SWF_TAG_SHOW_FRAME && length != 0)
        {
            return FVM_INVALID_ARGUMENT;
        }
    }
    return FVM_INVALID_ARGUMENT;
}
} // namespace


/* Decode swf. */
FVM_Result
fvm_swf_decode(const void *data, size_t size, size_t limit, fvm_detail::vector<unsigned char> &bytes, FVM_SWFInfo *info)
{
    const unsigned char *p = (const unsigned char *)data;
    if (!info || info->struct_size != sizeof(*info) || !p || size < 8)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (p[1] != 'W' || p[2] != 'S' || (p[0] != 'F' && p[0] != 'C') || p[3] < 1 || p[3] > 8)
    {
        return FVM_UNSUPPORTED;
    }
    if (p[0] == 'C' && p[3] < 6)
    {
        return FVM_INVALID_ARGUMENT;
    }

    unsigned long declared = read32(p + 4);
    if (declared < 14)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (declared > limit || declared > (size_t)-1)
    {
        return FVM_OUT_OF_MEMORY;
    }
    if (p[0] == 'F' && declared != size)
    {
        return FVM_INVALID_ARGUMENT;
    }

    try
    {
        fvm_detail::vector<unsigned char> decoded;
        decoded.resize((size_t)declared);
        if (p[0] == 'F')
        {
            std::memcpy(&decoded[0], p, size);
        }
        else
        {
            /* A bounded single zlib stream: reject truncation, extra streams,
             * trailing garbage and disagreement with SWF FileLength. */
            if (size - 8 > UINT_MAX || declared - 8 > UINT_MAX)
            {
                return FVM_UNSUPPORTED;
            }
            std::memcpy(&decoded[0], p, 8);
            decoded[0] = 'F';
            z_stream stream;
            std::memset(&stream, 0, sizeof(stream));
            stream.next_in = (Bytef *)(p + 8);
            stream.avail_in = (uInt)(size - 8);
            stream.next_out = &decoded[8];
            stream.avail_out = (uInt)(declared - 8);
            int status = inflateInit(&stream);
            if (status != Z_OK)
            {
                return status == Z_MEM_ERROR ? FVM_OUT_OF_MEMORY : FVM_ERROR;
            }
            status = inflate(&stream, Z_FINISH);
            bool complete = status == Z_STREAM_END && stream.avail_in == 0 && stream.total_out == declared - 8;
            inflateEnd(&stream);
            if (!complete)
            {
                return status == Z_MEM_ERROR ? FVM_OUT_OF_MEMORY : FVM_INVALID_ARGUMENT;
            }
        }
        FVM_Result result = inspect_fws(&decoded[0], decoded.size(), info);
        if (result == FVM_OK)
        {
            bytes.swap(decoded);
        }
        return result;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}


/* Compute and return swf inspect. */
extern "C" FVM_Result fvm_swf_inspect(const void *data, size_t size, FVM_SWFInfo *info)
{
    fvm_detail::vector<unsigned char> bytes;
    return fvm_swf_decode(data, size, FVM_SWF_DEFAULT_LIMIT, bytes, info);
}
