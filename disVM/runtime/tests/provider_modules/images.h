/* Copyright (c) 2026 bhgv
 * EN: Native test-only live image providers, short reads and opened-object failure injection.
 * RU: Native test-only провайдеры живых образов, short reads и отказы открытого объекта.
 */
#ifndef PROVIDER_MODULE_IMAGES_H
#define PROVIDER_MODULE_IMAGES_H
#include "../../vfs/provider.h"
#include "../../vfs/cancel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
static __thread int refuse_image_allocation; /* Fixture-only allocation refusal on this exact native loader caller. */

static pthread_mutex_t read_lock = PTHREAD_MUTEX_INITIALIZER; /* Fixture-only static lifetime, never a guest/root address. */
static pthread_cond_t read_changed = PTHREAD_COND_INITIALIZER; /* Exact native read/parent FD rendezvous. */
static int read_entered; /* Protected by read_lock, real child loader reached native I/O. */
static int read_cancelled; /* Protected by read_lock, child's operation source requested cancellation. */


/* EN/RU: Native cancellation touches only process-lifetime fixture primitives, never provider/image/guest pointers.
 * Native-отмена касается лишь process-lifetime fixture primitives, не provider/image/guest pointers.
 */
static void read_cancel(void *unused)
{
    (void)unused;
    pthread_mutex_lock(&read_lock);
    read_cancelled = 1;
    pthread_cond_broadcast(&read_changed);
    pthread_mutex_unlock(&read_lock);
}

static __thread size_t expected_image_size; /* Exact next loader image allocation after opened-object metadata. */
static pthread_mutex_t allocation_lock = PTHREAD_MUTEX_INITIALIZER; /* Fixture-only copied-buffer ownership ledger. */
static void *image_allocations[16]; /* Live native image pointers, never managed addresses. */
static unsigned allocated_images; /* Total actual tracked image allocations. */
static unsigned freed_images; /* Total matching actual frees, checked after runtime retirement. */


/* EN/RU: Original free, consumes native allocation or accepts NULL, no fixture recursion.
 * Исходный free, поглощает native allocation либо NULL, без fixture-рекурсии.
 */
extern void __real_free(void *);


/* EN/RU: Observe actual image-buffer retirement across normal/parser/cancellation exits, preserving the production free.
 * Наблюдать настоящее освобождение image-buffer при normal/parser/cancellation exit, сохраняя production free.
 */
void __wrap_free(void *value)
{
    if (value)
    {
        pthread_mutex_lock(&allocation_lock);
        for (unsigned i = 0; i < allocated_images; i++)
        {
            if (image_allocations[i] == value)
            {
                image_allocations[i] = NULL;
                freed_images++;
                break;
            }
        }
        pthread_mutex_unlock(&allocation_lock);
    }
    __real_free(value);
}


/* EN/RU: Original allocator, caller owns returned allocation/NULL.
 * Исходный allocator, caller владеет результатом/NULL.
 */
extern void *__real_malloc(size_t);


/* EN/RU: Refuse only the next image allocation after opened metadata, never other guest/native workers.
 * Отказать лишь следующему image-allocation после opened metadata, не затрагивая чужие workers.
 */
void *__wrap_malloc(size_t size)
{
    if (refuse_image_allocation)
    {
        refuse_image_allocation = 0;
        return NULL;
    }
    void *result = __real_malloc(size); /* Actual loader-owned allocation, no fake storage. */
    if (expected_image_size && size == expected_image_size)
    {
        expected_image_size = 0;
        if (result)
        {
            pthread_mutex_lock(&allocation_lock);
            if (allocated_images == sizeof(image_allocations) / sizeof(*image_allocations))
            {
                exit(2);
            }
            image_allocations[allocated_images++] = result;
            pthread_mutex_unlock(&allocation_lock);
        }
    }
    return result;
}


/* EN/RU: Immutable compiled bytes are owned until every provider/open owner retires.
 * Неизменяемые compiled bytes принадлежат до завершения всех provider/open owners.
 */
