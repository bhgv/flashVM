/* FlashVM: endian-independent resource diagnostics with atomic malformed-record rejection. */
#include "fault_wire.hpp"
#include <cstring>

/* Write a bounded Uint32 diagnostic field in network byte order without host alignment assumptions.
 */
static void store_u32(unsigned char *data, unsigned long value)
{
    data[0] = (unsigned char)(value >> 24);
    data[1] = (unsigned char)(value >> 16);
    data[2] = (unsigned char)(value >> 8);
    data[3] = (unsigned char)value;
}

/* Read a Uint32 field without loading unaligned host integers. */
static unsigned long load_u32(const unsigned char *data)
{
    return ((unsigned long)data[0] << 24) | ((unsigned long)data[1] << 16) |
           ((unsigned long)data[2] << 8) | data[3];
}

/* Encode authority identifiers and bounded strings without serializing host structure padding. */
void fvm_fault_wire_encode(unsigned char *data, const FVM_ExceptionInfo &fault)
{
    std::memset(data, 0, FVM_FAULT_WIRE_SIZE);
    std::memcpy(data, "FFME\0\0\0\1", 8);
    store_u32(data + 8, fault.resource);
    store_u32(data + 12, fault.origin_thread);
    store_u32(data + 16, fault.flags & 7UL);
    std::memcpy(data + 20, fault.kind, 16);
    std::memcpy(data + 36, fault.path, 512);
    std::memcpy(data + 548, fault.operation, 32);
    std::memcpy(data + 580, fault.message, 128);
    std::memcpy(data + 708, fault.address, 512);
}

/* Merge a complete authority cause while retaining current caller location, thread and code. */
bool fvm_fault_wire_decode(FVM_ExceptionInfo *fault, const unsigned char *data, size_t size)
{
    if (!fault || fault->code < FVM_RESOURCE_CLOSED || fault->code > FVM_RESOURCE_FAILURE ||
        !data || size != FVM_FAULT_WIRE_SIZE || std::memcmp(data, "FFME\0\0\0\1", 8) ||
        load_u32(data + 16) > 7 || !std::memchr(data + 20, 0, 16) ||
        !std::memchr(data + 36, 0, 512) || !std::memchr(data + 548, 0, 32) ||
        !std::memchr(data + 580, 0, 128) || !std::memchr(data + 708, 0, 512) ||
        (std::strcmp((const char *)data + 20, "mutex") &&
         std::strcmp((const char *)data + 20, "channel")))
        return false;

    FVM_ExceptionInfo result = *fault;
    result.resource = load_u32(data + 8);
    result.origin_thread = load_u32(data + 12);
    result.flags |= load_u32(data + 16) | FVM_EXCEPTION_NETWORK | FVM_EXCEPTION_REMOTE_CONTEXT;
    std::memcpy(result.kind, data + 20, sizeof(result.kind));
    /* Preserve an existing carrier path when it already identifies the receiving operation. */
    if (!result.path[0])
        std::memcpy(result.path, data + 36, sizeof(result.path));
    if (data[548])
        std::memcpy(result.operation, data + 548, sizeof(result.operation));
    if (data[580])
        std::memcpy(result.message, data + 580, sizeof(result.message));
    /* A relayed error can identify an authority beyond the current transport endpoint. */
    if (data[708])
        std::memcpy(result.address, data + 708, sizeof(result.address));
    *fault = result;
    return true;
}
