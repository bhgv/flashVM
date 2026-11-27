/* Copyright (c) 2026 bhgv
 * EN: Discover checked contracts and reverse scope requirements from the exact module fid.
 * RU: Обнаруживать проверенные контракты и требования встречного контекста по точному fid.
 */


/* EN: Decode one advertised contract, accepting both published forms for Int32.
 * RU: Декодировать один объявленный контракт, включая обе формы Int32. */
static int decode_export(const FVM_SRecord *record, StyxRpcModuleImport *method, char *name)
{
    if (record->tag == STYX_FUNCTION_PROFILE_TAG || record->tag == STYX_FUNCTION_CALLER_PROFILE)
    {
        if (record->size <= INTERFACE_HEADER || record->size > INTERFACE_HEADER + INTERFACE_NAME_LIMIT ||
            record->payload[0] != INTERFACE_KIND)
        {
            return 0;
        }
        size_t size = record->size - INTERFACE_HEADER;
        memcpy(name, record->payload + INTERFACE_HEADER, size);
        name[size] = 0;
        for (size_t i = 1; i < INTERFACE_HEADER; ++i)
        {
            method->signature = method->signature * 256U + record->payload[i];
        }
        method->kind = STYXRPC_MODULE_INT;
        return !memchr(name, 0, size);
    }
    StyxCallShape shape;
    unsigned long request = styxshapedpayload(record->payload, record->size, name, &method->signature, &shape);
    if (request == STYX_FUNCTION_REQUEST_TAG && method->signature == STYXRPC_INT_SIGNATURE)
    {
        method->kind = STYXRPC_MODULE_INT;
        return 1;
    }
    if (request != STYX_MANAGED_REQUEST_TAG)
    {
        return 0;
    }
    memcpy(method->schema.arguments, shape.arguments, sizeof(shape.arguments));
    memcpy(method->schema.result, shape.result, sizeof(shape.result));
    method->kind = strpbrk(shape.arguments, "c") || strpbrk(shape.result, "c") ? STYXRPC_MODULE_CHANNEL :
        styxmanaged_graph_required(shape.arguments) || strpbrk(shape.result, "rh") ?
        STYXRPC_MODULE_GRAPH : STYXRPC_MODULE_MANAGED;
    return 1;
}


/* EN: Reject a duplicate export name even when the signatures differ.
 * RU: Отклонить повтор имени экспорта даже при разных сигнатурах. */
static int name_available(const StyxRpcModuleExports *exports, const char *name)
{
    for (size_t i = 0; i < exports->count; ++i)
    {
        if (!strcmp(exports->names[i], name))
        {
            return 0;
        }
    }
    return 1;
}


/* EN/RU: See module_client.h. / См. module_client.h. */
StyxRpcStatus styxrpc_module_exports_scoped_at(StyxFid *dir, uint32_t pid,
    StyxRpcModuleExports *out, uint32_t *scoped)
{
    if (!dir || !out)
    {
        return STYXRPC_INVALID;
    }
    unsigned char bytes[PROFILE_CAPACITY + 1];
    size_t size = read_interface(dir, bytes, sizeof(bytes));
    StyxRpcModuleExports exports = {0};
    uint32_t mask = 0;
    const unsigned char *cursor = bytes;
    while (size && exports.count < STYXRPC_MAX_EXPORTS)
    {
        FVM_SRecord record;
        size_t used;
        StyxRpcModuleImport *method = exports.methods + exports.count;
        if (fvm_srecord_read(cursor, size, STYX_SHAPED_OFFER_MAX, &record, &used) != FVM_S_OK ||
            !decode_export(&record, method, exports.names[exports.count]))
        {
            break;
        }
        method->name = exports.names[exports.count];
        size_t total = (size_t)(cursor - bytes) + size;
        int caller;
        if (!name_available(&exports, method->name) || !check_import(dir, bytes, total, method, pid, &caller))
        {
            break;
        }
        if (caller)
        {
            mask |= UINT32_C(1) << exports.count;
        }
        ++exports.count;
        cursor += used;
        size -= used;
    }
    if (size || !exports.count)
    {
        return STYXRPC_INVALID;
    }
    *out = exports;
    for (size_t i = 0; i < out->count; ++i)
    {
        out->methods[i].name = out->names[i];
    }
    if (scoped)
    {
        *scoped = mask;
    }
    return STYXRPC_OK;
}


/* EN/RU: See module_client.h. / См. module_client.h. */
StyxRpcStatus styxrpc_module_exports_at(StyxFid *dir, uint32_t pid, StyxRpcModuleExports *out)
{
    return styxrpc_module_exports_scoped_at(dir, pid, out, NULL);
}


/* EN/RU: See module_client.h. / См. module_client.h. */
StyxRpcStatus styxrpc_module_exports(StyxClient *session, const char *path, uint32_t pid, StyxRpcModuleExports *out)
{
    if (!out)
    {
        return STYXRPC_INVALID;
    }
    StyxFid *dir = styxrpc_module_open(session, path);
    StyxRpcStatus status = styxrpc_module_exports_at(dir, pid, out);
    styxclientclunk(dir);
    return status;
}