typedef struct Image
{
    unsigned char *bytes; /* Owned actual source-built native-cell Dis image. */
    size_t size;          /* Exact owned allocation extent. */
} Image;


/* EN/RU: Native fixture state shared only through retained providers; counters are atomic.
 * Native fixture-state разделяется лишь через retained providers, counters атомарны.
 */
typedef struct ImageSource
{
    Image app;          /* Application image, normal typed dependency load. */
    Image first;        /* Original dependency image. */
    Image next;         /* Updated dependency with identical metadata/length for cache isolation. */
    int terminal;       /* Lower code source, upper ordinary-data source. */
    unsigned switched;  /* Atomic live dependency version. */
    unsigned opened;    /* Atomic dependency opens, proves which source supplied executable bytes. */
    unsigned closed;    /* Atomic independent opened-stream retirements. */
    unsigned destroyed; /* Atomic final provider retirement. */
} ImageSource;


/* EN/RU: One cursor pins actual selected image, never re-resolves the provider's later version.
 * Один cursor удерживает выбранный image, не разрешает новую версию provider повторно.
 */
typedef struct ImageCursor
{
    ImageSource *source; /* Borrowed under ordinary provider-wrapper pin. */
    Image *image;        /* Frozen selected bytes for this open, NULL for data/switch. */
    size_t offset;       /* Acknowledged prefix consumed once. */
    int dependency;      /* Inject faults only into the dependency, never application/control. */
    int switching;       /* Explicit fixture write changes future opens only. */
    int waiting;         /* Parent read waits for actual child loader I/O, not a guest polling loop. */
} ImageCursor;


/* EN/RU: Read actual compiled image before guest startup; owned bytes/length, fatal setup failure only.
 * Прочитать настоящий compiled image до startup guest; свои bytes/length, fatal только при отказе setup.
 */
static Image image_read(const char *name)
{
    Image image = {0}; /* Own complete compiled file bytes. */
    FILE *input = fopen(name, "rb"); /* Native fixture acquisition, not a guest/ambient fallback. */
    if (!input || fseek(input, 0, SEEK_END))
    {
        exit(2);
    }
    long size = ftell(input); /* Bounded actual native image length. */
    if (size <= 0 || size >= 8 * 1024 * 1024 || fseek(input, 0, SEEK_SET))
    {
        exit(2);
    }
    image.size = (size_t)size;
    image.bytes = malloc(image.size);
    if (!image.bytes || fread(image.bytes, 1, image.size, input) != image.size || fclose(input))
    {
        exit(2);
    }
    return image;
}


/* EN/RU: Return current path metadata; missing names are explicit absence, not permission failure.
 * Вернуть текущие path metadata; отсутствующие имена дают явное отсутствие, не отказ прав.
 */
static FVM_Result image_info(void *state, const char *path, FVM_FileInfo *output)
{
    ImageSource *source = state; /* Retained by current plan/provider wrapper. */
    if (strcmp(path, ".") && strcmp(path, "app.dis") && strcmp(path, "dep.dis") && strcmp(path, "data") &&
        (source->terminal || (strcmp(path, "switch") && strcmp(path, "waiting"))))
    {
        return FVM_NOT_FOUND;
    }
    memset(output, 0, sizeof(*output));
    output->directory = !strcmp(path, ".");
    output->mode = output->directory ? 0555 : !strcmp(path, "switch") ? 0222 : 0444;
    strcpy(output->name, path);
    Image *image = !strcmp(path, "app.dis") ? &source->app : !strcmp(path, "dep.dis") ?
        (__atomic_load_n(&source->switched, __ATOMIC_ACQUIRE) ? &source->next : &source->first) : NULL; /* Native current source. */
    output->length.low = image ? image->size : output->directory ? 0 : source->terminal ? 8 : 9;
    output->identity.low = image == &source->app ? 1 : 2;
    output->modified.low = 1;
    return dvm_cancel_requested(dvm_cancel_current()) ? FVM_ERROR : FVM_OK;
}


/* EN/RU: Copy metadata from the actually opened object, including deliberately mismatched path/open metadata cases.
 * Скопировать metadata открытого объекта, включая намеренное различие path/open metadata.
 */
