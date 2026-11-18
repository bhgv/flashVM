/* Copyright (c) 2026 bhgv
 * [GC:] EN: Bounded provider-backed Dis images, operation-local cancellation and original native-cell parser.
 * RU: Ограниченные Dis-образы провайдера, отмена операции и исходный native-cell parser.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "raise.h"
#include "loader.h"
#include "resource.h"
#include "native_wait.h"
#include "cancel.h"
#include "runtime_gc.h"
#include "runqueue.h"
#include "close_worker.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/* [GC:] EN/RU: Native-only heap state survives managed reentry exceptions; no unregistered managed owners.
 * Native heap-state переживает exceptions managed reentry, не содержит неучтённых managed owners.
 */
typedef struct ProviderModule
{
    DVM_VfsHandle handle; /* Exclusive opened object, moved to native closer before managed reentry. */
    DVM_CloseJob *close;  /* Preallocated cleanup reservation, no allocation after open. */
    unsigned char *bytes; /* Owned bounded full image through original parsemod call. */
    size_t length;        /* Exact advertised opened-object length. */
    Dir metadata;         /* Private uncached loader metadata, never a global cache identity. */
} ProviderModule;


/* EN/RU: Assemble exact unsigned64 metadata bits, without signed/narrowing conversion.
 * Собрать точные unsigned64 metadata bits без знакового/сужающего преобразования.
 */
static uint64_t number(FVM_FileNumber64 value)
{
    return ((uint64_t)value.high << 32) | value.low;
}


/* EN/RU: Close staged object asynchronously or discard unused reservation; native-only, no allocation or VM access.
 * Передать staged object в asynchronous close либо освободить пустой reservation; без allocation и VM.
 */
static void closeimage(ProviderModule *image)
{
    if (image->handle.ops)
    {
        dvm_close_submit_resource(image->close, &image->handle);
    }
    else
    {
        dvm_close_discard(image->close);
    }
    image->close = NULL;
}


/* EN/RU: Borrow selected plan/image on native source; open once, stat that handle and read exactly its bounded extent.
 * Never resolve another path or retry another provider. All reads use the current native cancellation scope.
 * Заимствовать selected plan/image на native source; один open, stat его handle и чтение точного bounded extent.
 * Без нового path lookup и другого provider; reads используют текущий native cancellation scope.
 */
static int readimage(DVM_ResourcePlan *plan, ProviderModule *image)
{
    FVM_FileInfo metadata; /* Opened object's actual metadata, not earlier path metadata. */
    if (dvm_cancel_requested(dvm_cancel_current()) ||
        dvm_provider_open(plan->provider, plan->relative, "r", &image->handle) != FVM_OK ||
        (image->handle.native_flags & (DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_DATA)) !=
            (DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_DATA) ||
        dvm_vfs_info(&image->handle, &metadata) != FVM_OK || metadata.directory ||
        metadata.length.high > 0xffffffffUL || metadata.length.low > 0xffffffffUL)
    {
        return 0;
    }
    uint64_t length = number(metadata.length); /* Validate before any allocation/narrowing. */
    if (!length || length >= DVM_MODULE_IMAGE_LIMIT || dvm_cancel_requested(dvm_cancel_current()))
    {
        return 0;
    }
    image->bytes = malloc((size_t)length);
    if (!image->bytes)
    {
        return 0;
    }
    image->length = (size_t)length;
    size_t received = 0; /* Exact cumulative acknowledged prefix, never inferred from an error. */
    while (received < image->length && !dvm_cancel_requested(dvm_cancel_current()))
    {
        DVM_IoResult result = dvm_vfs_read_chunk(&image->handle, image->bytes + received, image->length - received);
        if (result.status != FVM_OK || !(result.flags & DVM_IO_PREFIX) || (result.flags & DVM_IO_UNCERTAIN) ||
            !result.count || result.count > image->length - received)
        {
            return 0;
        }
        received += result.count;
        if (result.stop == DVM_IO_EOF && received != image->length)
        {
            return 0;
        }
    }
    if (received != image->length || dvm_cancel_requested(dvm_cancel_current()))
    {
        return 0;
    }
    image->metadata.length = image->length;
    image->metadata.mode = DVM_MODULE_NO_CACHE;
    image->metadata.qid.type = DVM_MODULE_UNCACHEABLE_QID;
    return 1;
}


/* [GC:] EN/RU: See loader.h; cleanup native image before propagating a managed reentry/parser exception.
 * См. loader.h; очистить native image до передачи exception managed reentry/parser.
 */
Module *dvm_provider_readmod(char *path, DVM_ResourcePlan *plan)
{
    Prog *process = currun(); /* ACTIVE current owner only, never a remote/foreign process query. */
    if (!process || !((DVM_RuntimeProcess *)process)->provider_execution)
    {
        kwerrstr("provider module execution requires an accepted execution-provider launch");
        return NULL;
    }
    if (!plan || !plan->provider || !(dvm_provider_native_flags(plan->provider) & DVM_PROVIDER_CANCEL_PATH))
    {
        kwerrstr("module provider requires native cancellation support");
        return NULL;
    }
    ProviderModule *image = calloc(1, sizeof(*image)); /* Stable through native wait and error unwinding. */
    if (!image)
    {
        kwerrstr("cannot allocate provider module state");
        return NULL;
    }
    image->close = dvm_close_prepare();
    if (!image->close)
    {
        free(image);
        kwerrstr("cannot reserve provider module close");
        return NULL;
    }
    if (waserror())
    {
        closeimage(image);
        free(image->bytes);
        free(image);
        nexterror();
    }
    DVM_VfsWait *operation = NULL; /* Source-owned wait/scope; removed before managed reentry. */
    int released = dvm_vfs_path_enter(&operation); /* ACTIVE owner publishes roots and registers cancellation before leaving. */
    int valid = 0; /* No parse is admitted on incomplete or cancelled input. */
    int cancelled = 0; /* Exact native operation outcome before managed reentry. */
    if (released >= 0)
    {
        valid = readimage(plan, image);
        closeimage(image);
        cancelled = dvm_vfs_wait_end(operation);
        dvm_runtime_gc_native_enter(released);
    }
    Module *answer = NULL; /* Ordinary parser returns an owned module or NULL. */
    if (released >= 0 && valid && !cancelled)
    {
        answer = parsemod(path, image->bytes, (ulong)image->length, &image->metadata);
    }
    else
    {
        kwerrstr(cancelled || released == -2 ? "provider module load cancelled" : "cannot read complete provider module image");
    }
    closeimage(image);
    free(image->bytes);
    free(image);
    poperror();
    return answer;
}
