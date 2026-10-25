/* Bounded byte collection shared by files, sockets, channels and compiler output. */
#include "disvm/module_image.h"
#include <stdlib.h>
#include <string.h>


/* Reject invalid/occupied outputs before allocation or consuming a transport. */
static DVM_ImageResult validate(const char *name, size_t size, size_t quota, DVM_ModuleImage *out)
{
    if (!name || !*name || !out || out->bytes || out->name || out->size || !size || !quota)
    {
        return DVM_IMAGE_INVALID;
    }
    if (size > quota || size > DVM_MODULE_IMAGE_MAX)
    {
        return DVM_IMAGE_LIMIT;
    }
    return DVM_IMAGE_OK;
}


/* Allocate unpublished native storage; publish only after complete acquisition. */
static DVM_ImageResult allocate_image(const char *name, size_t size, DVM_ModuleImage *image)
{
    size_t length = strlen(name);
    image->name = (char *)malloc(length + 1);
    image->bytes = (unsigned char *)malloc(size);
    if (!image->name || !image->bytes)
    {
        dvm_image_dispose(image);
        return DVM_IMAGE_NOMEM;
    }
    memcpy(image->name, name, length + 1);
    image->size = size;
    return DVM_IMAGE_OK;
}


/* Copy borrowed bytes before their source (IDE buffer/resource) can disappear. */
DVM_ImageResult dvm_image_copy(const char *name, const void *bytes, size_t size, size_t quota, DVM_ModuleImage *out)
{
    DVM_ModuleImage image = {0, 0, 0};
    DVM_ImageResult result = validate(name, size, quota, out);
    if (result != DVM_IMAGE_OK)
    {
        return result;
    }
    if (!bytes)
    {
        return DVM_IMAGE_INVALID;
    }
    result = allocate_image(name, size, &image);
    if (result != DVM_IMAGE_OK)
    {
        return result;
    }
    memcpy(image.bytes, bytes, size);
    *out = image;
    return DVM_IMAGE_OK;
}


/* Preserve framing across short reads and close ownership on every failure. */
DVM_ImageResult
dvm_image_read(const char *name, DVM_ModuleRead read, void *user, size_t size, size_t quota, DVM_ModuleImage *out)
{
    DVM_ModuleImage image = {0, 0, 0};
    DVM_ImageResult result = validate(name, size, quota, out);
    size_t offset = 0;
    if (result != DVM_IMAGE_OK)
    {
        return result;
    }
    if (!read)
    {
        return DVM_IMAGE_INVALID;
    }
    result = allocate_image(name, size, &image);
    if (result != DVM_IMAGE_OK)
    {
        return result;
    }
    while (offset < size)
    {
        size_t received = 0;
        size_t capacity = size - offset;
        if (capacity > DVM_MODULE_READ_CHUNK)
        {
            capacity = DVM_MODULE_READ_CHUNK;
        }
        if (!read(user, image.bytes + offset, capacity, &received) || received > capacity)
        {
            result = DVM_IMAGE_IO;
            break;
        }
        if (!received)
        {
            result = DVM_IMAGE_TRUNCATED;
            break;
        }
        offset += received;
    }
    if (result != DVM_IMAGE_OK)
    {
        dvm_image_dispose(&image);
        return result;
    }
    *out = image;
    return DVM_IMAGE_OK;
}


/* No VM finalization or transport close occurs while freeing the owned copy. */
void dvm_image_dispose(DVM_ModuleImage *image)
{
    if (!image)
    {
        return;
    }
    free(image->bytes);
    free(image->name);
    image->bytes = 0;
    image->name = 0;
    image->size = 0;
}
