/* Dis module bytes use the same portable file implementation as FlashVM. */
#include "disvm/flashvm_host.h"
#include "file_port.h"

/* Adapt status without changing file positions beyond the requested frame. */
static int read_file(void *file, void *bytes, size_t size, size_t *received)
{
    return fvm_port_file_read((FVM_PortFile *)file, bytes, size, received) == FVM_OK;
}

/* Validate before opening so an invalid request never touches the filesystem. */
DVM_ImageResult dvm_image_file(const char *name, const char *path, size_t size,
                             size_t quota, DVM_ModuleImage *out)
{
    FVM_PortFile *file = 0;
    DVM_ImageResult result;
    if (!path || !*path || !name || !*name || !out || out->bytes || out->name || out->size || !size || !quota)
        return DVM_IMAGE_INVALID;
    if (size > quota || size > DVM_MODULE_IMAGE_MAX)
        return DVM_IMAGE_LIMIT;
    if (fvm_port_file_open(path, "r", &file) != FVM_OK)
        return DVM_IMAGE_IO;
    result = dvm_image_read(name, read_file, file, size, quota, out);
    fvm_port_file_close(file);
    return result;
}
