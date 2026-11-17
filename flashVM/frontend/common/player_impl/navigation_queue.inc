/* Copyright (c) 2026 bhgv
 * EN: Shared player navigation_queue implementation; included in owner-thread dependency order.
 * RU: Shared player navigation_queue; подключается в порядке owner-thread зависимостей.
 */


/* A copied request owns its text and one explicit target root until safe-point delivery. */
typedef struct FVM_DesktopRequest
{
    char *url;                       /* Owned reference copied from the VM's counted string. */
    char *base;                      /* Owned origin provenance, independent of selected target. */
    FVM_Object *target;              /* [GC:] Explicitly pinned while queued. */
    unsigned int flags;              /* Load family or private deferred unload operation. */
    int discard;                     /* Staged cancellation decision; publication happens later. */
    struct FVM_DesktopRequest *next; /* Native FIFO link. */
} FVM_DesktopRequest;


/* Per-run immutable bindings carry the owning navigation alongside language-call shape. */
typedef struct FVM_DesktopLoadBuiltin
{
    unsigned int flags;                       /* LOAD_TARGET or LOAD_VARIABLES request family. */
    int method;                               /* Nonzero for MovieClip receiver form. */
    struct FVM_DesktopNavigation *navigation; /* Borrowed until the context is destroyed. */
} FVM_DesktopLoadBuiltin;


/* Per-run transport and request queue; no mutable process-wide registration. */
typedef struct FVM_DesktopNavigation
{
    FVM_DesktopLoadBuiltin builtins[FVM_DESKTOP_LOAD_BINDING_COUNT]; /* Context callback payloads. */
    char *pending;                            /* Owned next root URL after the old VM is released. */
    char *pending_base;                       /* Owned provenance for the next root request. */
    FVM_DesktopRequest *head, *tail;          /* Owned FIFO, drained only outside VM callbacks. */
    size_t count;                             /* Bound queued fetches by the remaining load budget. */
    size_t unload_count;                      /* Independently bound unloads even when fetch budget is zero. */
    unsigned long transfer_id;                /* Native observation ordinal for the active request. */
    FVM_DesktopTransfer *transfer;            /* Owned in-flight native-byte job, joined before GC unpin. */
    FVM_DesktopRequest *active_request;       /* [GC:] Keeps its target pinned across VM turns. */
    FVM_Object *root;                         /* Borrowed active root, pinned by play_bytes. */
    FVM_DesktopAudio *audio;                  /* Borrowed owner-thread audio until play_bytes cleanup. */
    const char *initial_base;                 /* Borrowed initial canonical location or executable resource path. */
    const FVM_DesktopManagedLoader *managed; /* Optional explicit VFS bridge; immutable through this run. */
    const FVM_DesktopLoader *policy;          /* Borrowed for the entire source run. */
    FVM_DesktopBackgroundSession *background; /* Optional borrowed owner-thread transfer session. */
    FVM_DesktopPlaybackSession *control;      /* Borrowed state shared across whole-root navigation. */
    FVM_DesktopFrameSession *frames;          /* Borrowed native observer, independent of managed scene owners. */
    unsigned long remaining;                  /* Remaining requests, including child and variable fetches. */
} FVM_DesktopNavigation;


/* Service is defined after the bounded transport helpers below. */
static FVM_Result service_requests(FVM_Context *, FVM_DesktopNavigation *);


/* [GC:] Release all native FIFO ownership before the corresponding context is destroyed. */
static void discard_requests(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    if (navigation && navigation->transfer)
    {
        FVM_DesktopTransferView view;
        FVM_DesktopRequest *request = navigation->active_request;
        memset(&view, 0, sizeof(view));
        fvm_desktop_transfer_poll(navigation->transfer, &view);
        background_terminal(navigation->background,
                            navigation->transfer_id,
                            request->flags & FVM_AVM1_URL_LOAD_VARIABLES ? FVM_DESKTOP_LOAD_VARIABLES
                                                                         : FVM_DESKTOP_LOAD_CHILD,
                            request->url,
                            request->base,
                            &view,
                            FVM_ERROR,
                            1);
        fvm_desktop_transfer_release(navigation->transfer);
        navigation->transfer = 0;
    }
    if (navigation && navigation->active_request)
    {
        FVM_DesktopRequest *request = navigation->active_request;
        navigation->active_request = 0;
        fvm_gc_unpin(context, request->target);
        free(request->url);
        free(request->base);
        free(request);
    }
    while (navigation && navigation->head)
    {
        FVM_DesktopRequest *request = navigation->head;
        navigation->head = request->next;
        fvm_gc_unpin(context, request->target);
        free(request->url);
        free(request->base);
        free(request);
    }
    if (navigation)
    {
        navigation->tail = 0;
        navigation->count = 0;
        navigation->unload_count = 0;
    }
}


/* [GC:] Join an abandoned job before releasing the target pin or copied request text. */
static void abandon_active(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    FVM_DesktopRequest *request = navigation->active_request;
    FVM_DesktopTransferView view;
    if (!request)
    {
        return;
    }
    memset(&view, 0, sizeof(view));
    fvm_desktop_transfer_poll(navigation->transfer, &view);
    background_terminal(navigation->background,
                        navigation->transfer_id,
                        request->flags & FVM_AVM1_URL_LOAD_VARIABLES ? FVM_DESKTOP_LOAD_VARIABLES
                                                                     : FVM_DESKTOP_LOAD_CHILD,
                        request->url,
                        request->base,
                        &view,
                        FVM_ERROR,
                        1);
    fvm_desktop_transfer_release(navigation->transfer);
    navigation->transfer = 0;
    navigation->active_request = 0;
    fvm_gc_unpin(context, request->target);
    free(request->url);
    free(request->base);
    free(request);
}


