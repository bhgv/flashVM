/* Link against the real FlashVM channel implementation, never a mock channel. */
#include "disvm/flashvm_host.h"
#ifdef NDEBUG
#undef NDEBUG /* Product checks must execute in Release builds too. */
#endif
#include <assert.h>
#include <string.h>

/* Verify binary transport, owned receive, quota rejection and close semantics. */
int main(void)
{
    static const char bytes[] = {'D', 0, 'I', 0, 'S', (char)255};
    DVM_ModuleImage image = {0, 0, 0};
    DVM_ModuleImage source = {0, 0, 0};
    FVM_Channel *channel = 0;
    FVM_Value message;
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(dvm_image_channel("channel:empty", channel, 0, 1024, &image) == DVM_IMAGE_PENDING);
    assert(dvm_image_copy("memory:all-bytes", bytes, sizeof(bytes), 1024, &source) == DVM_IMAGE_OK);
    assert(dvm_image_send_channel(channel, &source, 0) == DVM_IMAGE_OK);
    assert(dvm_image_channel("channel:module", channel, 0, 1024, &image) == DVM_IMAGE_OK);
    assert(image.size == sizeof(bytes) && !memcmp(image.bytes, bytes, sizeof(bytes)));
    dvm_image_dispose(&image);
    assert(dvm_image_send_channel(channel, &source, 0) == DVM_IMAGE_OK);
    assert(dvm_image_channel("channel:quota", channel, 0, 2, &image) == DVM_IMAGE_LIMIT);
    assert(dvm_image_channel("channel:consumed", channel, 0, 1024, &image) == DVM_IMAGE_PENDING);
    fvm_value_number(&message, 42);
    assert(fvm_channel_send(channel, &message) == FVM_OK);
    assert(dvm_image_channel("channel:wrongtype", channel, 0, 1024, &image) == DVM_IMAGE_INVALID);
    assert(dvm_image_send_channel(channel, &source, 0) == DVM_IMAGE_OK);
    assert(dvm_image_channel("channel:last", channel, 0, 1024, &image) == DVM_IMAGE_OK);
    dvm_image_dispose(&source);
    fvm_channel_close(channel);
    fvm_channel_release(channel);
    assert(!memcmp(image.bytes, bytes, sizeof(bytes)));
    dvm_image_dispose(&image);
    return 0;
}