static FVM_Result image_stat(void *state, FVM_FileInfo *output)
{
    ImageCursor *cursor = state; /* Exclusive source operation. */
    if (!cursor->source->terminal && cursor->dependency && CONTROL == 4)
    {
        return FVM_ERROR;
    }
    FVM_Result result = image_info(cursor->source, cursor->dependency ? "dep.dis" : cursor->image ? "app.dis" : "data", output);
    if (cursor->image)
    {
        output->length.low = cursor->image->size;
    }
    if (!cursor->source->terminal && cursor->dependency && CONTROL == 3)
    {
        output->length.low = 8 * 1024 * 1024;
    }
    if (cursor->image && !(cursor->dependency && (CONTROL == 3 || CONTROL == 4 || CONTROL == 7 || CONTROL == 8 || CONTROL == 14 || CONTROL == 15)))
    {
        expected_image_size = cursor->image->size;
    }
    if (!cursor->source->terminal && cursor->dependency && CONTROL == 7)
    {
        refuse_image_allocation = 1;
    }
    if (!cursor->source->terminal && cursor->dependency && CONTROL == 8)
    {
        output->length.high = 1;
    }
    if (!cursor->source->terminal && cursor->dependency && CONTROL == 14)
    {
        output->length.high = 1UL << 32; /* Malformed split part must not wrap back into a small valid size. */
    }
    if (!cursor->source->terminal && cursor->dependency && CONTROL == 15)
    {
        output->directory = 1; /* Opened-object kind, not earlier path metadata, decides admission. */
    }
    return result;
}


/* EN/RU: Read a three-byte prefix or injected short EOF; all positive counts are actual copied bytes.
 * Прочитать prefix до трёх bytes либо injected short EOF; положительные counts — реальные copied bytes.
 */
