/* Collect a module frame over the actual FlashVM deadline-aware TCP transport. */
#include <stdio.h>
#include <stdlib.h>
#include "disvm/flashvm_host.h"

#ifdef NDEBUG
#undef NDEBUG /* Product checks must execute in Release builds too. */
#endif
#include <string.h>


/* The test server sends a short-read frame, a second frame and then EOF. */
int main(int argc, char **argv)
{
    static const unsigned char first[] = {0xc0, 0x0c, 0x80, 0x30, 0, 255, 42};
    DVM_ModuleImage image = {0, 0, 0};
    FVM_NetSocket *socket;
    if (!(argc == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    socket = fvm_net_open("127.0.0.1", argv[1], 2000, 5000);
    if (!(socket))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_socket("tcp:first", socket, sizeof(first), 1024, &image) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(image.size == sizeof(first) && !memcmp(image.bytes, first, sizeof(first))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_image_dispose(&image);
    if (!(dvm_image_socket("tcp:next", socket, 3, 1024, &image) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(image.bytes, "end", 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_image_dispose(&image);
    if (!(dvm_image_socket("tcp:eof", socket, 1, 1024, &image) == DVM_IMAGE_TRUNCATED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_net_close(socket);
    if (!(!image.bytes && !image.name))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
