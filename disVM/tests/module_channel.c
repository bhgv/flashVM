/* Link against the real FlashVM channel implementation, never a mock channel. */
#include <stdio.h>
#include <stdlib.h>
#include "disvm/flashvm_host.h"

#ifdef NDEBUG
#undef NDEBUG /* Product checks must execute in Release builds too. */
#endif
#include <string.h>


/* Verify binary transport, owned receive, quota rejection and close semantics. */
int main(void)
{
    static const char bytes[] = {'D', 0, 'I', 0, 'S', (char)255};
    DVM_ModuleImage image = {0, 0, 0};
    DVM_ModuleImage source = {0, 0, 0};
    FVM_Channel *channel = 0;
    FVM_Value message;
    if (!(fvm_channel_create(&channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_channel("channel:empty", channel, 0, 1024, &image) == DVM_IMAGE_PENDING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_copy("memory:all-bytes", bytes, sizeof(bytes), 1024, &source) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_send_channel(channel, &source, 0) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_channel("channel:module", channel, 0, 1024, &image) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(image.size == sizeof(bytes) && !memcmp(image.bytes, bytes, sizeof(bytes))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_image_dispose(&image);
    if (!(dvm_image_send_channel(channel, &source, 0) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_channel("channel:quota", channel, 0, 2, &image) == DVM_IMAGE_LIMIT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_channel("channel:consumed", channel, 0, 1024, &image) == DVM_IMAGE_PENDING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&message, 42);
    if (!(fvm_channel_send(channel, &message) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_channel("channel:wrongtype", channel, 0, 1024, &image) == DVM_IMAGE_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_send_channel(channel, &source, 0) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_channel("channel:last", channel, 0, 1024, &image) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_image_dispose(&source);
    fvm_channel_close(channel);
    fvm_channel_release(channel);
    if (!(!memcmp(image.bytes, bytes, sizeof(bytes))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_image_dispose(&image);
    return 0;
}
