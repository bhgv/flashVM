/* Encode arbitrary Dis bytes for the real FlashVM counted-string channel ABI. */
#include "disvm/flashvm_host.h"
#include <stdlib.h>
#include <string.h>


/* Decode a single ASCII hex digit without locale-sensitive classification. */
static int hex_digit(unsigned char value)
{
    if (value >= '0' && value <= '9')
    {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f')
    {
        return value - 'a' + 10;
    }
    if (value >= 'A' && value <= 'F')
    {
        return value - 'A' + 10;
    }
    return -1;
}


/* Validate framing/quota before allocating the decoded native buffer. */
static DVM_ImageResult decode_message(const char *name, const FVM_Value *message, size_t quota, DVM_ModuleImage *out)
{
    size_t length;
    size_t size;
    size_t index;
    unsigned char *decoded;
    DVM_ImageResult result;
    if (message->type != FVM_VALUE_STRING || message->metatable || !message->string)
    {
        return DVM_IMAGE_INVALID;
    }
    length = fvm_value_string_size(message);
    if (length <= DVM_MODULE_CHANNEL_PREFIX_SIZE ||
        memcmp(message->string, DVM_MODULE_CHANNEL_PREFIX, DVM_MODULE_CHANNEL_PREFIX_SIZE))
    {
        return DVM_IMAGE_INVALID;
    }
    length -= DVM_MODULE_CHANNEL_PREFIX_SIZE;
    if (length % 2)
    {
        return DVM_IMAGE_INVALID;
    }
    size = length / 2;
    if (size > quota || size > DVM_MODULE_IMAGE_MAX)
    {
        return DVM_IMAGE_LIMIT;
    }
    decoded = (unsigned char *)malloc(size);
    if (!decoded)
    {
        return DVM_IMAGE_NOMEM;
    }
    for (index = 0; index < size; index++)
    {
        int high = hex_digit((unsigned char)message->string[DVM_MODULE_CHANNEL_PREFIX_SIZE + index * 2]);
        int low = hex_digit((unsigned char)message->string[DVM_MODULE_CHANNEL_PREFIX_SIZE + index * 2 + 1]);
        if (high < 0 || low < 0)
        {
            free(decoded);
            return DVM_IMAGE_INVALID;
        }
        decoded[index] = (unsigned char)((high << 4) | low);
    }
    result = dvm_image_copy(name, decoded, size, quota, out);
    free(decoded);
    return result;
}


/* Receiving consumes one message and closes channel ownership on every path.
 * [GC:] Only native owned strings cross this boundary; VM objects are rejected. */
DVM_ImageResult
dvm_image_channel(const char *name, FVM_Channel *channel, int blocking, size_t quota, DVM_ModuleImage *out)
{
    FVM_Value message;
    FVM_Result status;
    DVM_ImageResult result;
    if (!name || !*name || !channel || !out || out->bytes || out->name || out->size || !quota ||
        (blocking != 0 && blocking != 1))
    {
        return DVM_IMAGE_INVALID;
    }
    status = fvm_channel_receive_owned(channel, &message, blocking);
    if (status == FVM_NOT_FOUND)
    {
        return DVM_IMAGE_PENDING;
    }
    if (status != FVM_OK)
    {
        return DVM_IMAGE_IO;
    }
    result = decode_message(name, &message, quota, out);
    fvm_channel_message_release(&message);
    return result;
}


/* FlashVM strings require valid WTF-8: ASCII hex preserves every possible byte.
 * The channel copies before send returns; encoded storage is never retained here. */
DVM_ImageResult dvm_image_send_channel(FVM_Channel *channel, const DVM_ModuleImage *image, int blocking)
{
    static const char hex[] = "0123456789abcdef";
    FVM_Value message;
    FVM_Result status;
    char *encoded;
    size_t length;
    size_t index;
    if (!channel || !image || !image->bytes || !image->size || (blocking != 0 && blocking != 1))
    {
        return DVM_IMAGE_INVALID;
    }
    if (image->size > DVM_MODULE_IMAGE_MAX)
    {
        return DVM_IMAGE_LIMIT;
    }
    length = DVM_MODULE_CHANNEL_PREFIX_SIZE + image->size * 2;
    encoded = (char *)malloc(length);
    if (!encoded)
    {
        return DVM_IMAGE_NOMEM;
    }
    memcpy(encoded, DVM_MODULE_CHANNEL_PREFIX, DVM_MODULE_CHANNEL_PREFIX_SIZE);
    for (index = 0; index < image->size; index++)
    {
        encoded[DVM_MODULE_CHANNEL_PREFIX_SIZE + index * 2] = hex[image->bytes[index] >> 4];
        encoded[DVM_MODULE_CHANNEL_PREFIX_SIZE + index * 2 + 1] = hex[image->bytes[index] & 15];
    }
    status = fvm_value_string_bytes(&message, encoded, length);
    if (status == FVM_OK)
    {
        status = blocking ? fvm_channel_send(channel, &message) : fvm_channel_try_send(channel, &message);
    }
    free(encoded);
    if (status == FVM_OK)
    {
        return DVM_IMAGE_OK;
    }
    if (status == FVM_NOT_FOUND)
    {
        return DVM_IMAGE_PENDING;
    }
    if (status == FVM_OUT_OF_MEMORY)
    {
        return DVM_IMAGE_NOMEM;
    }
    return DVM_IMAGE_IO;
}
