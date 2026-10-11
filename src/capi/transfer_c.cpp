#include "../core/runtime.hpp"
#include "../core/channel_packet.h"
#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include <new>
#include <cstring>
namespace
{
struct Packet
{
    unsigned char *data;
    size_t size;
    Packet() : data(0), size(0)
    {
    }
    ~Packet()
    {
        fvm_serialized_free(data);
    }
};
void release(void *packet)
{
    delete (Packet *)packet;
}
} // namespace
extern "C" FVM_Result fvm_channel_send_value(FVM_Context *context, FVM_Channel *channel,
                                             const FVM_Value *value, int blocking)
{
    int file = 0;
    if (!context || !channel || !value || context->destroying)
        return FVM_INVALID_ARGUMENT;
    if (fvm_validate_value(context, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    fvm_channel_is_file(channel, &file);
    if (!file && value->type != FVM_VALUE_OBJECT && !value->metatable)
        return fvm_channel_send_primitive(channel, value, blocking);
    Packet *packet = new (std::nothrow) Packet;
    if (!packet)
        return FVM_OUT_OF_MEMORY;
    FVM_Result result = fvm_value_serialize(context, value, &packet->data, &packet->size);
    if (result == FVM_OK)
    {
        if (file)
            result = fvm_channel_file_send_record(channel, packet->data, packet->size);
        else
            result = fvm_channel_packet_send(channel, packet, release, blocking);
    }
    if (file || result != FVM_OK)
        delete packet;
    return result;
}
extern "C" FVM_Result fvm_channel_receive_value(FVM_Context *context, FVM_Channel *channel,
                                                FVM_Value *out, int blocking)
{
    int file = 0;
    void *raw = 0;
    FVM_Value value;
    FVM_Result result;
    if (!context || !channel || !out || context->destroying)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    fvm_channel_is_file(channel, &file);
    if (file)
    {
        unsigned char *data = 0;
        size_t size = 0;
        result = fvm_channel_file_receive_record(channel, &data, &size, blocking);
        if (result == FVM_OK)
            result = fvm_value_deserialize(context, data, size, out);
        fvm_serialized_free(data);
        return result;
    }
    result = fvm_channel_receive_mixed(channel, &value, &raw, blocking);
    if (result != FVM_OK)
        return result;
    if (raw)
    {
        Packet *packet = (Packet *)raw;
        result = fvm_value_deserialize(context, packet->data, packet->size, out);
        release(packet);
        return result;
    }
    try
    {
        *out = value;
        if (value.type == FVM_VALUE_STRING)
        {
            size_t length = std::strlen(value.string);
            char *copy = new (std::nothrow) char[length + 1];
            if (!copy)
                result = FVM_OUT_OF_MEMORY;
            else
            {
                std::memcpy(copy, value.string, length + 1);
                try
                {
                    context->strings.push_back(copy);
                }
                catch (...)
                {
                    delete[] copy;
                    throw;
                }
                out->string = copy;
            }
        }
    }
    catch (...)
    {
        result = FVM_OUT_OF_MEMORY;
    }
    fvm_channel_message_release(&value);
    if (result != FVM_OK)
        fvm_value_undefined(out);
    return result;
}
