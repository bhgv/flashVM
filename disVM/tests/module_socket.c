/* Collect a module frame over the actual FlashVM deadline-aware TCP transport. */
#include "disvm/flashvm_host.h"
#ifdef NDEBUG
#undef NDEBUG /* Product checks must execute in Release builds too. */
#endif
#include <assert.h>
#include <string.h>

/* The test server sends a short-read frame, a second frame and then EOF. */
int main(int argc, char **argv)
{
    static const unsigned char first[] = {0xc0, 0x0c, 0x80, 0x30, 0, 255, 42};
    DVM_ModuleImage image = {0, 0, 0};
    FVM_NetSocket *socket;
    assert(argc == 2);
    socket = fvm_net_open("127.0.0.1", argv[1], 2000, 5000);
    assert(socket);
    assert(dvm_image_socket("tcp:first", socket, sizeof(first), 1024, &image) == DVM_IMAGE_OK);
    assert(image.size == sizeof(first) && !memcmp(image.bytes, first, sizeof(first)));
    dvm_image_dispose(&image);
    assert(dvm_image_socket("tcp:next", socket, 3, 1024, &image) == DVM_IMAGE_OK);
    assert(!memcmp(image.bytes, "end", 3));
    dvm_image_dispose(&image);
    assert(dvm_image_socket("tcp:eof", socket, 1, 1024, &image) == DVM_IMAGE_TRUNCATED);
    fvm_net_close(socket);
    assert(!image.bytes && !image.name);
    return 0;
}
