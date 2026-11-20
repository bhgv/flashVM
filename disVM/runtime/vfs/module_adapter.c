/* Copyright (c) 2026 bhgv
 * EN: Publish immutable trusted adapter bytes as one exact VFS file.
 * RU: Опубликовать неизменяемые байты доверенного адаптера одним файлом VFS.
 */
#include "module_adapter.h"
#include "provider_internal.h"
#include "loader.h"
#include "cancel.h"
#include <stdlib.h>
#include <string.h>


typedef struct AdapterImage
{
    size_t size; /* Immutable positive image length below Dis loader limit. */
    unsigned char bytes[]; /* Copied native bytes, pinned by provider and open handles. */
} AdapterImage;


typedef struct AdapterCursor
{
    AdapterImage *image; /* Borrowed from the factory pin held by the open handle. */
    size_t offset; /* This handle's independent sequential read position. */
} AdapterCursor;


/* EN/RU: Describe the immutable file or report cancellation.
 * Описать неизменяемый файл либо сообщить об отмене. */
static FVM_Result image_info(void *state, const char *relative, FVM_FileInfo *out)
{
    AdapterImage *image = state;
    if (strcmp(relative, "."))
    {
        return FVM_NOT_FOUND;
    }
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    memset(out, 0, sizeof(*out));
    strcpy(out->name, "adapter.dis");
    out->mode = 0555;
    out->length.low = (unsigned long)image->size;
    return FVM_OK;
}


/* EN/RU: Describe the image pinned by this cursor.
 * Описать образ, удерживаемый этой позицией чтения. */
static FVM_Result cursor_info(void *state, FVM_FileInfo *out)
{
    return image_info(((AdapterCursor *)state)->image, ".", out);
}


/* EN/RU: Copy at most size bytes from the current position into borrowed output.
 * Скопировать до size байт с текущей позиции в заимствованный буфер. */
static FVM_Result cursor_read(void *state, void *bytes, size_t size, size_t *received)
{
    AdapterCursor *cursor = state;
    *received = 0;
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    size_t remaining = cursor->image->size - cursor->offset;
    *received = size < remaining ? size : remaining;
    if (*received)
    {
        memcpy(bytes, cursor->image->bytes + cursor->offset, *received);
        cursor->offset += *received;
    }
    return FVM_OK;
}

static const DVM_VfsOps stream = {.info = cursor_info, .read = cursor_read, .close = free}; /* Read-only cursor. */


/* EN/RU: Allocate a read cursor for dot. Writable opens have no authority.
 * Выделить позицию чтения для точки. Запись не разрешена. */
static FVM_Result image_open(void *state, const char *relative, const char *mode, DVM_VfsHandle *out)
{
    if (strcmp(relative, "."))
    {
        return FVM_NOT_FOUND;
    }
    if (strcmp(mode, "r") || dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    AdapterCursor *cursor = calloc(1, sizeof(*cursor));
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    cursor->image = state;
    out->ops = &stream;
    out->state = cursor;
    out->native_flags = DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_CLOSE;
    return FVM_OK;
}

/* EN/RU: Report effective rights to MAUTORWX for the exact immutable node.
 * Сообщить MAUTORWX права точного неизменяемого узла. */
static FVM_Result image_access(void *state, const char *relative, unsigned access)
{
    (void)state;
    if (strcmp(relative, "."))
    {
        return FVM_NOT_FOUND;
    }
    return (access & FVM_ACCESS_WRITE) || dvm_cancel_requested(dvm_cancel_current()) ?
        FVM_ERROR : FVM_OK;
}

static const DVM_ProviderOps factory = { /* Private identity distinguishes local adapter grants. */
    .info = image_info, .open = image_open, .destroy = free, .flags = DVM_PROVIDER_STABLE_BIND,
    .access = image_access
};


/* EN/RU: See module_adapter.h; adopt copied bytes only on successful construction.
 * См. module_adapter.h; принять копию байтов только при успехе. */
DVM_Provider *dvm_module_adapter(const void *bytes, size_t size)
{
    if (!bytes || !size || size >= DVM_MODULE_IMAGE_LIMIT)
    {
        return NULL;
    }
    AdapterImage *image = malloc(sizeof(*image) + size);
    if (!image)
    {
        return NULL;
    }
    image->size = size;
    memcpy(image->bytes, bytes, size);
    DVM_Provider *provider = dvm_provider_create_native(&factory, image, NULL, 1,
        DVM_PROVIDER_CANCEL_PATH | DVM_PROVIDER_CANCEL_CAPTURE);
    if (!provider)
    {
        free(image);
    }
    return provider;
}


/* EN/RU: See module_adapter.h; identity cannot be supplied as a wire flag.
 * См. module_adapter.h; wire-флаг не устанавливает эту идентичность. */
int dvm_module_adapter_node(DVM_Provider *provider, const char *relative)
{
    return provider && provider->ops == &factory && relative && !strcmp(relative, ".");
}
