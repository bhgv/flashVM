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
#include "module_shapes.h"
#include "scalar_files.h"
#include "managed_value.h"
#include "../../native/terminal/call_scope.h"
#include "../../native/terminal/scalar_verify.h"
#include "../../native/terminal/load_profile.h"
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
    DVM_ScalarFiles *mounted; /* Owned discovered route until descriptor construction. */
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
static int readimage(DVM_Provider *provider, const char *relative, ProviderModule *image)
{
    FVM_FileInfo metadata; /* Opened object's actual metadata, not earlier path metadata. */
    if (dvm_cancel_requested(dvm_cancel_current()) ||
        dvm_provider_open(provider, relative, "r", &image->handle) != FVM_OK ||
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


/* EN/RU: Read one profile record in the native operation scope, closing asynchronously.
 * Прочитать профиль в области нативной операции и закрыть асинхронно. */
static int check_profile(DVM_Provider *profile, unsigned long tag, const char *name, uint32_t signature)
{
    DVM_CloseJob *close = name[0] ? dvm_close_prepare() : NULL;
    if (!close)
    {
        return 0;
    }
    DVM_VfsHandle reader = {0};
    if (dvm_provider_open(profile, ".", "r", &reader) != FVM_OK)
    {
        dvm_close_discard(close);
        return 0;
    }
    int matched = 0;
    if (reader.native_flags & DVM_VFS_CANCEL_DATA)
    {
        unsigned char frame[STYX_SCALAR_INTERFACE_CAPACITY + 1];
        DVM_IoResult got = dvm_vfs_read_chunk(&reader, frame, sizeof(frame));
        matched = got.status == FVM_OK && (got.flags & DVM_IO_PREFIX) &&
            !(got.flags & DVM_IO_UNCERTAIN) && styxscalaraccept(tag, frame, got.count, name, signature);
    }
    dvm_close_submit_resource(close, &reader);
    return matched;
}


typedef struct ScalarImport
{
    char name[INTERFACE_NAME_LIMIT + 1]; /* Copied compiled export name. */
    uint32_t signature; /* Copied compiled type signature. */
    StyxCallShape shape; /* Optional independently compiled caller-side shape. */
} ScalarImport;


/* EN/RU: Read the complete bounded profile with a fresh, cancellable native handle.
 * Прочитать полный ограниченный профиль через новый нативный handle с отменой. */
static size_t mounted_profile(DVM_Provider *directory, const char *name, unsigned char *bytes, size_t capacity)
{
    DVM_CloseJob *close = dvm_close_prepare();
    DVM_VfsHandle reader = {0};
    if (!close || dvm_provider_open(directory, name, "r", &reader) != FVM_OK)
    {
        dvm_close_discard(close);
        return 0;
    }
    size_t result = 0, used = 0;
    while ((reader.native_flags & DVM_VFS_CANCEL_DATA) && used < capacity)
    {
        DVM_IoResult got = dvm_vfs_read_chunk(&reader, bytes + used, capacity - used);
        if (got.status != FVM_OK || !(got.flags & DVM_IO_PREFIX) ||
            (got.flags & DVM_IO_UNCERTAIN) || got.count > capacity - used)
        {
            break;
        }
        used += got.count;
        if (!got.count || got.stop == DVM_IO_EOF)
        {
            result = used < capacity ? used : 0;
            break;
        }
    }
    dvm_close_submit_resource(close, &reader);
    return result;
}


/* EN/RU: Capture every requested function on one verified directory generation.
 * Захватить все запрошенные функции из одного проверенного поколения каталога. */
static DVM_ScalarFiles *mounted_functions(DVM_ResourcePlan *plan, const ScalarImport *imports, size_t count)
{
    FVM_FileInfo info;
    if (!count || dvm_provider_info(plan->provider, plan->relative, &info) != FVM_OK ||
        !info.directory || !(info.mode & 0444) || !(info.mode & 0111))
    {
        return NULL;
    }
    DVM_Provider *directory = dvm_provider_capture(plan->provider, plan->relative);
    if (!directory)
    {
        return NULL;
    }
    unsigned char profile[STYX_MOUNTED_EXPORT_LIMIT * STYX_SHAPED_OFFER_MAX + 1];
    size_t size = mounted_profile(directory, ".interface", profile, sizeof(profile));
    FVM_FileInfo delta;
    int field_deltas = size && dvm_provider_info(directory, STYX_MANAGED_DELTA_FILE, &delta) == FVM_OK &&
        !delta.directory && (delta.mode & 0444);
    unsigned long graph_tag = STYX_MANAGED_GRAPH_TAG;
    int read_graph_marker = 0;
    DVM_ScalarFiles *head = NULL;
    DVM_ScalarFiles **tail = &head;
    unsigned matched = 0;
    for (size_t i = 0; i < count; i++)
    {
        const char *name = imports[i].name;
        int caller = 0, explicit_end = 0;
        StyxCallShape shape;
        uint32_t signature = imports[i].signature;
        unsigned bit = !strchr(name, '/') && strcmp(name, ".") && strcmp(name, "..") && strcmp(name, ".interface") ?
            styxscalar_profile_match_protocol(profile, size, name, signature, &caller, &shape, &explicit_end) : 0;
        const StyxCallShape *local = &imports[i].shape;
        int shape_conflict = bit && shape.arguments[0] && local->arguments[0] &&
            (strcmp(shape.arguments, local->arguments) || strcmp(shape.result, local->result));
        int dynamic = !styxloadedrequest(signature);
        size_t arguments = 0, result = 0;
        unsigned long ref_profile = 0;
        if (dynamic && bit)
        {
            ref_profile = styxshaperef(&shape, &result);
            int managed = explicit_end == STYX_CALL_RECORD_FRAMING;
            if (!local->arguments[0] || (managed ?
                !dvm_styx_value_shape_supported(shape.arguments) || !dvm_styx_value_shape_supported(shape.result) :
                !ref_profile && !styxshapecells(&shape, &arguments, &result)))
            {
                bit = 0;
            }
        }
        unsigned long managed_tag = bit && field_deltas && explicit_end == STYX_CALL_RECORD_FRAMING &&
            (strchr(shape.arguments, 'r') || strchr(shape.result, 'r')) ?
            styxmanaged_capability(delta.length.high, delta.length.low) : STYX_MANAGED_REQUEST_TAG;
        if (bit && explicit_end == STYX_CALL_RECORD_FRAMING)
        {
            int needs_graph = styxmanaged_graph_required(shape.arguments);
            if (needs_graph && (!caller || managed_tag != STYX_MANAGED_GRAPH_TAG))
            {
                bit = 0;
            }
            else if (managed_tag == STYX_MANAGED_GRAPH_TAG)
            {
                if (caller && !read_graph_marker)
                {
                    unsigned char marker[sizeof(STYX_MANAGED_POSITION_GRAPH_MARKER)];
                    size_t length = mounted_profile(directory, STYX_MANAGED_DELTA_FILE, marker, sizeof(marker));
                    if (length == sizeof(marker) - 1 &&
                        !memcmp(marker, STYX_MANAGED_POSITION_GRAPH_MARKER, length))
                    {
                        graph_tag = STYX_MANAGED_POSITION_GRAPH_TAG;
                    }
                    else if (length == sizeof(marker) - 1 &&
                        !memcmp(marker, STYX_MANAGED_SPARSE_GRAPH_MARKER, length))
                    {
                        graph_tag = STYX_MANAGED_SPARSE_GRAPH_TAG;
                    }
                    read_graph_marker = 1;
                }
                managed_tag = needs_graph ? graph_tag : graph_tag == STYX_MANAGED_POSITION_GRAPH_TAG ?
                    STYX_MANAGED_POSITION_FLAT_TAG : STYX_MANAGED_WIDE_REFS_TAG;
            }
        }
        if (!bit || (bit & matched) || shape_conflict || !(*tail = dvm_scalar_function_files(directory, name)))
        {
            dvm_scalar_files_release(head);
            head = NULL;
            break;
        }
        DVM_CallerBinding binding;
        if (dynamic && caller && !dvm_provider_caller_binding((*tail)->request, &binding))
        {
            dvm_scalar_files_release(head);
            head = NULL;
            break;
        }
        (*tail)->signature = signature;
        (*tail)->caller = caller;
        (*tail)->explicit_end = explicit_end;
        (*tail)->managed_tag = managed_tag;
        (*tail)->dynamic = dynamic;
        (*tail)->ref_profile = ref_profile;
        (*tail)->result_cells = result;
        (*tail)->shape = local->arguments[0] && shape.arguments[0] ? *local : shape;
        matched |= bit;
        tail = &(*tail)->next;
    }
    dvm_provider_release(directory);
    return head;
}


#ifdef DVM_TERMINAL_LAUNCH
/* EN: Parse the exact image granted by the standalone host launch command.
 * RU: Разобрать образ, выданный хостом при настройке самостоятельного запуска. */
Module *dvm_provider_read_launch_image(DVM_Provider *provider, char *label)
{
    ProviderModule image = {0};
    image.close = dvm_close_prepare();
    if (!image.close)
    {
        return NULL;
    }
    DVM_VfsWait *wait;
    int released = dvm_vfs_path_enter(&wait);
    int valid = released >= 0 && readimage(provider, ".", &image);
    closeimage(&image);
    if (released >= 0)
    {
        valid = !dvm_vfs_wait_end(wait) && valid;
        dvm_runtime_gc_native_enter(released);
    }
    Module *module = valid ? parsemod(label, image.bytes, (ulong)image.length, &image.metadata) : NULL;
    free(image.bytes);
    return module;
}


#endif

/* [GC:] EN/RU: See loader.h; cleanup native image before propagating a managed reentry/parser exception.
 * См. loader.h; очистить native image до передачи exception managed reentry/parser.
 */
Module *dvm_provider_readmod(char *path, DVM_ResourcePlan *plan, const Import *imports)
{
    Prog *process = currun(); /* ACTIVE current owner only, never a remote/foreign process query. */
    int execution = process && ((DVM_RuntimeProcess *)process)->provider_execution;
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
        dvm_scalar_files_release(image->mounted);
        free(image->bytes);
        free(image);
        nexterror();
    }
    ScalarImport copied[STYX_MOUNTED_EXPORT_LIMIT] = {0};
    size_t count = 0;
    if (imports && imports != H)
    {
        Module *caller = process && DVM_R.M && DVM_R.M != H ? DVM_R.M->m : NULL;
        for (; count < STYX_MOUNTED_EXPORT_LIMIT && imports[count].name; count++)
        {
            size_t length = styxscalar_name_length(imports[count].name);
            if (length)
            {
                memcpy(copied[count].name, imports[count].name, length);
                copied[count].signature = (uint32_t)imports[count].sig;
                dvm_module_import_shape(caller, imports, count, &copied[count].shape);
            }
        }
        if (imports[count].name)
        {
            count = 0;
        }
    }
    char *name = count == 1 ? copied[0].name : "";
    uint32_t signature = count == 1 ? copied[0].signature : 0;
    DVM_VfsWait *operation = NULL; /* Source-owned wait/scope; removed before managed reentry. */
    int released = dvm_vfs_path_enter(&operation); /* ACTIVE owner publishes roots and registers cancellation before leaving. */
    int valid = 0; /* No parse is admitted on incomplete or cancelled input. */
    int cancelled = 0; /* Exact native operation outcome before managed reentry. */
    DVM_Provider *adapter = NULL;
    int refused = 0;
    int denied = 0;
    int mismatch = 0;
    unsigned long request_tag = 0, reply_tag = 0;
    struct DVM_ScalarFiles *files = NULL;
    if (released >= 0)
    {
        DVM_Provider *profile = NULL;
        unsigned long tag = 0;
        adapter = process ? dvm_module_adapter_source(plan->provider, plan->relative, &denied, &profile, &tag, &request_tag, &reply_tag, &files) : NULL;
        if (process && !denied && !adapter && !request_tag)
        {
            image->mounted = mounted_functions(plan, copied, count);
            if (image->mounted)
            {
                files = image->mounted;
                request_tag = STYX_FUNCTION_REQUEST_TAG;
                reply_tag = STYX_FUNCTION_REPLY_TAG;
            }
        }
        refused = denied || (!execution && !adapter && !request_tag);
        FVM_FileInfo original;
        int available = !refused && (image->mounted || (!adapter && !request_tag) || adapter == plan->provider ||
            (dvm_provider_info(plan->provider, plan->relative, &original) == FVM_OK && !original.directory));
        mismatch = available && profile &&
            ((request_tag && signature != DVM_SCALAR_INT_SIGNATURE) || !check_profile(profile, tag, name, signature));
        valid = available && !mismatch &&
            (request_tag || readimage(adapter ? adapter : plan->provider, adapter ? "." : plan->relative, image));
        closeimage(image);
        cancelled = dvm_vfs_wait_end(operation);
        dvm_runtime_gc_native_enter(released);
    }
    Module *answer = NULL; /* Ordinary parser returns an owned module or NULL. */
    if (released >= 0 && valid && !cancelled)
    {
        if (request_tag)
        {
            answer = dvm_scalar_module(path, name, request_tag, reply_tag, plan->names.version, files);
        }
        else
        {
            if (adapter)
            {
                image->metadata.qid.type = DVM_MODULE_ADAPTER_QID;
                image->metadata.qid.path = plan->names.version;
            }
            answer = parsemod(path, image->bytes, (ulong)image->length, &image->metadata);
        }
    }
    else
    {
        kwerrstr(cancelled || released == -2 ? "provider module load cancelled" :
            mismatch ? "remote scalar interface mismatch" :
            denied ? "adapter projection requires read and execute" :
            refused ? "provider module execution requires an accepted execution-provider launch" :
            "cannot read complete provider module image");
    }
    closeimage(image);
    dvm_scalar_files_release(image->mounted);
    free(image->bytes);
    free(image);
    poperror();
    return answer;
}