static FVM_Result image_read_chunk(void *state, void *bytes, size_t size, size_t *received)
{
    ImageCursor *cursor = state; /* Per-open offset, never source-global position. */
    if (dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    if (CONTROL == 10 && cursor->waiting)
    {
        pthread_mutex_lock(&read_lock);
        while (!read_entered)
        {
            pthread_cond_wait(&read_changed, &read_lock);
        }
        pthread_mutex_unlock(&read_lock);
        size_t count = size < 5 ? size : 5;
        memcpy(bytes, "ready", count);
        *received = count;
        return FVM_OK;
    }
    if (CONTROL == 10 && cursor->dependency && !cursor->source->terminal)
    {
        DVM_CancelTap *tap = dvm_cancel_subscribe(dvm_cancel_current(), read_cancel, NULL, NULL); /* Actual child native operation scope. */
        if (!tap)
        {
            exit(2);
        }
        pthread_mutex_lock(&read_lock);
        read_entered = 1;
        pthread_cond_broadcast(&read_changed);
        while (!read_cancelled)
        {
            pthread_cond_wait(&read_changed, &read_lock);
        }
        pthread_mutex_unlock(&read_lock);
        dvm_cancel_unsubscribe(tap);
        return FVM_ERROR;
    }
    const unsigned char *input = cursor->image ? cursor->image->bytes : (const unsigned char *)(cursor->source->terminal ? "terminal" : "execution"); /* Frozen native input. */
    size_t length = cursor->image ? cursor->image->size : cursor->source->terminal ? 8 : 9;
    if (!cursor->source->terminal && cursor->dependency && CONTROL == 2)
    {
        length /= 2;
    }
    size_t count = length - cursor->offset;
    if (count > size)
    {
        count = size;
    }
    if (cursor->image && count > 3)
    {
        count = 3;
    }
    memcpy(bytes, input + cursor->offset, count);
    cursor->offset += count;
    *received = count;
    if (!cursor->source->terminal && cursor->dependency && CONTROL == 6)
    {
        dvm_cancel_request(dvm_cancel_current()); /* Test actual sticky native operation cancellation after a copied prefix. */
    }
    if (!cursor->source->terminal && cursor->dependency && CONTROL == 9)
    {
        *received = size + 1; /* Malformed acknowledgement, no corresponding out-of-bounds copy. */
    }
    return FVM_OK;
}


/* EN/RU: Switch future dependency opens only; current opened image remains pinned to original bytes.
 * Переключить лишь будущие dependency-opens; текущий image удерживает прежние bytes.
 */
static FVM_Result image_write(void *state, const void *bytes, size_t size)
{
    ImageCursor *cursor = state; /* Native fixture write-only switch. */
    (void)bytes;
    if (!cursor->switching || !size || dvm_cancel_requested(dvm_cancel_current()))
    {
        return FVM_ERROR;
    }
    __atomic_store_n(&cursor->source->switched, 1, __ATOMIC_RELEASE);
    return FVM_OK;
}


/* EN/RU: Dispose cursor, final provider wrapper pin retires independently on the native closer.
 * Освободить cursor, последний provider-wrapper pin отдельно снимается native-closer.
 */
static void image_close(void *state)
{
    ImageCursor *cursor = state; /* Exclusive close owner. */
    __atomic_add_fetch(&cursor->source->closed, 1, __ATOMIC_RELAXED);
    free(cursor);
}
static const DVM_VfsOps image_stream = {.info = image_stat, .read = image_read_chunk, .write = image_write,
    .close = image_close}; /* Actual byte stream, no fake Module result. */


/* EN/RU: Open selected immutable image or fixture data/control, failure never returns a partial handle.
 * Открыть выбранный immutable image либо fixture data/control, при отказе нет partial handle.
 */
static FVM_Result image_open(void *state, const char *path, const char *mode, DVM_VfsHandle *output)
{
    ImageSource *source = state; /* Native owner through provider. */
    FVM_FileInfo metadata; /* Ordinary path existence, before cursor publication. */
    if (image_info(source, path, &metadata) != FVM_OK || metadata.directory)
    {
        return FVM_ERROR;
    }
    int switching = !strcmp(path, "switch"); /* Only execution provider exports this fixture node. */
    if (strcmp(mode, switching ? "wo" : "r"))
    {
        return FVM_ERROR;
    }
    int dependency = !strcmp(path, "dep.dis"); /* Count refusals too, not only successful handles. */
    if (dependency)
    {
        __atomic_add_fetch(&source->opened, 1, __ATOMIC_RELAXED);
    }
    if (!source->terminal && dependency && CONTROL == 1)
    {
        return FVM_ERROR;
    }
    ImageCursor *cursor = calloc(1, sizeof(*cursor)); /* Exclusive offset ownership. */
    if (!cursor)
    {
        return FVM_OUT_OF_MEMORY;
    }
    cursor->source = source;
    cursor->dependency = dependency;
    cursor->switching = switching;
    cursor->waiting = !strcmp(path, "waiting");
    cursor->image = !strcmp(path, "app.dis") ? &source->app : dependency ?
        (__atomic_load_n(&source->switched, __ATOMIC_ACQUIRE) ? &source->next : &source->first) : NULL;
    output->state = cursor;
    output->ops = &image_stream;
    output->native_flags = DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_CLOSE;
    if (!source->terminal && dependency && CONTROL == 11)
    {
        output->native_flags &= ~DVM_VFS_CANCEL_DATA;
    }
    if (!source->terminal && dependency && CONTROL == 12)
    {
        output->native_flags &= ~DVM_VFS_CANCEL_INFO;
    }
    return FVM_OK;
}


/* EN/RU: Dispose immutable bytes after every opened stream retires; counter proves final owner drain.
 * Освободить immutable bytes после всех streams; counter доказывает final owner-drain.
 */
static void image_destroy(void *state)
{
    ImageSource *source = state; /* Stack fixture lives through runtime shutdown. */
    free(source->app.bytes);
    free(source->first.bytes);
    free(source->next.bytes);
    __atomic_add_fetch(&source->destroyed, 1, __ATOMIC_RELEASE);
}
static const DVM_ProviderOps image_operations = {.info = image_info, .open = image_open,
    .destroy = image_destroy, .flags = DVM_PROVIDER_STABLE_BIND}; /* Live images under one stable root. */
#endif
