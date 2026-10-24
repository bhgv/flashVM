/* Dis and SWF share FlashVM's deadline-aware POSIX/Winsock transport. */
#include "disvm/flashvm_host.h"

/* A partial read retains the existing socket's total deadline/cancellation. */
static int read_socket(void *socket, void *bytes, size_t size, size_t *received)
{
    return fvm_net_read((FVM_NetSocket *)socket, bytes, size, received);
}

/* Borrow the connected stream: framing and TLS/HTTP/Styx decoding belong above it. */
DVM_ImageResult dvm_image_socket(const char *name, FVM_NetSocket *socket, size_t size,
                               size_t quota, DVM_ModuleImage *out)
{
    if (!socket)
        return DVM_IMAGE_INVALID;
    return dvm_image_read(name, read_socket, socket, size, quota, out);
}