/* [GC:] Remove one linked request after its native effects have committed or been discarded. */
static void release_request(FVM_Context *context, FVM_DesktopRequest *request)
{
    fvm_gc_unpin(context, request->target);
    free(request->url);
    free(request->base);
    free(request);
}


/* Resolve the global target argument or method receiver while still in the calling AVM1 scope. */
static FVM_Result unload_builtin(FVM_Context *context,
                                 FVM_Object *receiver,
                                 unsigned int argc,
                                 const FVM_Value *args,
                                 FVM_Value *result,
                                 void *userdata)
{
    FVM_DesktopNavigation *navigation = (FVM_DesktopNavigation *)userdata;
    FVM_AVM1TargetInfo info;
    FVM_Value target;
    FVM_DesktopRequest *request;
    FVM_Result status;
    int attached = 0;
    if (result)
    {
        fvm_value_undefined(result);
    }
    if (!navigation || argc > FVM_DESKTOP_UNLOAD_MAX_ARGS || (argc && !args))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(&target);
    if (!argc)
    {
        target.type = FVM_VALUE_OBJECT;
        target.object = receiver;
    }
    else if (args[0].type == FVM_VALUE_OBJECT)
    {
        target = args[0];
    }
    else if (args[0].type == FVM_VALUE_STRING)
    {
        info.struct_size = sizeof(info);
        status = fvm_avm1_get_target_info(context, &info);
        if (status != FVM_OK || !info.active || !info.base_clip)
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (fvm_value_string_size(args) == strlen("_level0") && !memcmp(args[0].string, "_level0", strlen("_level0")))
        {
            target.type = FVM_VALUE_OBJECT;
            target.object = navigation->root;
        }
        else
        {
            status = fvm_exec_scope_target(context, 0, info.target_clip, info.base_clip, args, &target);
            if (status != FVM_OK)
            {
                return status;
            }
        }
    }
    else
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (target.type != FVM_VALUE_OBJECT || !target.object)
    {
        return FVM_NOT_FOUND;
    }
    status = fvm_movieclip_attached_to(context, target.object, navigation->root, &attached);
    if (status != FVM_OK || !attached)
    {
        return status != FVM_OK ? status : FVM_NOT_FOUND;
    }
    if (navigation->unload_count >= FVM_DESKTOP_MAX_LOADS)
    {
        return FVM_UNSUPPORTED;
    }
    request = (FVM_DesktopRequest *)calloc(1, sizeof(*request));
    if (!request)
    {
        return FVM_OUT_OF_MEMORY;
    }
    request->target = target.object;
    request->flags = FVM_DESKTOP_REQUEST_UNLOAD;
    status = fvm_gc_pin(context, request->target);
    if (status != FVM_OK)
    {
        free(request);
        return status;
    }
    if (navigation->tail)
    {
        navigation->tail->next = request;
    }
    else
    {
        navigation->head = request;
    }
    navigation->tail = request;
    ++navigation->unload_count;
    return FVM_OK;
}


/* Process urgent unloads in script order before polling an in-flight transfer.
 * Only requests preceding the unload are cancelled; later reloads of the instance survive. */
static FVM_Result service_unloads(FVM_Context *context, FVM_DesktopNavigation *navigation)
{
    while (navigation->unload_count)
    {
        FVM_DesktopRequest *unload = navigation->head, *request, *previous;
        FVM_Result status;
        int attached = 0, cancel_active = 0;
        while (unload && unload->flags != FVM_DESKTOP_REQUEST_UNLOAD)
        {
            unload = unload->next;
        }
        if (!unload)
        {
            return FVM_ERROR;
        }
        status = fvm_movieclip_attached_to(context, unload->target, navigation->root, &attached);
        if (status != FVM_OK)
        {
            return status;
        }
        if (attached && navigation->active_request)
        {
            status =
                fvm_movieclip_attached_to(context, navigation->active_request->target, unload->target, &cancel_active);
            if (status != FVM_OK)
            {
                return status;
            }
        }
        /* Stage all subtree decisions before the atomic clear detaches native parent edges. */
        for (request = navigation->head; request != unload; request = request->next)
        {
            request->discard = 0;
            if (attached)
            {
                status = fvm_movieclip_attached_to(context, request->target, unload->target, &request->discard);
                if (status != FVM_OK)
                {
                    return status;
                }
            }
        }
        if (attached)
        {
            status = fvm_movieclip_unload(context, unload->target);
            if (status != FVM_OK)
            {
                return status; /* No cancellation or old-content mutation on allocation failure. */
            }
            if (unload->target == navigation->root)
            { /* Silence PCM before cancellation joins a potentially slow network provider. */
                status = fvm_desktop_audio_clear(context, navigation->audio);
                if (status != FVM_OK)
                {
                    return status; /* Content is committed; owner error cleanup completes teardown.
                                    */
                }
            }
        }
        if (cancel_active)
        {
            abandon_active(context, navigation);
        }
        previous = 0;
        request = navigation->head;
        while (request)
        {
            FVM_DesktopRequest *next = request->next;
            int done = request == unload;
            if (done || request->discard)
            {
                if (previous)
                {
                    previous->next = next;
                }
                else
                {
                    navigation->head = next;
                }
                if (navigation->tail == request)
                {
                    navigation->tail = previous;
                }
                if (done)
                {
                    --navigation->unload_count;
                }
                else
                {
                    --navigation->count;
                }
                release_request(context, request);
            }
            else
            {
                previous = request;
            }
            if (done)
            {
                break;
            }
            request = next;
        }
    }
    return FVM_OK;
}
