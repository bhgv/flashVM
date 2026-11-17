/* Copyright (c) 2026 bhgv
 * EN: Shared player navigation_requests implementation; included in owner-thread dependency order.
 * RU: Shared player navigation_requests; подключается в порядке owner-thread зависимостей.
 */


/* Copy counted source text without accepting embedded NUL or overlong locations. */
static char *copy_location(const FVM_Value *value, FVM_Result *status)
{
    size_t size = fvm_value_string_size(value);
    char *copy;
    if (!size || size > FVM_DESKTOP_MAX_LOCATION || memchr(value->string, 0, size))
    {
        *status = FVM_INVALID_ARGUMENT;
        return 0;
    }
    if (*status != FVM_OK)
    {
        return 0;
    }
    copy = (char *)malloc(size + 1);
    if (!copy)
    {
        *status = FVM_OUT_OF_MEMORY;
    }
    if (copy)
    {
        memcpy(copy, value->string, size);
        copy[size] = 0;
    }
    return copy;
}


/* [GC:] One queue implementation copies URL/origin and pins the exact destination identity.
 * No script callback or transport occurs after membership validation. */
static FVM_Result enqueue_request(FVM_Context *context,
                                  FVM_DesktopNavigation *navigation,
                                  const FVM_Value *url,
                                  const FVM_Value *base,
                                  FVM_Object *target,
                                  unsigned int flags)
{
    FVM_DesktopRequest *request;
    FVM_Result status;
    int attached = 0;
    if (navigation->count >= navigation->remaining)
    {
        return FVM_UNSUPPORTED;
    }
    status = fvm_movieclip_attached_to(context, target, navigation->root, &attached);
    if (status != FVM_OK || !attached)
    {
        return status != FVM_OK ? status : FVM_NOT_FOUND;
    }
    request = (FVM_DesktopRequest *)calloc(1, sizeof(*request));
    if (!request)
    {
        return FVM_OUT_OF_MEMORY;
    }
    request->url = copy_location(url, &status);
    request->base = copy_location(base, &status);
    request->target = target;
    request->flags = flags;
    if (status == FVM_OK)
    {
        status = fvm_gc_pin(context, request->target);
    }
    if (status != FVM_OK)
    {
        free(request->url);
        free(request->base);
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
    ++navigation->count;
    return FVM_OK;
}


/* [GC:] Direct receiver/object targets never round-trip through replaceable names or toString.
 * The enclosing native call barrier retains arguments through URL coercion callbacks. */
static FVM_Result request_direct(FVM_Context *context,
                                 FVM_DesktopNavigation *navigation,
                                 const FVM_Value *url,
                                 FVM_Object *target,
                                 unsigned int flags)
{
    FVM_AVM1TargetInfo info;
    FVM_Value text, base;
    FVM_Result status = fvm_exec_to_string(context, url, &text);
    if (status != FVM_OK)
    {
        return status;
    }
    info.struct_size = sizeof(info);
    status = fvm_avm1_get_target_info(context, &info);
    if (status != FVM_OK || !info.active || !info.base_clip)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_movieclip_get_source(context, info.base_clip, &base);
    if (status != FVM_OK)
    {
        return status;
    }
    return enqueue_request(context, navigation, &text, &base, target, flags);
}


/* Resolve and retain target/origin inside the active frame; transport waits for a safe point. */
static FVM_Result request_movie(FVM_Context *context,
                                FVM_Object *receiver,
                                unsigned int argc,
                                const FVM_Value *args,
                                FVM_Value *result,
                                void *userdata)
{
    FVM_DesktopNavigation *navigation = (FVM_DesktopNavigation *)userdata;
    FVM_AVM1TargetInfo info;
    FVM_Value target, base;
    unsigned int flags;
    FVM_Result status;
    size_t target_size;
    (void)receiver;
    (void)result;
    if (argc != FVM_DESKTOP_URL_ARGUMENTS || !args || !navigation ||
        args[FVM_DESKTOP_URL_URL].type != FVM_VALUE_STRING || args[FVM_DESKTOP_URL_TARGET].type != FVM_VALUE_STRING ||
        args[FVM_DESKTOP_URL_FLAGS].type != FVM_VALUE_NUMBER)
    {
        return FVM_INVALID_ARGUMENT;
    }
    flags = (unsigned int)args[FVM_DESKTOP_URL_FLAGS].number;
    if (((flags & ~FVM_DESKTOP_URL_METHOD_MASK) != FVM_AVM1_URL_LOAD_TARGET &&
         (flags & ~FVM_DESKTOP_URL_METHOD_MASK) != FVM_AVM1_URL_LOAD_VARIABLES) ||
        (flags & FVM_DESKTOP_URL_METHOD_MASK) > FVM_AVM1_URL_METHOD_GET || navigation->count >= navigation->remaining)
    {
        return FVM_UNSUPPORTED;
    }
    info.struct_size = sizeof(info);
    status = fvm_avm1_get_target_info(context, &info);
    if (status != FVM_OK || !info.active || !info.base_clip)
    {
        return FVM_INVALID_ARGUMENT;
    }
    target_size = fvm_value_string_size(args + FVM_DESKTOP_URL_TARGET);
    fvm_value_undefined(&target);
    if (target_size == strlen("_level0") && !memcmp(args[FVM_DESKTOP_URL_TARGET].string, "_level0", target_size))
    {
        target.type = FVM_VALUE_OBJECT;
        target.object = navigation->root;
    }
    else if (!target_size)
    {
        target.type = FVM_VALUE_OBJECT;
        target.object = info.target_clip;
    }
    else
    {
        status =
            fvm_exec_scope_target(context, 0, info.target_clip, info.base_clip, args + FVM_DESKTOP_URL_TARGET, &target);
    }
    if (status != FVM_OK || target.type != FVM_VALUE_OBJECT || !target.object)
    {
        return FVM_NOT_FOUND;
    }
    status = fvm_movieclip_get_source(context, info.base_clip, &base);
    if (status != FVM_OK)
    {
        return status;
    }
    return enqueue_request(context, navigation, args + FVM_DESKTOP_URL_URL, &base, target.object, flags);
}


/* Standard host-facing loader spellings reuse the VM's counted/coerced URL service. */
static FVM_Result load_builtin(FVM_Context *context,
                               FVM_Object *receiver,
                               unsigned int argc,
                               const FVM_Value *args,
                               FVM_Value *result,
                               void *userdata)
{
    const FVM_DesktopLoadBuiltin *builtin = (const FVM_DesktopLoadBuiltin *)userdata;
    FVM_Value target;
    int method_form = builtin->method;
    unsigned int method_index;
    if (!argc || !args)
    {
        return FVM_INVALID_ARGUMENT;
    }
    /* AVM1 unqualified names may resolve through a clip prototype before globals.
     * Preserve both public argument shapes when that shared spelling is inherited. */
    if (method_form && argc >= FVM_DESKTOP_LOAD_GLOBAL_MIN_ARGS &&
        (argc >= FVM_DESKTOP_LOAD_GLOBAL_FULL_ARGS || args[FVM_DESKTOP_LOAD_TARGET_INDEX].type == FVM_VALUE_OBJECT ||
         (args[FVM_DESKTOP_LOAD_TARGET_INDEX].type == FVM_VALUE_STRING &&
          !((fvm_value_string_size(args + FVM_DESKTOP_LOAD_TARGET_INDEX) == strlen("GET") &&
             !memcmp(args[FVM_DESKTOP_LOAD_TARGET_INDEX].string, "GET", strlen("GET"))) ||
            (fvm_value_string_size(args + FVM_DESKTOP_LOAD_TARGET_INDEX) == strlen("POST") &&
             !memcmp(args[FVM_DESKTOP_LOAD_TARGET_INDEX].string, "POST", strlen("POST")))))))
    {
        method_form = 0;
    }
    method_index = method_form ? FVM_DESKTOP_LOAD_METHOD_INDEX : FVM_DESKTOP_LOAD_GLOBAL_METHOD_INDEX;
    if (result)
    {
        fvm_value_undefined(result);
    }
    if (!method_form && argc < FVM_DESKTOP_LOAD_GLOBAL_MIN_ARGS)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (argc > method_index && args[method_index].type != FVM_VALUE_UNDEFINED)
    {
        const FVM_Value *method = args + method_index;
        if (method->type != FVM_VALUE_STRING || fvm_value_string_size(method) != strlen("GET") ||
            memcmp(method->string, "GET", strlen("GET")))
        {
            return FVM_UNSUPPORTED;
        }
    }
    if (method_form)
    {
        return request_direct(context, builtin->navigation, args, receiver, builtin->flags);
    }
    target = args[FVM_DESKTOP_LOAD_TARGET_INDEX];
    if (target.type == FVM_VALUE_OBJECT)
    {
        return request_direct(context, builtin->navigation, args, target.object, builtin->flags);
    }
    return fvm_exec_url(context, args, &target, builtin->flags);
}


/* Numbered level loading currently maps only level zero to the existing root lifecycle.
 * Numeric coercion is explicit; unsupported levels never trigger a transport request. */
static FVM_Result load_numbered_builtin(FVM_Context *context,
                                        FVM_Object *receiver,
                                        unsigned int argc,
                                        const FVM_Value *args,
                                        FVM_Value *result,
                                        void *userdata)
{
    FVM_Value target;
    FVM_Value level;
    FVM_Result status;
    (void)receiver;
    (void)userdata;
    if (result)
    {
        fvm_value_undefined(result);
    }
    if (!args || argc < FVM_DESKTOP_NUMBERED_LOAD_MIN_ARGS || argc > FVM_DESKTOP_NUMBERED_LOAD_MAX_ARGS)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_exec_to_number(context, 0, args + FVM_DESKTOP_NUMBERED_LEVEL_INDEX, &level);
    if (status != FVM_OK)
    {
        return status;
    }
    if (level.number != 0)
    {
        return FVM_UNSUPPORTED;
    }
    if (argc == FVM_DESKTOP_NUMBERED_LOAD_MAX_ARGS &&
        args[FVM_DESKTOP_NUMBERED_METHOD_INDEX].type != FVM_VALUE_UNDEFINED)
    {
        const FVM_Value *method = args + FVM_DESKTOP_NUMBERED_METHOD_INDEX;
        if (method->type != FVM_VALUE_STRING || fvm_value_string_size(method) != strlen("GET") ||
            memcmp(method->string, "GET", strlen("GET")))
        {
            return FVM_UNSUPPORTED;
        }
    }
    fvm_value_string(&target, "_level0");
    return fvm_exec_url(context, args, &target, FVM_AVM1_URL_LOAD_TARGET);
}


/* Numbered root unloading reuses the exact same pin/queue/audio cancellation path. */
static FVM_Result unload_numbered_builtin(FVM_Context *context,
                                          FVM_Object *receiver,
                                          unsigned int argc,
                                          const FVM_Value *args,
                                          FVM_Value *result,
                                          void *userdata)
{
    FVM_DesktopNavigation *navigation = (FVM_DesktopNavigation *)userdata;
    FVM_Value target;
    FVM_Value level;
    FVM_Result status;
    if (!navigation || !args || argc != FVM_DESKTOP_UNLOAD_MAX_ARGS)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_exec_to_number(context, 0, args, &level);
    if (status != FVM_OK)
    {
        return status;
    }
    if (level.number != 0)
    {
        return FVM_UNSUPPORTED;
    }
    fvm_value_undefined(&target);
    target.type = FVM_VALUE_OBJECT;
    target.object = navigation->root;
    return unload_builtin(context, receiver, FVM_DESKTOP_UNLOAD_MAX_ARGS, &target, result, navigation);
}
