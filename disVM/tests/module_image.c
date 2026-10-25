/* Real byte acquisition and failure cleanup; no Dis execution is claimed. */
#include <stdio.h>
#include <stdlib.h>
#include "disvm/module_image.h"

#ifdef NDEBUG
#undef NDEBUG /* Product checks must execute in Release builds too. */
#endif
#include <string.h>


/* In-memory transport deliberately fragments every read to exercise framing. */
typedef struct Reader
{
    const unsigned char *bytes;
    size_t length;
    size_t offset;
    int calls;
    int fail;
    int oversized;
} Reader;


/* Emulate a conforming short-read transport, plus explicit failure injection. */
static int read_bytes(void *user, void *bytes, size_t capacity, size_t *received)
{
    Reader *reader = (Reader *)user;
    size_t count = reader->length - reader->offset;
    reader->calls++;
    if (reader->fail)
    {
        return 0;
    }
    if (reader->oversized)
    {
        *received = capacity + 1;
        return 1;
    }
    if (count > 2)
    {
        count = 2;
    }
    if (count > capacity)
    {
        count = capacity;
    }
    memcpy(bytes, reader->bytes + reader->offset, count);
    reader->offset += count;
    *received = count;
    return 1;
}


/* Cover binary copying, embedded resources, framing, quotas and failed reads. */
int main(void)
{
    static const unsigned char embedded[] = {0xc0, 0x0c, 0x80, 0x30, 0, 42, 0, 7};
    DVM_ModuleImage image = {0, 0, 0};
    Reader reader = {embedded, sizeof(embedded), 0, 0, 0, 0};
    if (!(dvm_image_copy("embedded:answer", embedded, sizeof(embedded), 64, &image) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(image.bytes != embedded && image.size == sizeof(embedded)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(image.bytes, embedded, sizeof(embedded))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(image.name, "embedded:answer")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_copy("occupied", embedded, 1, 64, &image) == DVM_IMAGE_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_image_dispose(&image);
    if (!(dvm_image_read("channel:one", read_bytes, &reader, 5, 64, &image) == DVM_IMAGE_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(reader.offset == 5 && reader.calls == 3 && image.size == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_image_dispose(&image);
    reader.offset = 0;
    if (!(dvm_image_read("truncated", read_bytes, &reader, 9, 64, &image) == DVM_IMAGE_TRUNCATED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!image.bytes && !image.name && !image.size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    reader.calls = 0;
    if (!(dvm_image_read("quota", read_bytes, &reader, 9, 8, &image) == DVM_IMAGE_LIMIT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(reader.calls == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    reader.fail = 1;
    if (!(dvm_image_read("error", read_bytes, &reader, 4, 64, &image) == DVM_IMAGE_IO))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    reader.fail = 0;
    reader.oversized = 1;
    if (!(dvm_image_read("bad callback", read_bytes, &reader, 4, 64, &image) == DVM_IMAGE_IO))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_copy("limit", embedded, DVM_MODULE_IMAGE_MAX + 1, DVM_MODULE_IMAGE_MAX + 1, &image) ==
          DVM_IMAGE_LIMIT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_copy("empty", embedded, 0, 64, &image) == DVM_IMAGE_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_copy("null", 0, 1, 64, &image) == DVM_IMAGE_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(dvm_image_read("null reader", 0, 0, 1, 64, &image) == DVM_IMAGE_INVALID))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    dvm_image_dispose(&image);
    dvm_image_dispose(&image);
    dvm_image_dispose(0);
    return 0;
}
